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
#include "avcodec_trace.h"
#ifdef SUPPORT_AUDIO_CONVERT
#include "audio_format_converter.h"
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

        if (mediaCodec_ != nullptr) {
            mediaCodec_->Release();
            mediaCodec_ = nullptr;
        }
        // 清理中间队列
        intermediateBufferQueue_ = nullptr;
        intermediateQueueProducer_ = nullptr;
        intermediateQueueConsumer_ = nullptr;
#ifdef SUPPORT_AUDIO_CONVERT
        if (channelConverter_ != nullptr) {
            channelConverter_->Destroy();
            channelConverter_ = nullptr;
        }
#endif
        if (mediaCodecInputProducer != nullptr) {
            mediaCodecInputProducer = nullptr;
        }
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
        eventReceiver_->OnEvent({"audio_encoder_filter", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_UNSUPPORT_AUD_ENC_TYPE)});
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
        MEDIA_LOG_E("audioEncoder start fail");
        return Status::ERROR_AUD_ENC_START_FAILED;
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
        MEDIA_LOG_E("audioEncoder stop fail");
        return Status::ERROR_AUD_ENC_STOP_FAILED;
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
        needChannelConvert_.store(false);

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
        MEDIA_LOG_E("audioEncoder release fail");
        return Status::ERROR_FRAMEWORK_ERR;
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

Status AudioEncoderFilter::SetPureAudioMode()
{
    MEDIA_LOG_I("SetPureAudioMode");
    isPureAudioMode_.store(true);
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
    constexpr int32_t CHANNEL_MONO = 1;
    constexpr int32_t CHANNEL_STEREO = 2;
    if (formatConfig == nullptr) {
        return CHANNEL_MONO;  // 默认单声道
    }

    // 输入为单声道时不调整
    if (inputChannels <= CHANNEL_MONO) {
        return inputChannels;
    }

    // 根据MIME类型调整声道数
    if (formatConfig->mimeType == "audio/raw") {
        // RAW格式不变化
        return std::min(inputChannels, formatConfig->maxChannels);
    } else if (formatConfig->mimeType == "audio/mpeg" || formatConfig->mimeType == "audio/mp4a-latm") {
        // MP3/AAC格式转为双声道
        return std::min(CHANNEL_STEREO, formatConfig->maxChannels);
    } else if (formatConfig->mimeType == "audio/3gpp" || formatConfig->mimeType == "audio/amr-wb") {
        // AMR格式转为单声道
        return CHANNEL_MONO;
    } else {
        // 其余格式转为单声道
        return CHANNEL_MONO;
    }
}

void AudioEncoderFilter::GetAudioPCMConfigure(const std::shared_ptr<Meta> &meta,
    std::shared_ptr<AudioConfigure> &pcmConfig, const std::string &inputMetaName)
{
    FALSE_RETURN_MSG(meta != nullptr, "meta is nullptr");
    FALSE_RETURN_MSG(pcmConfig != nullptr, "pcmConfig is nullptr");
    meta->GetData(Tag::AUDIO_SAMPLE_RATE, pcmConfig->sampleRate);
    meta->GetData(Tag::AUDIO_CHANNEL_COUNT, pcmConfig->channelCount);
    meta->GetData(Tag::MEDIA_BITRATE, pcmConfig->bitrate);
    meta->GetData(Tag::MIME_TYPE, pcmConfig->mimeType);
    meta->GetData(Tag::AUDIO_CHANNEL_LAYOUT, pcmConfig->channelLayout);
    meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, pcmConfig->sampleFormat);
    MEDIA_LOG_I("%{public}s mime=%{public}s, sampleRate=%{public}d",
        inputMetaName.c_str(), pcmConfig->mimeType.c_str(), pcmConfig->sampleRate);
    MEDIA_LOG_I("%{public}s audio format: channels=%{public}d, sampleRate=%{public}d",
        inputMetaName.c_str(), pcmConfig->channelCount, pcmConfig->sampleRate);
    MEDIA_LOG_I("%{public}s audio format: channellayout=%{public}u, sampleFormat=%{public}d", inputMetaName.c_str(),
        static_cast<uint32_t>(pcmConfig->channelLayout), static_cast<int32_t>(pcmConfig->sampleFormat));
}

