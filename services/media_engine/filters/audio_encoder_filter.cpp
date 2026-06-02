/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "audio_encoder_filter.h"

#include "common/log.h"
#include "filter/filter_factory.h"
#include "media_codec/media_codec.h"
#include "common/media_core.h"
#include "avcodec_sysevent.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_define.h"
#ifdef SUPPORT_AUDIO_CONVERT
#include "audio_format_converter.h"
#include "audio_suite_pcm_buffer.h"
#include "audio_stream_info.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_RECORDER, "AudioEncoderFilter" };

// 比特率对应的采样率档位
struct BitrateSampleRateMap {
    std::vector<int64_t> bitrates;  // 离散的比特率值列表
    std::vector<int32_t> supportedSampleRates;
};

// 音频格式参数配置
struct AudioFormatConfig {
    std::string mimeType;
    std::vector<BitrateSampleRateMap> bitrateSampleRateMaps;
    int32_t maxChannels;  // 最大声道数
};

// MP3格式配置表
static const AudioFormatConfig MP3_FORMAT_CONFIG = {
    "audio/mpeg",
    {
        {{8000, 16000}, {8000, 11025, 12000, 16000, 22050, 24000}},
        {{32000, 40000, 48000, 56000, 64000}, {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000}},
        {{80000, 96000, 112000, 128000, 160000}, {16000, 22050, 24000, 32000, 44100, 48000}},
        {{192000, 224000, 256000, 320000}, {32000, 44100, 48000}}
    },
    2  // MP3最大支持2声道
};

// AAC格式配置表（暂不限制采样率和码率关系）
static const AudioFormatConfig AAC_FORMAT_CONFIG = {
    "audio/mp4a-latm",
    {},  // AAC暂不限制采样率和码率关系
    2   // AAC最大支持2声道
};

// RAW格式配置表
static const AudioFormatConfig RAW_FORMAT_CONFIG = {
    "audio/raw",
    {},  // RAW不限制采样率和码率关系
    8   // RAW支持多声道
};

// AMR_NB格式配置表
static const AudioFormatConfig AMR_NB_FORMAT_CONFIG = {
    "audio/3gpp",
    {
        {{4750, 5150, 5900, 6700, 7400, 7950, 10200, 12200}, {8000}}
    },
    1   // AMR_NB仅支持单声道
};

// AMR_WB格式配置表
static const AudioFormatConfig AMR_WB_FORMAT_CONFIG = {
    "audio/amr-wb",
    {
        {{6600, 8850, 12650, 14250, 15850, 18250, 19850, 23050, 23850}, {16000}}
    },
    1   // AMR_WB仅支持单声道
};

}

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace OHOS::MediaAVCodec;
#ifdef SUPPORT_AUDIO_CONVERT
using namespace OHOS::AudioStandard;
using namespace OHOS::AudioStandard::AudioSuite;
#endif
using namespace OHOS::Media::Plugins;
static AutoRegisterFilter<AudioEncoderFilter> g_registerAudioEncoderFilter("builtin.recorder.audioencoder",
    FilterType::FILTERTYPE_AENC,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioEncoderFilter>(name, FilterType::FILTERTYPE_AENC);
    });

class AudioEncoderFilterLinkCallback : public FilterLinkCallback {
public:
    explicit AudioEncoderFilterLinkCallback(std::shared_ptr<AudioEncoderFilter> audioEncoderFilter)
        : audioEncoderFilter_(std::move(audioEncoderFilter))
    {
    }

    ~AudioEncoderFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }
private:
    std::weak_ptr<AudioEncoderFilter> audioEncoderFilter_;
};

class IntermediateBufferAvailableListener : public IConsumerListener {
public:
    explicit IntermediateBufferAvailableListener(std::shared_ptr<AudioEncoderFilter> audioEncoderFilter)
        : audioEncoderFilter_(std::move(audioEncoderFilter))
    {
    }

    ~IntermediateBufferAvailableListener() override = default;

    void OnBufferAvailable() override
    {
        if (auto filter = audioEncoderFilter_.lock()) {
            filter->NotifyProcessingThread();
        }
    }

private:
    std::weak_ptr<AudioEncoderFilter> audioEncoderFilter_;
};

AudioEncoderFilter::AudioEncoderFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    MEDIA_LOG_I("audio encoder filter create");
}

AudioEncoderFilter::~AudioEncoderFilter()
{
    MEDIA_LOG_I("audio encoder filter destroy");

    if (isTranscoderMode_) {
        // 停止处理线程
        StopProcessingThread();

        // 清理中间队列
        intermediateBufferQueue_ = nullptr;
        intermediateQueueProducer_ = nullptr;
        intermediateQueueConsumer_ = nullptr;
    }
}

