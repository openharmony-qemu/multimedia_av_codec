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
#include "ffmpeg_amrnb_decoder_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_common.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"
#include "common/log.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegAmrnbDecoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 1;
constexpr int32_t SUPPORT_SAMPLE_RATE = 1;
constexpr int32_t MIN_OUTBUF_SIZE = 1280;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 150;
constexpr int32_t SAMPLE_RATE_PICK[SUPPORT_SAMPLE_RATE] = {8000};
constexpr int32_t US_PER_SECOND = 1000000;

constexpr int32_t frame_sizes_nb[16] = {
    13, 14, 16, 18, 20, 21, 27, 32, 6, // 0 - 8, valid frame size
    0, 0, 0, 0, 0, 0, 0 // 9 -15, invalid
};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegAmrnbDecoderPlugin::FFmpegAmrnbDecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels(0), sampleRate(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegAmrnbDecoderPlugin::~FFmpegAmrnbDecoderPlugin()
{
    if (basePlugin != nullptr) {
        basePlugin->Release();
        basePlugin.reset();
    }
}

Status FFmpegAmrnbDecoderPlugin::Init()
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::Prepare()
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    eosFlushed_ = false;
    return basePlugin->Reset();
}

Status FFmpegAmrnbDecoderPlugin::Start()
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    return Status::OK;
}

static std::string AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

Status FFmpegAmrnbDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    AVPacket *pkt = av_packet_alloc();
    CHECK_AND_RETURN_RET_LOG(pkt != nullptr, Status::ERROR_NO_MEMORY, "allocate packet error.");
    avPacket_ = std::shared_ptr<AVPacket>(pkt, [](AVPacket *p) {
        av_packet_free(&p);
    });
    Status ret = basePlugin->AllocateContext("amrnb");
    Status checkresult = CheckInit(parameter);
    CHECK_AND_RETURN_RET_LOG(checkresult == Status::OK, checkresult, "check init error.");
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "amrnb init error.");
    ret = basePlugin->InitContext(parameter);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "amrnb init context error.");
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    format->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat_);
    Status retContext = basePlugin->OpenContext();
    avCodecContext_ = basePlugin->GetCodecContext();
    return retContext;
}