void AudioEncoderFilter::AdjustAudioConfigure(std::shared_ptr<AudioConfigure> &adjustedConfig,
    std::shared_ptr<AudioConfigure> &inputPCM, std::shared_ptr<AudioConfigure> &outputPCM)
{
    // 获取格式配置
    const AudioFormatConfig* formatConfig = GetFormatConfig(outputPCM->mimeType);

    // 调整后的参数
    adjustedConfig->sampleRate = inputPCM->sampleRate;
    adjustedConfig->bitrate = outputPCM->bitrate;
    adjustedConfig->channelCount = inputPCM->channelCount;

    if (formatConfig != nullptr) {
        // 1. 码率优先判断：若码率未设置，使用输入码率
        if (adjustedConfig->bitrate <= 0 && inputPCM->bitrate > 0) {
            adjustedConfig->bitrate = inputPCM->bitrate;
        }

        // 2. 查找最接近的离散码率值
        int64_t originalBitrate = adjustedConfig->bitrate;
        adjustedConfig->bitrate = FindClosestBitrate(formatConfig, adjustedConfig->bitrate);
        if (originalBitrate != adjustedConfig->bitrate) {
            MEDIA_LOG_I("Bitrate adjusted from %{public}lld to %{public}lld (closest standard bitrate)",
                        static_cast<long long>(originalBitrate), static_cast<long long>(adjustedConfig->bitrate));
        }

        // 3. 根据码率查找支持的采样率范围
        std::vector<int32_t> supportedSampleRates = FindSupportedSampleRates(formatConfig, adjustedConfig->bitrate);

        // 4. 在支持的采样率中查找最接近输入采样率的值
        if (!supportedSampleRates.empty()) {
            adjustedConfig->sampleRate = FindClosestSampleRate(supportedSampleRates, inputPCM->sampleRate);
        } else {
            // 如果没有采样率限制，使用输出配置值或输入值
            adjustedConfig->sampleRate = (outputPCM->sampleRate > 0) ? outputPCM->sampleRate : inputPCM->sampleRate;
        }

        // 5. 根据格式调整声道数
        adjustedConfig->channelCount = AdjustChannelCount(formatConfig, inputPCM->channelCount);

        MEDIA_LOG_I("FormatConfig: mime=%{public}s, adjustedConfig->sampleRate=%{public}d",
            formatConfig->mimeType.c_str(), adjustedConfig->sampleRate);
        MEDIA_LOG_I("FormatConfig: adjustedChannels=%{public}d, adjustedConfig->bitrate=%{public}lld",
            adjustedConfig->channelCount, static_cast<long long>(adjustedConfig->bitrate));
    } else {
        // 未知格式，使用默认处理
        MEDIA_LOG_I("Unknown format: %{public}s, using default values", outputPCM->mimeType.c_str());
        adjustedConfig->sampleRate = (outputPCM->sampleRate > 0) ? outputPCM->sampleRate : inputPCM->sampleRate;
        adjustedConfig->bitrate = (outputPCM->bitrate > 0) ? outputPCM->bitrate : inputPCM->bitrate;
        adjustedConfig->channelCount = 1;  // 默认单声道
    }
}