Status AudioEncoderFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType_), Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void AudioEncoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    mediaCodec_ = std::make_shared<MediaCodec>();
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec is nullptr");
    int32_t ret = mediaCodec_->Init(codecMimeType_, true);
    if (ret != 0 && isTranscoderMode_) {
        MEDIA_LOG_I("TranscoderMode");
        FALSE_RETURN(eventReceiver_ != nullptr);
        eventReceiver_->OnEvent({"audio_encoder_filter", EventType::EVENT_ERROR, MSERR_UNSUPPORT_AUD_ENC_TYPE});
    }
}

Status AudioEncoderFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    configureParameter_ = parameter;
    FALSE_RETURN_V_NOLOG(!isTranscoderMode_, Status::OK);
    MEDIA_LOG_I("Configure");
    FALSE_RETURN_V(mediaCodec_ != nullptr, Status::ERROR_NULL_POINTER);
    int32_t ret = mediaCodec_->Configure(parameter);
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::Configure error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

sptr<Surface> AudioEncoderFilter::GetInputSurface()
{
    FALSE_RETURN_V(mediaCodec_ != nullptr, nullptr);
    MEDIA_LOG_I("GetInputSurface");
    return mediaCodec_->GetInputSurface();
}

Status AudioEncoderFilter::DoPrepare()
{
    FALSE_RETURN_V(filterCallback_ != nullptr, Status::ERROR_NULL_POINTER);
    MEDIA_LOG_I("Prepare");
    switch (filterType_) {
        case FilterType::FILTERTYPE_AENC:
            if (isTranscoderMode_) {
                MEDIA_LOG_I("TranscoderMode");
                return filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                    StreamType::STREAMTYPE_ENCODED_AUDIO);
            }
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_ENCODED_AUDIO);
            break;
        default:
            break;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoStart()
{
    FALSE_RETURN_V(mediaCodec_ != nullptr, Status::ERROR_NULL_POINTER);
    MEDIA_LOG_I("Start");
    int32_t ret = mediaCodec_->Start();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoStart error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status AudioEncoderFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status AudioEncoderFilter::DoStop()
{
    FALSE_RETURN_V(mediaCodec_ != nullptr, Status::ERROR_NULL_POINTER);
    MEDIA_LOG_I("Stop");
    int32_t ret = mediaCodec_->Stop();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoStop error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoFlush()
{
    FALSE_RETURN_V(mediaCodec_ != nullptr, Status::ERROR_NULL_POINTER);
    MEDIA_LOG_I("Flush");
    int32_t ret = mediaCodec_->Flush();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoFlush error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::DoRelease()
{
    FALSE_RETURN_V(mediaCodec_ != nullptr, Status::ERROR_NULL_POINTER);
    MEDIA_LOG_I("Release");

    if (isTranscoderMode_) {
        // 停止处理线程
        StopProcessingThread();
#ifdef SUPPORT_AUDIO_CONVERT
        // 清理转换器
        if (channelConverter_ != nullptr) {
            channelConverter_->Destroy();
            channelConverter_ = nullptr;
        }
#endif
        needChannelConvert_ = false;

        // 清理中间队列
        intermediateQueueProducer_ = nullptr;
        intermediateQueueConsumer_ = nullptr;
        intermediateBufferQueue_ = nullptr;

        {
            std::lock_guard<std::mutex> lock(producerMutex_);
            mediaCodecInputProducer = nullptr;
        }
    }

    int32_t ret = mediaCodec_->Release();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::DoRelease error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::NotifyEos()
{
    FALSE_RETURN_V(mediaCodec_ != nullptr, Status::ERROR_NULL_POINTER);
    MEDIA_LOG_I("NotifyEos");
    int32_t ret = mediaCodec_->NotifyEos();
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::NotifyEos error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::SetTranscoderMode()
{
    MEDIA_LOG_I("SetTranscoderMode");
    isTranscoderMode_ = true;
    return Status::OK;
}

void AudioEncoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec is nullptr");
    MEDIA_LOG_I("SetParameter");
    mediaCodec_->SetParameter(parameter);
}

void AudioEncoderFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status AudioEncoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<AudioEncoderFilterLinkCallback>(shared_from_this());
    if (mediaCodec_) {
        std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
        mediaCodec_->GetOutputFormat(parameter);
        int32_t frameSize = 0;
        if (parameter->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != parameter->end() &&
            parameter->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize)) {
            configureParameter_->Set<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize);
        }
    }
    auto ret = nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    if (ret != Status::OK) {
        SetFaultEvent("AudioEncoderFilter::LinkNext error", (int32_t)ret);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "OnLinked failed");
    return Status::OK;
}

Status AudioEncoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status AudioEncoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType AudioEncoderFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
}