Status FFmpegAmrnbDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB);
    parameter = format;
    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::ExtractOneAmrnbFrame()
{
    CHECK_AND_RETURN_RET_LOG(inputBuf_.size() >= 1, Status::ERROR_NOT_ENOUGH_DATA, "inputBuf_ is invalid");

    uint8_t toc = inputBuf_[0];
    int32_t mode = (toc >> 3) & 0x0F;
    int32_t frameSize = frame_sizes_nb[mode];
    AVCODEC_LOGD("mode=%{public}d, frameSize=%{public}d, inputBuf_.size=%{public}zu",
                 mode, frameSize, inputBuf_.size());
    if (frameSize <= 0) {
        AVCODEC_LOGE("frameSize is invalid");
        inputBuf_.clear();
        return Status::ERROR_INVALID_DATA;
    }
    CHECK_AND_RETURN_RET_LOG(static_cast<size_t>(frameSize) <= inputBuf_.size(), Status::ERROR_NOT_ENOUGH_DATA,
                             "buffer size is not enough");

    if (av_new_packet(avPacket_.get(), frameSize) < 0) {
        AVCODEC_LOGE("allocate new packet failed");
        avPacket_.reset();
        return Status::ERROR_NO_MEMORY;
    }

    memcpy_s(avPacket_->data, avPacket_->size, inputBuf_.data(), frameSize);
    avPacket_->pts = nextPts_;
    AVCODEC_LOGD("avPacket_->pts: %{public}" PRId64, avPacket_->pts);
    int64_t durationPerSample_ = US_PER_SECOND / avCodecContext_->sample_rate;
    nextPts_ += nbSamplesPerFrame_ * durationPerSample_;

    inputBuf_.erase(inputBuf_.begin(), inputBuf_.begin() + frameSize);

    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    auto memory = inputBuffer->memory_;
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, Status::ERROR_INVALID_DATA, "input memory null");
    int32_t sourceSize = memory->GetSize();
    const uint8_t *srcBuffer = memory->GetAddr();
    bool isEos = inputBuffer->flag_ & MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    bool checkSize = (sourceSize <= 0) && !isEos;
    CHECK_AND_RETURN_RET_LOG(!checkSize, Status::ERROR_INVALID_DATA, "input size is %{public}d, but flag is not eos",
        sourceSize);
    if (!isEos) {
        inputBuf_.insert(inputBuf_.end(), srcBuffer, srcBuffer + sourceSize);
    }
    nextPts_ = inputBuffer->pts_;
    dataCallback_->OnInputBufferDone(inputBuffer);
    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    bool isEos = outputBuffer->flag_ & MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;

    if (!isEos) {
        nbSamplesPerFrame_ = outputBuffer->memory_->GetSize() / (sampleFormat_ + 1);
        AVCODEC_LOGD("nbSamplesPerFrame_:%{public}d", nbSamplesPerFrame_);
        Status extractSucc = ExtractOneAmrnbFrame();
        CHECK_AND_RETURN_RET_LOG(extractSucc == Status::OK, Status::ERROR_NOT_ENOUGH_DATA,
            "amrnb split frame failed, remaining size: %{public}zu", inputBuf_.size());
    } else {
        avPacket_->size = 0;
        avPacket_->data = nullptr;
        avPacket_->pts = outputBuffer->pts_;
    }

    auto retSend = avcodec_send_packet(avCodecContext_.get(), avPacket_.get());
    av_packet_unref(avPacket_.get());

    if (retSend == AVERROR(EAGAIN)) {
        return Status::ERROR_NOT_ENOUGH_DATA;
    } else if (retSend == AVERROR_EOF) {
        dataCallback_->OnInputBufferDone(outputBuffer);
        AVCODEC_LOGE("send eos frame, msg:%{public}s", AVStrError(retSend).data());
        return Status::END_OF_STREAM;
    } else if (retSend < 0) {
        AVCODEC_LOGE("send packet failed: %{public}s", AVStrError(retSend).c_str());
        return Status::ERROR_INVALID_DATA;
    }

    auto retRecv = basePlugin->ReceiveBuffer(outputBuffer);
    if (retRecv == Status::OK) {
        CHECK_AND_RETURN_RET_LOG(inputBuf_.empty(), Status::ERROR_AGAIN, "input buffer empty");
        return Status::OK;
    } else if (retRecv == Status::ERROR_NOT_ENOUGH_DATA) {
        AVCODEC_LOGE("receive frame not enough, ret: %{public}d", static_cast<int32_t>(retRecv));
        return Status::ERROR_NOT_ENOUGH_DATA;
    } else if (retRecv == Status::ERROR_AGAIN) {
        AVCODEC_LOGD("receive frame eagain, ret: %{public}d", static_cast<int32_t>(retRecv));
        return Status::ERROR_AGAIN;
    } else if (retRecv == Status::END_OF_STREAM) {
        AVCODEC_LOGI("receive eos frame, ret: %{public}d", static_cast<int32_t>(retRecv));
        return Status::END_OF_STREAM;
    } else {
        AVCODEC_LOGE("receive frame failed, ret: %{public}d", static_cast<int32_t>(retRecv));
        return Status::ERROR_UNKNOWN;
    }
}

Status FFmpegAmrnbDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegAmrnbDecoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    inputBuf_.clear();
    return basePlugin->Flush();
}

Status FFmpegAmrnbDecoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(amrMutex_);
    return basePlugin->Release();
}

Status FFmpegAmrnbDecoderPlugin::CheckInit(const std::shared_ptr<Meta> &format)
{
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channels);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    for (int32_t i = 0; i < SUPPORT_SAMPLE_RATE; i++) {
        if (sampleRate == SAMPLE_RATE_PICK[i]) {
            break;
        } else if (i == SUPPORT_SAMPLE_RATE - 1) {
            return Status::ERROR_INVALID_PARAMETER;
        }
    }

    CHECK_AND_RETURN_RET_LOG(basePlugin->CheckSampleFormat(format, channels),
        Status::ERROR_INVALID_PARAMETER, "CheckSampleFormat error");
    return Status::OK;
}

int32_t FFmpegAmrnbDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t FFmpegAmrnbDecoderPlugin::GetOutputBufferSize()
{
    return MIN_OUTBUF_SIZE;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
