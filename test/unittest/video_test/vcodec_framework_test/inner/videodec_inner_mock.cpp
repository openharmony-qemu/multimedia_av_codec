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

#include "videodec_inner_mock.h"
#include "avbuffer_inner_mock.h"
#include "avformat_inner_mock.h"
#include "avmemory_inner_mock.h"
#include "surface_inner_mock.h"

namespace OHOS {
namespace MediaAVCodec {
VideoDecCallbackExtMock::VideoDecCallbackExtMock(std::shared_ptr<MediaCodecCallbackMock> cb) : mockCb_(cb) {}

void VideoDecCallbackExtMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void VideoDecCallbackExtMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatInnerMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void VideoDecCallbackExtMock::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (mockCb_ != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock =
            buffer == nullptr ? nullptr : std::make_shared<AVBufferInnerMock>(buffer);
        mockCb_->OnNeedInputData(index, bufMock);
    }
}

void VideoDecCallbackExtMock::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (mockCb_ != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock =
            buffer == nullptr ? nullptr : std::make_shared<AVBufferInnerMock>(buffer);
        return mockCb_->OnNewOutputData(index, bufMock);
    }
}

VideoDecCallbackMock::VideoDecCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb) : mockCb_(cb) {}

void VideoDecCallbackMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void VideoDecCallbackMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatInnerMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void VideoDecCallbackMock::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    if (mockCb_ != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock =
            buffer == nullptr ? nullptr : std::make_shared<AVMemoryInnerMock>(buffer);
        mockCb_->OnNeedInputData(index, memMock);
    }
}

void VideoDecCallbackMock::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
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

int32_t VideoDecInnerMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoDec_ != nullptr) {
        std::shared_ptr<VideoDecCallbackMock> callback =
            cb == nullptr ? nullptr : std::make_shared<VideoDecCallbackMock>(cb);
        return videoDec_->SetCallback(callback);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    if (videoDec_ != nullptr) {
        std::shared_ptr<VideoDecCallbackExtMock> callback =
            cb == nullptr ? nullptr : std::make_shared<VideoDecCallbackExtMock>(cb);
        return videoDec_->SetCallback(callback);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::SetOutputSurface(std::shared_ptr<SurfaceMock> surface)
{
    if (surface != nullptr) {
        auto decSurface = std::static_pointer_cast<SurfaceInnerMock>(surface);
        sptr<Surface> nativeSurface = decSurface->GetSurface();
        if (videoDec_ != nullptr && nativeSurface != nullptr) {
            return videoDec_->SetOutputSurface(nativeSurface);
        }
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatInnerMock>(format);
        return videoDec_->Configure(fmt->GetFormat());
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::Start()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Start();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::Stop()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Stop();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::Flush()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Flush();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::Reset()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Reset();
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::Release()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Release();
    }
    return AV_ERR_UNKNOWN;
}

std::shared_ptr<FormatMock> VideoDecInnerMock::GetOutputDescription()
{
    if (videoDec_ != nullptr) {
        Format format;
        (void)videoDec_->GetOutputFormat(format);
        return std::make_shared<AVFormatInnerMock>(format);
    }
    return nullptr;
}

int32_t VideoDecInnerMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatInnerMock>(format);
        return videoDec_->SetParameter(fmt->GetFormat());
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoDec_ != nullptr) {
        AVCodecBufferInfo info;
        info.presentationTimeUs = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        AVCodecBufferFlag flags = static_cast<AVCodecBufferFlag>(attr.flags);
        return videoDec_->QueueInputBuffer(index, info, flags);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::RenderOutputData(uint32_t index)
{
    if (videoDec_ != nullptr) {
        return videoDec_->ReleaseOutputBuffer(index, true);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::FreeOutputData(uint32_t index)
{
    if (videoDec_ != nullptr) {
        return videoDec_->ReleaseOutputBuffer(index, false);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::PushInputBuffer(uint32_t index)
{
    if (videoDec_ != nullptr) {
        return videoDec_->QueueInputBuffer(index);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::RenderOutputBuffer(uint32_t index)
{
    if (videoDec_ != nullptr) {
        return videoDec_->ReleaseOutputBuffer(index, true);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecInnerMock::FreeOutputBuffer(uint32_t index)
{
    if (videoDec_ != nullptr) {
        return videoDec_->ReleaseOutputBuffer(index, false);
    }
    return AV_ERR_UNKNOWN;
}

bool VideoDecInnerMock::IsValid()
{
    return true;
}
} // namespace MediaAVCodec
} // namespace OHOS