Status AudioEncoderFilter::UpdateParameterToConfigure(const std::shared_ptr<Meta> &meta)
{
    Plugins::AudioSampleFormat dstSampleFormat = Plugins::AudioSampleFormat::INVALID_WIDTH;
    Plugins::AudioSampleFormat oriSampleFormat = Plugins::AudioSampleFormat::INVALID_WIDTH;
    std::string outputMime;
    if (meta != nullptr && configureParameter_ != nullptr && meta->GetData(Tag::AUDIO_SAMPLE_FORMAT, dstSampleFormat)) {
        configureParameter_->GetData(Tag::AUDIO_SAMPLE_FORMAT, oriSampleFormat);
        MEDIA_LOG_I("Configure, sampleFormat: " PUBLIC_LOG_D32 " -> " PUBLIC_LOG_D32,
            static_cast<int32_t>(oriSampleFormat), static_cast<int32_t>(dstSampleFormat));
        configureParameter_->SetData(Tag::AUDIO_SAMPLE_FORMAT, dstSampleFormat);
        configureParameter_->GetData(Tag::MIME_TYPE, outputMime);
    }
    shouldJudgeChange = isPureAudioMode_.load() || (!isPureAudioMode_.load() && outputMime == "audio/mpeg");
    if (isTranscoderMode_ && shouldJudgeChange.load()) {
        int32_t outputbuffersize = 0;
        if (meta->GetData(Tag::AUDIO_MAX_OUTPUT_SIZE, outputbuffersize)) {
            MEDIA_LOG_I("AUDIO_MAX_OUTPUT_SIZE is outputbuffersize: " PUBLIC_LOG_D32,
                static_cast<int32_t>(outputbuffersize));
        } else {
            MEDIA_LOG_I("not get AUDIO_MAX_OUTPUT_SIZE");
        }

        // 获取输入PCM参数.
        std::shared_ptr<AudioConfigure> inputPCM = std::make_shared<AudioConfigure>();
        GetAudioPCMConfigure(transcoderMeta_, inputPCM, "UpdateParameterToConfigure input");

        // 获取输出配置参数
        std::shared_ptr<AudioConfigure> outputPCM = std::make_shared<AudioConfigure>();
        GetAudioPCMConfigure(configureParameter_, outputPCM, "UpdateParameterToConfigure output");

        std::shared_ptr<AudioConfigure> adjustedPCM = std::make_shared<AudioConfigure>();
        AdjustAudioConfigure(adjustedPCM, inputPCM, outputPCM);
        // 设置调整后的参数
        FALSE_RETURN_V(configureParameter_ != nullptr, Status::ERROR_NULL_POINTER);
        configureParameter_->Set<Tag::AUDIO_SAMPLE_RATE>(adjustedPCM->sampleRate);
        configureParameter_->Set<Tag::MEDIA_BITRATE>(adjustedPCM->bitrate);
        configureParameter_->Set<Tag::AUDIO_CHANNEL_COUNT>(adjustedPCM->channelCount);
        configureParameter_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(
            adjustedPCM->channelCount == 1 ? AudioChannelLayout::MONO : AudioChannelLayout::STEREO);

        int32_t buffersize = 4096 * adjustedPCM->channelCount * SAMPLE_FORMAT_16;
        if (outputbuffersize != 0) {
            configureParameter_->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(outputbuffersize);
        } else {
            configureParameter_->Set<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(buffersize);
            MEDIA_LOG_I("AUDIO_MAX_INPUT_BUFFER_SIZE buffersize is: " PUBLIC_LOG_D32,
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
        std::shared_ptr<AudioConfigure> inputAudio = std::make_shared<AudioConfigure>();
        GetAudioPCMConfigure(transcoderMeta_, inputAudio, "TranscoderMeta");

        // 获取输出音频格式信息
        std::shared_ptr<AudioConfigure> outputAudio = std::make_shared<AudioConfigure>();
        GetAudioPCMConfigure(meta, outputAudio, "meta");

        // Check if channel conversion is needed
#ifdef SUPPORT_AUDIO_CONVERT
        needChannelConvert_.store((inputAudio->channelCount != outputAudio->channelCount) ||
            (inputAudio->sampleRate != outputAudio->sampleRate));

        if (needChannelConvert_.load() && shouldJudgeChange.load()) {
            // 初始化转换器输入格式（使用 AudioStandard 命名空间的类型）
            converterInputFormat_.rate = static_cast<AudioStandard::AudioSamplingRate>(inputAudio->sampleRate);
            converterInputFormat_.audioChannelInfo.channelLayout =
                static_cast<AudioStandard::AudioChannelLayout>(inputAudio->channelLayout);
            converterInputFormat_.format =
                static_cast<AudioStandard::AudioSampleFormat>(inputAudio->sampleFormat);
            converterInputFormat_.audioChannelInfo.numChannels = static_cast<uint32_t>(inputAudio->channelCount);

            // 初始化转换器输出格式（单声道）
            converterOutputFormat_.rate = static_cast<AudioStandard::AudioSamplingRate>(outputAudio->sampleRate);
            converterOutputFormat_.audioChannelInfo.channelLayout =
                static_cast<AudioStandard::AudioChannelLayout>(outputAudio->channelLayout);
            converterOutputFormat_.format =
                static_cast<AudioStandard::AudioSampleFormat>(outputAudio->sampleFormat);
            converterOutputFormat_.audioChannelInfo.numChannels = static_cast<uint32_t>(outputAudio->channelCount);

            // 创建转换器
            channelConverter_ = AudioFormatConverter::Create(
                converterInputFormat_, converterOutputFormat_);
            if (channelConverter_ == nullptr) {
                MEDIA_LOG_E("Failed to create channel converter");
                needChannelConvert_.store(false);
            } else {
                needChannelConvert_.store(true);
                MEDIA_LOG_I("Created channel converter: %{public}d channels -> 1 channel",
                            inputAudio->channelCount);
            }
        }
#endif

        if (needChannelConvert_.load() && shouldJudgeChange.load()) {
            AddInternalBufferQueue();
            FALSE_RETURN_MSG(intermediateQueueProducer_ != nullptr, "intermediateBufferQueue_ is nullptr");
            onLinkedResultCallback_->OnLinkedResult(intermediateQueueProducer_, transcoderMeta_);
        } else {
            onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), transcoderMeta_);
            MEDIA_LOG_I("not pura audio mode");
        }

        MEDIA_LOG_I("OnLinkedResult completed, processing thread started");
        return;
    }
    onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), meta);
}

