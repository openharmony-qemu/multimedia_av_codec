/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "audio_buffer_info.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioBufferInfo"};
constexpr uint8_t LOGD_FREQUENCY = 5;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Media;
AudioBufferInfo::AudioBufferInfo(const uint32_t bufferSize, const std::string_view &name, const uint32_t metaSize)
    : isHasMeta_(false),
      isEos_(false),
      isFirstFrame_(false),
      isUsing(false),
      status_(BufferStatus::IDLE),
      bufferSize_(bufferSize),
      metaSize_(metaSize),
      name_(name),
      buffer_(nullptr),
      metadata_(nullptr),
      flag_(AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE)
{
    if (metaSize_ > 0) {
        metadata_ =
            std::make_shared<AVSharedMemoryBase>(metaSize_, AVSharedMemory::Flags::FLAGS_READ_ONLY, std::string(name_));
        int32_t ret = metadata_->Init();
        if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
            AVCODEC_LOGE("Create metadata avsharedmemory failed, ret = %{public}d", ret);
        }
        isHasMeta_ = true;
    }

    buffer_ =
        std::make_shared<AVSharedMemoryBase>(bufferSize_, AVSharedMemory::Flags::FLAGS_READ_WRITE, std::string(name_));
    int32_t ret = buffer_->Init();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("Create buffer avsharedmemory failed, ret = %{public}d", ret);
    }
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "AudioBufferInfo constructor %{public}s buffer.", name_.data());
}

AudioBufferInfo::~AudioBufferInfo()
{
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "AudioBufferInfo destructor %{public}s buffer.", name_.data());
    isEos_ = false;
    status_ = BufferStatus::IDLE;

    if (buffer_) {
        buffer_.reset();
        buffer_ = nullptr;
    }

    if (metadata_) {
        metadata_.reset();
        metadata_ = nullptr;
    }
}

std::shared_ptr<AVSharedMemoryBase> AudioBufferInfo::GetBuffer() const noexcept
{
    return buffer_;
}

BufferStatus AudioBufferInfo::GetStatus() const noexcept
{
    return status_;
}

bool AudioBufferInfo::IsAvailable() const noexcept
{
    return status_ == BufferStatus::IDLE;
}

bool AudioBufferInfo::CheckIsEos() const noexcept
{
    return isEos_;
}

bool AudioBufferInfo::SetBufferOwned()
{
    status_ = BufferStatus::OWEN_BY_CLIENT;
    return true;
}

void AudioBufferInfo::SetEos(bool eos)
{
    isEos_ = eos;
    if (isEos_) {
        flag_ = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS;
    } else {
        flag_ = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    }
}

void AudioBufferInfo::SetBufferAttr(const AVCodecBufferInfo &attr)
{
    info_ = attr;
}

AVCodecBufferInfo AudioBufferInfo::GetBufferAttr() const noexcept
{
    return info_;
}

AVCodecBufferFlag AudioBufferInfo::GetFlag() const noexcept
{
    return flag_;
}

bool AudioBufferInfo::CheckIsFirstFrame() const noexcept
{
    return isFirstFrame_;
}

void AudioBufferInfo::SetFirstFrame() noexcept
{
    isFirstFrame_ = true;
}

bool AudioBufferInfo::CheckIsUsing() const noexcept
{
    return isUsing;
}

void AudioBufferInfo::SetUsing() noexcept
{
    isUsing = true;
}

std::shared_ptr<AVSharedMemoryBase> AudioBufferInfo::GetMetadata() const noexcept
{
    return metadata_;
}

bool AudioBufferInfo::IsHasMetaData() const noexcept
{
    return isHasMeta_;
}

uint32_t AudioBufferInfo::GetBufferSize() const noexcept
{
    return bufferSize_;
}

bool AudioBufferInfo::ResetBuffer()
{
    isEos_ = false;
    isFirstFrame_ = false;
    isUsing = false;
    status_ = BufferStatus::IDLE;
    flag_ = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    if (buffer_) {
        buffer_->ClearUsedSize();
    } else {
        return false;
    }
    return true;
}
} // namespace MediaAVCodec
} // namespace OHOS