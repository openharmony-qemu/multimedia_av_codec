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

#include "videoenc_inner_mock.h"
#include "avbuffer_inner_mock.h"
#include "avformat_inner_mock.h"
#include "avmemory_inner_mock.h"
#include "surface_inner_mock.h"

namespace OHOS {
namespace MediaAVCodec {
VideoEncCallbackExtMock::VideoEncCallbackExtMock(std::shared_ptr<MediaCodecCallbackMock> cb) : mockCb_(cb) {}

void VideoEncCallbackExtMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void VideoEncCallbackExtMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatInnerMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void VideoEncCallbackExtMock::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (mockCb_ != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock =
            buffer == nullptr ? nullptr : std::make_shared<AVBufferInnerMock>(buffer);
        mockCb_->OnNeedInputData(index, bufMock);
    }
}

void VideoEncCallbackExtMock::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (mockCb_ != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock =
            buffer == nullptr ? nullptr : std::make_shared<AVBufferInnerMock>(buffer);
        return mockCb_->OnNewOutputData(index, bufMock);
    }
}

VideoEncCallbackMock::VideoEncCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb) : mockCb_(cb) {}

void VideoEncCallbackMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void VideoEncCallbackMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatInnerMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void VideoEncCallbackMock::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    if (mockCb_ != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock =
            buffer == nullptr ? nullptr : std::make_shared<AVMemoryInnerMock>(buffer);
        mockCb_->OnNeedInputData(index, memMock);
    }
}

void VideoEncCallbackMock::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                   std::shared_ptr<AVSharedMemory> buffer)
{
    if (mockCb_ != nullptr) {
        struct OH_AVCodecBufferAttr bufferInfo;
        bufferInfo.pts = info.presentationTimeUs;
        bufferInfo.size = info.size;
        bufferInfo.offset = info.offset;
        bufferInfo.flags = flag;
        std::shared_ptr<AVMemoryMock> memMock =
            buffer == nullptr ? nullptr : std::make_shared<AVMemoryInnerMock>(buffer);
        return mockCb_->OnNewOutputData(index, memMock, bufferInfo);
    }
}

VideoEncParamCallbackMock::VideoEncParamCallbackMock(std::shared_ptr<MediaCodecParameterCallbackMock> cb) : mockCb_(cb)
{
}

void VideoEncParamCallbackMock::OnInputParameterAvailable(uint32_t index, std::shared_ptr<Format> parameter)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatInnerMock>();
        formatMock->format_ = std::move(*parameter);
        mockCb_->OnInputParameterAvailable(index, formatMock);
    }
}

int32_t VideoEncInnerMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoEnc_ != nullptr) {
        std::shared_ptr<VideoEncCallbackMock> callback =
            cb == nullptr ? nullptr : std::make_shared<VideoEncCallbackMock>(cb);
        return videoEnc_->SetCallback(callback);
    }

    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    if (videoEnc_ != nullptr) {
        std::shared_ptr<VideoEncCallbackExtMock> callback =
            cb == nullptr ? nullptr : std::make_shared<VideoEncCallbackExtMock>(cb);
        return videoEnc_->SetCallback(callback);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb)
{
    if (videoEnc_ != nullptr) {
        std::shared_ptr<VideoEncParamCallbackMock> callback =
            cb == nullptr ? nullptr : std::make_shared<VideoEncParamCallbackMock>(cb);
        return videoEnc_->SetCallback(callback);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatInnerMock>(format);
        return videoEnc_->Configure(fmt->GetFormat());
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::Start()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Start();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::Stop()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Stop();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::Flush()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Flush();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::Reset()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Reset();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::Release()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Release();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::NotifyEos()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->NotifyEos();
    }
    return AV_ERR_UNKNOWN;
}

std::shared_ptr<FormatMock> VideoEncInnerMock::GetOutputDescription()
{
    if (videoEnc_ != nullptr) {
        Format format;
        (void)videoEnc_->GetOutputFormat(format);
        return std::make_shared<AVFormatInnerMock>(format);
    }
    return nullptr;
}

std::shared_ptr<FormatMock> VideoEncInnerMock::GetInputDescription()
{
    if (videoEnc_ != nullptr) {
        Format format;
        (void)videoEnc_->GetInputFormat(format);
        return std::make_shared<AVFormatInnerMock>(format);
    }
    return nullptr;
}

int32_t VideoEncInnerMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatInnerMock>(format);
        return videoEnc_->SetParameter(fmt->GetFormat());
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::FreeOutputData(uint32_t index)
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->ReleaseOutputBuffer(index);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoEnc_ != nullptr) {
        AVCodecBufferInfo info;
        info.presentationTimeUs = attr.pts;
        info.offset = attr.offset;
        info.size = attr.size;
        AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(attr.flags);
        return videoEnc_->QueueInputBuffer(index, info, flag);
    }
    return AV_ERR_UNKNOWN;
}

std::shared_ptr<SurfaceMock> VideoEncInnerMock::CreateInputSurface()
{
    if (videoEnc_ != nullptr) {
        sptr<Surface> surface = videoEnc_->CreateInputSurface();
        if (surface != nullptr) {
            return std::make_shared<SurfaceInnerMock>(surface);
        }
    }
    return nullptr;
}

int32_t VideoEncInnerMock::PushInputBuffer(uint32_t index)
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->QueueInputBuffer(index);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::PushInputParameter(uint32_t index)
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->QueueInputParameter(index);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoEncInnerMock::FreeOutputBuffer(uint32_t index)
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->ReleaseOutputBuffer(index);
    }
    return AV_ERR_UNKNOWN;
}

bool VideoEncInnerMock::IsValid()
{
    return true;
}
} // namespace MediaAVCodec
} // namespace OHOS