void AudioEncoderFilter::AddInternalBufferQueue()
{
    // 创建中间缓存队列
    constexpr int32_t INTERMEDIATE_BUFFER_NUM = 8;
    intermediateBufferQueue_ = AVBufferQueue::Create(INTERMEDIATE_BUFFER_NUM,
        MemoryType::SHARED_MEMORY, "AudioEncoderIntermediateQueue");
    if (intermediateBufferQueue_ == nullptr) {
        MEDIA_LOG_E("Create internal bufferQueue failed!");
        FALSE_RETURN(eventReceiver_ != nullptr);
        eventReceiver_->OnEvent({"audio_encoder_filter", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_NO_MEMORY)});
        return;
    }

    intermediateQueueProducer_ = intermediateBufferQueue_->GetProducer();
    intermediateQueueConsumer_ = intermediateBufferQueue_->GetConsumer();
    if (intermediateQueueConsumer_ == nullptr || intermediateQueueProducer_ == nullptr) {
        MEDIA_LOG_E("GetProducer or GetConsumer returned nullptr");
        intermediateBufferQueue_ = nullptr;
        intermediateQueueProducer_ = nullptr;
        intermediateQueueConsumer_ = nullptr;
        FALSE_RETURN(eventReceiver_ != nullptr);
        eventReceiver_->OnEvent({"audio_encoder_filter", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_NO_MEMORY)});
        return;
    }

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

Status AudioEncoderFilter::AcquireIntermediateBuffer(std::shared_ptr<AVBuffer>& buffer)
{
    FALSE_RETURN_V(intermediateQueueConsumer_ != nullptr, Status::ERROR_NULL_POINTER);
    
    buffer = nullptr;
    Status ret = intermediateQueueConsumer_->AcquireBuffer(buffer);
    if (ret != Status::OK || buffer == nullptr) {
        MEDIA_LOG_E("AcquireBuffer failed, ret: %{public}d", ret);
        buffer = nullptr;
        return Status::ERROR_UNKNOWN;
    }
    
    MEDIA_LOG_D("Processing buffer, before flag: 0x%{public}u", buffer->flag_);
    return Status::OK;
}

Status AudioEncoderFilter::RequestCodecBuffer(std::shared_ptr<AVBuffer>& buffer, const AVBufferConfig& config)
{
    buffer = nullptr;
    
    std::lock_guard<std::mutex> lock(producerMutex_);
    
    FALSE_RETURN_V_MSG_E(mediaCodecInputProducer != nullptr,
        Status::ERROR_NULL_POINTER, "mediaCodecInputProducer is nullptr");
    
    Status ret = mediaCodecInputProducer->RequestBuffer(buffer, config, AVTRANSCODER_MAX_BUFFER_WAIT);
    if (ret != Status::OK || buffer == nullptr || buffer->memory_ == nullptr) {
        MEDIA_LOG_E("RequestBuffer from mediaCodec failed, ret: %{public}d", ret);
        buffer = nullptr;
        return Status::ERROR_UNKNOWN;
    }
    
    return Status::OK;
}

void AudioEncoderFilter::CopyBufferMetadata(const std::shared_ptr<AVBuffer>& srcBuffer,
    const std::shared_ptr<AVBuffer>& dstBuffer)
{
    FALSE_RETURN_NOLOG(srcBuffer != nullptr && dstBuffer != nullptr);
    
    dstBuffer->flag_ = srcBuffer->flag_;
    dstBuffer->pts_ = srcBuffer->pts_;
    dstBuffer->dts_ = srcBuffer->dts_;
    dstBuffer->duration_ = srcBuffer->duration_;
}