Status AudioEncoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    onLinkedResultCallback_ = callback;
    if (isTranscoderMode_) {
        transcoderMeta_ = meta;
        return UpdateParameterToConfigure(meta);
    }
    return Status::OK;
}

// 根据MIME类型获取格式配置
static const AudioFormatConfig* GetFormatConfig(const std::string& mimeType)
{
    if (mimeType == MP3_FORMAT_CONFIG.mimeType) {
        return &MP3_FORMAT_CONFIG;
    } else if (mimeType == AAC_FORMAT_CONFIG.mimeType) {
        return &AAC_FORMAT_CONFIG;
    } else if (mimeType == RAW_FORMAT_CONFIG.mimeType) {
        return &RAW_FORMAT_CONFIG;
    } else if (mimeType == AMR_NB_FORMAT_CONFIG.mimeType) {
        return &AMR_NB_FORMAT_CONFIG;
    } else if (mimeType == AMR_WB_FORMAT_CONFIG.mimeType) {
        return &AMR_WB_FORMAT_CONFIG;
    }
    return nullptr;
}

// 在支持的码率中查找最接近输入码率的值
static int64_t FindClosestBitrate(const AudioFormatConfig* formatConfig, int64_t targetBitrate)
{
    if (formatConfig == nullptr || formatConfig->bitrateSampleRateMaps.empty()) {
        return targetBitrate;
    }

    int64_t closestBitrate = targetBitrate;
    int64_t minDiff = INT64_MAX;

    // 遍历所有码率档位
    for (const auto& map : formatConfig->bitrateSampleRateMaps) {
        for (int64_t bitrate : map.bitrates) {
            int64_t diff = std::abs(bitrate - targetBitrate);
            if (diff < minDiff) {
                minDiff = diff;
                closestBitrate = bitrate;
            }
        }
    }

    return closestBitrate;
}

// 根据码率查找支持的采样率范围
static std::vector<int32_t> FindSupportedSampleRates(const AudioFormatConfig* formatConfig, int64_t bitrate)
{
    if (formatConfig == nullptr || formatConfig->bitrateSampleRateMaps.empty()) {
        return {};
    }

    // 先找到最接近的码率
    int64_t closestBitrate = FindClosestBitrate(formatConfig, bitrate);

    // 根据最接近的码率查找对应的采样率范围
    for (const auto& map : formatConfig->bitrateSampleRateMaps) {
        for (int64_t supportedBitrate : map.bitrates) {
            if (supportedBitrate == closestBitrate) {
                return map.supportedSampleRates;
            }
        }
    }

    // 如果没有匹配的码率，返回空
    return {};
}

// 在支持的采样率中查找最接近输入采样率的值
static int32_t FindClosestSampleRate(const std::vector<int32_t>& supportedRates, int32_t inputRate)
{
    if (supportedRates.empty()) {
        return inputRate;
    }

    auto it = std::min_element(supportedRates.begin(), supportedRates.end(),
        [inputRate](int32_t a, int32_t b) {
            return std::abs(a - inputRate) < std::abs(b - inputRate);
        });

    return (it != supportedRates.end()) ? *it : inputRate;
}

// 根据格式和输入声道数调整输出声道数
static int32_t AdjustChannelCount(const AudioFormatConfig* formatConfig, int32_t inputChannels)
{
    if (formatConfig == nullptr) {
        return 1;  // 默认单声道
    }

    // 输入为单声道时不调整
    if (inputChannels <= 1) {
        return inputChannels;
    }

    // 根据MIME类型调整声道数
    if (formatConfig->mimeType == "audio/raw") {
        // RAW格式不变化
        return std::min(inputChannels, formatConfig->maxChannels);
    } else if (formatConfig->mimeType == "audio/mpeg" || formatConfig->mimeType == "audio/mp4a-latm") {
        // MP3/AAC格式转为双声道
        return std::min(2, formatConfig->maxChannels);
    } else if (formatConfig->mimeType == "audio/3gpp" || formatConfig->mimeType == "audio/amr-wb") {
        // AMR格式转为单声道
        return 1;
    } else {
        // 其余格式转为单声道
        return 1;
    }
}