Status AudioEncoderFilter::ConvertAudioData(const std::shared_ptr<AVBuffer>& srcBuffer,
    const std::shared_ptr<AVBuffer>& dstBuffer)
{
#ifdef SUPPORT_AUDIO_CONVERT
    FALSE_RETURN_V_MSG_E(srcBuffer != nullptr && dstBuffer != nullptr,
        Status::ERROR_INVALID_PARAMETER, "CopyAudioData: invalid buffer parameter");
    FALSE_RETURN_V_MSG_E(srcBuffer->memory_ != nullptr && dstBuffer->memory_ != nullptr,
        Status::ERROR_INVALID_PARAMETER, "CopyAudioData: invalid memory");
    FALSE_RETURN_V_MSG_E(channelConverter_ != nullptr, Status::ERROR_NULL_POINTER, "channelConverter_ is nullptr");

    MEDIA_LOG_I("is converting");
    uint32_t outputSize = static_cast<uint32_t>(dstBuffer->memory_->GetCapacity());
    auto callback = std::make_shared<ConverterInputCallback>(srcBuffer);
    channelConverter_->SetInputCallback(callback);

    uint32_t actualOutputSize = 0;
    int32_t convertRet = channelConverter_->Process(
        dstBuffer->memory_->GetAddr(), outputSize, &actualOutputSize);
    FALSE_RETURN_V_MSG_E(convertRet == 0,
        Status::ERROR_UNKNOWN, "Channel conversion failed, ret: %{public}d", convertRet);

    // converter成功但无输出：数据在内部累积中，等后续帧凑齐再输出
    if (actualOutputSize == 0) {
        dstBuffer->memory_->SetSize(0);
        return Status::OK;
    }

    actualOutputSize = actualOutputSize > static_cast<uint32_t>(dstBuffer->memory_->GetCapacity()) ?
        static_cast<uint32_t>(dstBuffer->memory_->GetCapacity()) : actualOutputSize;
    dstBuffer->memory_->SetSize(actualOutputSize);

    CopyBufferMetadata(srcBuffer, dstBuffer);
    return Status::OK;
#else
    MEDIA_LOG_E("ConvertAudioData: audio convert not supported");
    return Status::ERROR_UNKNOWN;
#endif
}

Status AudioEncoderFilter::CopyAudioData(const std::shared_ptr<AVBuffer>& srcBuffer,
    const std::shared_ptr<AVBuffer>& dstBuffer)
{
    MEDIA_LOG_I("no need convert");
    FALSE_RETURN_V_MSG_E(srcBuffer != nullptr && dstBuffer != nullptr,
        Status::ERROR_INVALID_PARAMETER, "CopyAudioData: invalid buffer parameter");
    FALSE_RETURN_V_MSG_E(srcBuffer->memory_ != nullptr && dstBuffer->memory_ != nullptr,
        Status::ERROR_INVALID_PARAMETER, "CopyAudioData: invalid memory");

    CopyBufferMetadata(srcBuffer, dstBuffer);

    auto srcData = srcBuffer->memory_->GetAddr();
    auto dstData = dstBuffer->memory_->GetAddr();
    auto srcSize = srcBuffer->memory_->GetSize();
    auto dstCapacity = dstBuffer->memory_->GetCapacity();

    FALSE_RETURN_V_MSG_E(srcData != nullptr && dstData != nullptr,
        Status::ERROR_INVALID_PARAMETER, "CopyAudioData: invalid data address");
    FALSE_RETURN_V_MSG_E(srcSize > 0 && dstCapacity >= srcSize, Status::ERROR_INVALID_PARAMETER,
        "Empty source buffer or insufficient space, dstCapacity=%{public}u, srcSize=%{public}u", dstCapacity, srcSize);

    errno_t ret = memcpy_s(dstData, dstCapacity, srcData, srcSize);
    FALSE_RETURN_V_MSG_E(ret == 0, Status::ERROR_INVALID_PARAMETER,
        "memcpy_s failed, ret: %{public}d", ret);
    dstBuffer->memory_->SetSize(srcSize);
    
    return Status::OK;
}

Status AudioEncoderFilter::PushBufferToCodec(const std::shared_ptr<AVBuffer>& buffer, bool available)
{
    std::lock_guard<std::mutex> lock(producerMutex_);

    FALSE_RETURN_V_MSG_E(mediaCodecInputProducer != nullptr,
        Status::ERROR_NULL_POINTER, "mediaCodecInputProducer is nullptr when pushing buffer");

    Status ret = mediaCodecInputProducer->PushBuffer(buffer, available);
    FALSE_RETURN_V_MSG_E(ret == Status::OK,
        ret, "PushBuffer to mediaCodec failed, ret: %{public}d", ret);

    return ret;
}

void AudioEncoderFilter::ProcessIntermediateBuffer()
{
    FALSE_RETURN_MSG(intermediateQueueConsumer_ != nullptr, "intermediateQueueConsumer_ is nullptr");

    Status ret;
    do {
        MediaAVCodec::AVCodecTrace trace("AudioEncoderFilter::ProcessIntermediateBuffer");
        std::shared_ptr<AVBuffer> intermediateBuffer;
        ret = AcquireIntermediateBuffer(intermediateBuffer);
        FALSE_BREAK_NOLOG(ret == Status::OK);

        std::shared_ptr<AVBuffer> codecBuffer;
        AVBufferConfig config;
        ret = RequestCodecBuffer(codecBuffer, config);
        if (ret != Status::OK) {
            intermediateQueueConsumer_->ReleaseBuffer(intermediateBuffer);
            continue;
        }

        Status processRet;
#ifdef SUPPORT_AUDIO_CONVERT
        if (needChannelConvert_.load() && channelConverter_ != nullptr) {
            processRet = ConvertAudioData(intermediateBuffer, codecBuffer);
        } else {
            processRet = CopyAudioData(intermediateBuffer, codecBuffer);
        }
#else
        processRet = CopyAudioData(intermediateBuffer, codecBuffer);
#endif

        intermediateQueueConsumer_->ReleaseBuffer(intermediateBuffer);
        if (processRet != Status::OK) {
            PushBufferToCodec(codecBuffer, false);
            continue;
        }

        // 有实际输出数据时推送到编码器
        if (codecBuffer->memory_ != nullptr && codecBuffer->memory_->GetSize() > 0) {
            PushBufferToCodec(codecBuffer);
        } else if (intermediateBuffer->flag_ & BUFFER_IS_EOS) {
            // converter无输出但源帧携带EOS：必须将EOS信号送达编码器
            CopyBufferMetadata(intermediateBuffer, codecBuffer);
            codecBuffer->memory_->SetSize(0);
            PushBufferToCodec(codecBuffer);
        } else {
            // converter内部累积中（actualOutputSize=0），无数据可推送
            // 必须将codecBuffer归还池（available=false：归还freeBufferList，不通知消费者）
            // 否则buffer池会被耗尽导致RequestBuffer失败
            PushBufferToCodec(codecBuffer, false);
        }
    } while (ret == Status::OK);
}

void AudioEncoderFilter::StopProcessingThread()
{
    MediaAVCodec::AVCodecTrace trace("AudioEncoderFilter::StopProcessingThread");
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
    if (totalSize < 0 || readOffset_ >= static_cast<size_t>(totalSize)) {
        if (status != nullptr) {
            // 仅当源buffer携带EOS时才返回DATA_FINISHED，表示整个流结束
            // 否则返回NO_AVAILABLE_DATA，让converter内部缓存数据等后续帧凑齐
            if (sourceBuffer_->flag_ & BUFFER_IS_EOS) {
                *status = InputDataStatus::DATA_FINISHED;
            } else {
                *status = InputDataStatus::NO_AVAILABLE_DATA;
            }
        }
        return 0;
    }

    if (data != nullptr) {
        *data = static_cast<const uint8_t*>(sourceBuffer_->memory_->GetAddr()) + readOffset_;
    }

    size_t remainingSize = static_cast<size_t>(totalSize) - readOffset_;
    readOffset_ = static_cast<size_t>(totalSize); // 下次调用返回 DATA_FINISHED

    if (status != nullptr) {
        // 根据 sourceBuffer 的 flag 判断是否为 EOS
        // 如果是 EOS，返回 DATA_FINISHED；否则返回 HAVE_DATA
        if (sourceBuffer_->flag_ & BUFFER_IS_EOS) {
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