Status AudioEncoderFilter::UpdateParameterToConfigure(const std::shared_ptr<Meta> &meta)
{
    Plugins::AudioSampleFormat dstSampleFormat = Plugins::AudioSampleFormat::INVALID_WIDTH;
    Plugins::AudioSampleFormat oriSampleFormat = Plugins::AudioSampleFormat::INVALID_WIDTH;
    if (meta != nullptr && configureParameter_ != nullptr && meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, dstSampleFormat)) {
        configureParameter_->GetData(Tag::AUDIO_SAMPLE_FORMAT, oriSampleFormat);
        MEDIA_LOG_I("Configure, sampleFormat: " PUBLIC_LOG_D32 " -> " PUBLIC_LOG_D32,
            static_cast<int32_t>(oriSampleFormat), static_cast<int32_t>(dstSampleFormat));
        configureParameter_->SetData(Tag::AUDIO_SAMPLE_FORMAT, dstSampleFormat);
    }
    if (isTranscoderMode_) {
        int32_t outputbuffersize = 0;
        if (meta->GetData(Tag::AUDIO_MAX_OUTPUT_SIZE, outputbuffersize)) {
            MEDIA_LOG_I("AUDIO_MAX_OUTPUT_SIZE is outputbuffersize: " PUBLIC_LOG_D32,
                static_cast<int32_t>(outputbuffersize));
        } else {
            MEDIA_LOG_I("not get AUDIO_MAX_OUTPUT_SIZE");
        }

        // 获取输入PCM参数
        int32_t inputSampleRate = 0;
        int32_t inputChannelCount = 0;
        int64_t inputBitrate = 0;
        std::string inputMimeType = Plugins::MimeType::AUDIO_AAC;

        if (transcoderMeta_ != nullptr) {
            transcoderMeta_->GetData(Tag::AUDIO_SAMPLE_RATE, inputSampleRate);
            transcoderMeta_->GetData(Tag::AUDIO_CHANNEL_COUNT, inputChannelCount);
            transcoderMeta_->GetData(Tag::MEDIA_BITRATE, inputBitrate);
            transcoderMeta_->GetData(Tag::MIME_TYPE, inputMimeType);
        }

        // 获取输出配置参数
        int32_t outputSampleRate = 0;
        int32_t outputChannelCount = 0;
        int64_t outputBitrate = 0;
        std::string outputMimeType = Plugins::MimeType::AUDIO_AAC;

        if (configureParameter_ != nullptr) {
            configureParameter_->GetData(Tag::AUDIO_SAMPLE_RATE, outputSampleRate);
            configureParameter_->GetData(Tag::AUDIO_CHANNEL_COUNT, outputChannelCount);
            configureParameter_->GetData(Tag::MEDIA_BITRATE, outputBitrate);
            configureParameter_->GetData(Tag::MIME_TYPE, outputMimeType);
        }

        MEDIA_LOG_I("UpdateParameterToConfigure: input mime=%{public}s, sampleRate=%{public}d",
            inputMimeType.c_str(), inputSampleRate);
        MEDIA_LOG_I("input channels=%{public}d, bitrate=%{public}lld",
            inputChannelCount, static_cast<long long>(inputBitrate));
        MEDIA_LOG_I("UpdateParameterToConfigure: output mime=%{public}s, sampleRate=%{public}d",
            outputMimeType.c_str(), outputSampleRate);
        MEDIA_LOG_I("output channels=%{public}d, bitrate=%{public}lld",
            outputChannelCount, static_cast<long long>(outputBitrate));

        // 获取格式配置
        const AudioFormatConfig* formatConfig = GetFormatConfig(outputMimeType);

        // 调整后的参数
        int32_t adjustedSampleRate = inputSampleRate;
        int64_t adjustedBitrate = outputBitrate;
        int32_t adjustedChannelCount = inputChannelCount;

        if (formatConfig != nullptr) {
            // 1. 码率优先判断：若码率未设置，使用输入码率
            if (adjustedBitrate <= 0 && inputBitrate > 0) {
                adjustedBitrate = inputBitrate;
            }

            // 2. 查找最接近的离散码率值
            int64_t originalBitrate = adjustedBitrate;
            adjustedBitrate = FindClosestBitrate(formatConfig, adjustedBitrate);
            if (originalBitrate != adjustedBitrate) {
                MEDIA_LOG_I("Bitrate adjusted from %{public}lld to %{public}lld (closest standard bitrate)",
                            static_cast<long long>(originalBitrate), static_cast<long long>(adjustedBitrate));
            }

            // 3. 根据码率查找支持的采样率范围
            std::vector<int32_t> supportedSampleRates = FindSupportedSampleRates(formatConfig, adjustedBitrate);

            // 4. 在支持的采样率中查找最接近输入采样率的值
            if (!supportedSampleRates.empty()) {
                adjustedSampleRate = FindClosestSampleRate(supportedSampleRates, inputSampleRate);
            } else {
                // 如果没有采样率限制，使用输出配置值或输入值
                adjustedSampleRate = (outputSampleRate > 0) ? outputSampleRate : inputSampleRate;
            }

            // 5. 根据格式调整声道数
            adjustedChannelCount = AdjustChannelCount(formatConfig, inputChannelCount);

            MEDIA_LOG_I("FormatConfig: mime=%{public}s, adjustedSampleRate=%{public}d",
                formatConfig->mimeType.c_str(), adjustedSampleRate);
            MEDIA_LOG_I("FormatConfig: adjustedChannels=%{public}d, adjustedBitrate=%{public}lld",
                adjustedChannelCount, static_cast<long long>(adjustedBitrate));
        } else {
            // 未知格式，使用默认处理
            MEDIA_LOG_I("Unknown format: %{public}s, using default values", outputMimeType.c_str());
            adjustedSampleRate = (outputSampleRate > 0) ? outputSampleRate : inputSampleRate;
            adjustedBitrate = (outputBitrate > 0) ? outputBitrate : inputBitrate;
            adjustedChannelCount = 1;  // 默认单声道
        }

        // 设置调整后的参数
        FALSE_RETURN_V(configureParameter_ != nullptr, Status::ERROR_NULL_POINTER);
        configureParameter_->Set<Tag::AUDIO_SAMPLE_RATE>(adjustedSampleRate);
        configureParameter_->Set<Tag::MEDIA_BITRATE>(adjustedBitrate);
        configureParameter_->Set<Tag::AUDIO_CHANNEL_COUNT>(adjustedChannelCount);
        configureParameter_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(
            adjustedChannelCount == 1 ? AudioChannelLayout::MONO : AudioChannelLayout::STEREO);

        int32_t buffersize = 4096 * adjustedChannelCount * 16;
        if (outputbuffersize != 0) {
            configureParameter_->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(outputbuffersize);
        } else {
            configureParameter_->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(buffersize);
            MEDIA_LOG_I("AUDIO_MAX_OUTPUT_SIZE is buffersize: " PUBLIC_LOG_D32,
                static_cast<int32_t>(buffersize));
        }
    }

    FALSE_RETURN_V(mediaCodec_ != nullptr, Status::ERROR_NULL_POINTER);
    int32_t ret = mediaCodec_->Configure(configureParameter_);
    if (ret != 0) {
        SetFaultEvent("AudioEncoderFilter::UpdateParameterToConfigure error", ret);
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status AudioEncoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void AudioEncoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec is nullptr");
    FALSE_RETURN_MSG(onLinkedResultCallback_ != nullptr, "onLinkedResultCallback is nullptr");
    MEDIA_LOG_I("OnLinkedResult");
    mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
    mediaCodec_->Prepare();

    if (isTranscoderMode_) {
        // 获取输入音频格式信息
        int32_t transcoderSampleRate = 0;
        int32_t transcoderChannelCount = 0;
        Plugins::AudioChannelLayout transcoderChannelLayout = AudioChannelLayout::MONO;
        Plugins::AudioSampleFormat transcoderSampleFormat = Plugins::AudioSampleFormat::SAMPLE_S16LE;
        if (transcoderMeta_ != nullptr) {
            transcoderMeta_->GetData(Tag::AUDIO_SAMPLE_RATE, transcoderSampleRate);
            transcoderMeta_->GetData(Tag::AUDIO_CHANNEL_COUNT, transcoderChannelCount);
            MEDIA_LOG_I("TranscoderMeta audio format: channels=%{public}d, sampleRate=%{public}d",
                        transcoderChannelCount, transcoderSampleRate);
            transcoderMeta_->GetData(Tag::AUDIO_CHANNEL_LAYOUT, transcoderChannelLayout);
            transcoderMeta_->GetData(Tag::AUDIO_SAMPLE_FORMAT, transcoderSampleFormat);
            MEDIA_LOG_I("TranscoderMeta audio format: channellayout=%{public}u, sampleFormat=%{public}d",
                        static_cast<uint32_t>(transcoderChannelLayout), static_cast<int32_t>(transcoderSampleFormat));
        }

        int32_t metaChannelCount = 0;
        int32_t metaSampleRate = 0;
        Plugins::AudioChannelLayout metaChannelLayout = AudioChannelLayout::MONO;
        Plugins::AudioSampleFormat metaSampleFormat = Plugins::AudioSampleFormat::SAMPLE_S16LE;
        if (meta != nullptr) {
            meta->GetData(Tag::AUDIO_SAMPLE_RATE, metaSampleRate);
            meta->GetData(Tag::AUDIO_CHANNEL_COUNT, metaChannelCount);
            MEDIA_LOG_I("meta audio format: channels=%{public}d, sampleRate=%{public}d",
                        metaChannelCount, metaSampleRate);
            meta->GetData(Tag::AUDIO_CHANNEL_LAYOUT, metaChannelLayout);
            meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, metaSampleFormat);
            MEDIA_LOG_I("meta audio format: channellayout=%{public}u, sampleFormat=%{public}d",
                        static_cast<uint32_t>(metaChannelLayout), static_cast<int32_t>(metaSampleFormat));
        }
        // 检查是否需要声道转换（输入声道数 > 1 时需要转为单声道）
#ifdef SUPPORT_AUDIO_CONVERT
        needChannelConvert_ = (transcoderChannelCount != metaChannelCount) || (transcoderSampleRate != metaSampleRate);

        if (needChannelConvert_) {
            // 初始化转换器输入格式（使用 AudioStandard 命名空间的类型）
            converterInputFormat_.sampleRate = static_cast<AudioStandard::AudioSamplingRate>(transcoderSampleRate);
            converterInputFormat_.channelLayout = 
                static_cast<AudioStandard::AudioChannelLayout>(transcoderChannelLayout);
            converterInputFormat_.sampleFormat = static_cast<AudioStandard::AudioSampleFormat>(transcoderSampleFormat);
            converterInputFormat_.channelCount = transcoderChannelCount;

            // 初始化转换器输出格式（单声道）
            converterOutputFormat_.sampleRate = static_cast<AudioStandard::AudioSamplingRate>(metaSampleRate);
            converterOutputFormat_.channelLayout = static_cast<AudioStandard::AudioChannelLayout>(metaChannelLayout);
            converterOutputFormat_.sampleFormat = static_cast<AudioStandard::AudioSampleFormat>(metaSampleFormat);
            converterOutputFormat_.channelCount = metaChannelCount;

            // 创建转换器
            channelConverter_ = AudioFormatConverter::Create(
                converterInputFormat_, converterOutputFormat_);
            if (channelConverter_ == nullptr) {
                MEDIA_LOG_E("Failed to create channel converter");
                needChannelConvert_ = false;
            } else {
                needChannelConvert_ = true;
                MEDIA_LOG_I("Created channel converter: %{public}d channels -> 1 channel",
                            transcoderChannelCount);
            }
        }
#endif

        // 创建中间缓存队列
        constexpr int32_t INTERMEDIATE_BUFFER_NUM = 8;
        intermediateBufferQueue_ = AVBufferQueue::Create(INTERMEDIATE_BUFFER_NUM,
            MemoryType::SHARED_MEMORY, "AudioEncoderIntermediateQueue");
        FALSE_RETURN_MSG(intermediateBufferQueue_ != nullptr, "intermediateBufferQueue_ is nullptr");

        intermediateQueueProducer_ = intermediateBufferQueue_->GetProducer();
        intermediateQueueConsumer_ = intermediateBufferQueue_->GetConsumer();

        // 设置监听器（监听器会通知独立线程）
        sptr<IConsumerListener> intermediateListener_ = new IntermediateBufferAvailableListener(shared_from_this());
        intermediateQueueConsumer_->SetBufferAvailableListener(intermediateListener_);

        // 获取 MediaCodec 的输入队列生产者（必须在启动处理线程前初始化）
        {
            std::lock_guard<std::mutex> lock(producerMutex_);
            mediaCodecInputProducer = mediaCodec_->GetInputBufferQueue();
            FALSE_RETURN_MSG(mediaCodecInputProducer != nullptr, "mediaCodecInputProducer is nullptr");
        }

        // 启动独立处理线程
        isProcessingRunning_.store(true);
        processingThread_ = std::thread(&AudioEncoderFilter::ProcessingThreadFunc, this);

        // 将中间队列的生产者返回给上一个 Filter
        if (isTranscoderMode_) {
            onLinkedResultCallback_->OnLinkedResult(intermediateQueueProducer_, transcoderMeta_);
        } else {
            onLinkedResultCallback_->OnLinkedResult(intermediateQueueProducer_, meta);
        }

        MEDIA_LOG_I("OnLinkedResult completed, processing thread started");
        return;
    }
    onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), meta);
}

void AudioEncoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
    (void) meta;
}

void AudioEncoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
    (void) meta;
}

void AudioEncoderFilter::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    SetFaultEvent(errMsg + ", ret = " + std::to_string(ret));
}

void AudioEncoderFilter::SetFaultEvent(const std::string &errMsg)
{
    AudioCodecFaultInfo audioCodecFaultInfo;
    audioCodecFaultInfo.appName = bundleName_;
    audioCodecFaultInfo.instanceId = std::to_string(instanceId_);
    audioCodecFaultInfo.callerType ="player_framework";
    audioCodecFaultInfo.audioCodec = codecMimeType_;
    audioCodecFaultInfo.errMsg = errMsg;
    FaultAudioCodecEventWrite(audioCodecFaultInfo);
}

void AudioEncoderFilter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}

void AudioEncoderFilter::NotifyProcessingThread()
{
    {
        std::unique_lock<std::mutex> lock(processingMutex_);
        hasPendingBuffer_.store(true);
    }
    processingCondVar_.notify_one();
}

void AudioEncoderFilter::ProcessingThreadFunc()
{
    MEDIA_LOG_I("ProcessingThreadFunc started");

    while (isProcessingRunning_.load()) {
        std::unique_lock<std::mutex> lock(processingMutex_);

        // 等待有数据或停止信号
        processingCondVar_.wait(lock, [this] {
            return hasPendingBuffer_.load() || !isProcessingRunning_.load();
        });

        if (!isProcessingRunning_.load()) {
            break;
        }

        // 处理所有待处理的 buffer
        hasPendingBuffer_.store(false);
        lock.unlock();

        // 处理中间队列的数据
        ProcessIntermediateBuffer();
    }

    MEDIA_LOG_I("ProcessingThreadFunc stopped");
}

void AudioEncoderFilter::ProcessIntermediateBuffer()
{
    FALSE_RETURN_MSG(intermediateQueueConsumer_ != nullptr, "intermediateQueueConsumer_ is nullptr");
    FALSE_RETURN_MSG(mediaCodec_ != nullptr, "mediaCodec_ is nullptr");

    // 循环处理队列中的所有 buffer
    Status ret;
    do {
        std::shared_ptr<AVBuffer> intermediateBuffer;
        ret = intermediateQueueConsumer_->AcquireBuffer(intermediateBuffer);

        if (ret != Status::OK || intermediateBuffer == nullptr) {
            break;
        }

        // 打上 eos 标签
        MEDIA_LOG_D("Processing buffer, before flag: 0x%{public}u", intermediateBuffer->flag_);

        // 从 MediaCodec 的输入队列请求 buffer
        std::shared_ptr<AVBuffer> codecBuffer;
        AVBufferConfig config;
        Status requestRet;
        {
            std::lock_guard<std::mutex> lock(producerMutex_);
            if (mediaCodecInputProducer == nullptr) {
                MEDIA_LOG_E("mediaCodecInputProducer is nullptr");
                intermediateQueueConsumer_->ReleaseBuffer(intermediateBuffer);
                break;
            }
            // 使用 -1 表示无限等待，直到有可用 buffer
            requestRet = mediaCodecInputProducer->RequestBuffer(codecBuffer, config, 8);
        }

        if (requestRet != Status::OK || codecBuffer == nullptr || codecBuffer->memory_ == nullptr) {
            MEDIA_LOG_E("RequestBuffer from mediaCodec failed, ret: %{public}d", requestRet);
            intermediateQueueConsumer_->ReleaseBuffer(intermediateBuffer);
            // RequestBuffer 失败可能是暂时的，继续处理下一个 buffer
            continue;
        }

#ifdef SUPPORT_AUDIO_CONVERT
        // 如果需要声道转换，直接转换到 codecBuffer
        if (needChannelConvert_ && channelConverter_ != nullptr) {
            MEDIA_LOG_I("is converting");
            // 直接使用 codecBuffer 的容量作为输出大小，确保不会缓存数据
            uint32_t outputSize = codecBuffer->memory_->GetCapacity();

            // 设置转换器输入回调
            auto callback = std::make_shared<ConverterInputCallback>(intermediateBuffer);
            channelConverter_->SetInputCallback(callback);

            // 直接转换到 codecBuffer
            uint32_t actualOutputSize = 0;
            int32_t convertRet = channelConverter_->Process(
                codecBuffer->memory_->GetAddr(), outputSize, &actualOutputSize);

            if (convertRet != 0 || actualOutputSize == 0) {
                MEDIA_LOG_E("Channel conversion failed, ret: %{public}d", convertRet);
                intermediateQueueConsumer_->ReleaseBuffer(intermediateBuffer);
                continue;
            }

            // 设置实际输出大小
            actualOutputSize = actualOutputSize > codecBuffer->memory_->GetCapacity() ?
                codecBuffer->memory_->GetCapacity() : actualOutputSize;
            codecBuffer->memory_->SetSize(actualOutputSize);

            // 复制属性
            codecBuffer->flag_ = intermediateBuffer->flag_;
            codecBuffer->pts_ = intermediateBuffer->pts_;
            codecBuffer->dts_ = intermediateBuffer->dts_;
            codecBuffer->duration_ = intermediateBuffer->duration_;

            // 释放原始 buffer
            intermediateQueueConsumer_->ReleaseBuffer(intermediateBuffer);
        } else {
#endif
            MEDIA_LOG_I("no need convert");
            // 不需要转换，直接复制数据
            codecBuffer->flag_ = intermediateBuffer->flag_;
            codecBuffer->pts_ = intermediateBuffer->pts_;
            codecBuffer->dts_ = intermediateBuffer->dts_;
            codecBuffer->duration_ = intermediateBuffer->duration_;

            // 复制数据
            if (intermediateBuffer->memory_ != nullptr && codecBuffer->memory_ != nullptr) {
                auto srcData = intermediateBuffer->memory_->GetAddr();
                auto dstData = codecBuffer->memory_->GetAddr();
                auto srcSize = intermediateBuffer->memory_->GetSize();
                auto dstCapacity = codecBuffer->memory_->GetCapacity();

                if (srcData != nullptr && dstData != nullptr && srcSize > 0 && dstCapacity >= srcSize) {
                    memcpy_s(dstData, dstCapacity, srcData, srcSize);
                    codecBuffer->memory_->SetSize(srcSize);
                }
            }

            // 释放中间队列的 buffer
            intermediateQueueConsumer_->ReleaseBuffer(intermediateBuffer);
#ifdef SUPPORT_AUDIO_CONVERT
        }
#endif

        // 转发到 MediaCodec 的输入队列
        Status pushRet;
        {
            std::lock_guard<std::mutex> lock(producerMutex_);
            if (mediaCodecInputProducer != nullptr) {
                pushRet = mediaCodecInputProducer->PushBuffer(codecBuffer, true);
            } else {
                pushRet = Status::ERROR_NULL_POINTER;
                MEDIA_LOG_E("mediaCodecInputProducer is nullptr when pushing buffer");
            }
        }

        if (pushRet != Status::OK) {
            MEDIA_LOG_E("PushBuffer to mediaCodec failed, ret: %{public}d", pushRet);
            // PushBuffer 失败，继续处理下一个 buffer
            continue;
        }
    } while (ret == Status::OK);
}

void AudioEncoderFilter::StopProcessingThread()
{
    MEDIA_LOG_I("StopProcessingThread called");

    // 停止标志
    isProcessingRunning_.store(false);

    // 唤醒线程
    {
        std::unique_lock<std::mutex> lock(processingMutex_);
        hasPendingBuffer_.store(true);
    }
    processingCondVar_.notify_all();

    // 等待线程结束
    if (processingThread_.joinable()) {
        processingThread_.join();
    }

    MEDIA_LOG_I("StopProcessingThread completed");
}

#ifdef SUPPORT_AUDIO_CONVERT
AudioEncoderFilter::ConverterInputCallback::ConverterInputCallback(const std::shared_ptr<AVBuffer>& buffer)
    : sourceBuffer_(buffer), readOffset_(0) {}

int32_t AudioEncoderFilter::ConverterInputCallback::OnRequestData(
    const void** data, InputDataStatus* status)
{
    if (sourceBuffer_ == nullptr || sourceBuffer_->memory_ == nullptr) {
        if (status != nullptr) {
            *status = InputDataStatus::DATA_FINISHED;
        }
        return 0;
    }

    auto totalSize = sourceBuffer_->memory_->GetSize();
    if (readOffset_ >= totalSize) {
        if (status != nullptr) {
            *status = InputDataStatus::DATA_FINISHED;
        }
        return 0;
    }

    if (data != nullptr) {
        *data = static_cast<const uint8_t*>(sourceBuffer_->memory_->GetAddr()) + readOffset_;
    }

    size_t remainingSize = totalSize - readOffset_;
    readOffset_ = totalSize; // 下次调用返回 DATA_FINISHED

    if (status != nullptr) {
        // 根据 sourceBuffer 的 flag 判断是否为 EOS
        // 如果是 EOS，返回 DATA_FINISHED；否则返回 HAVE_DATA
        if (sourceBuffer_->flag_ & 1) {
            *status = InputDataStatus::DATA_FINISHED;
        } else {
            *status = InputDataStatus::HAVE_DATA;
        }
    }

    return static_cast<int32_t>(remainingSize);
}
#endif
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS