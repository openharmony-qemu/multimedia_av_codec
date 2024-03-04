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

#include "videodec_capi_mock.h"
#include "avbuffer_capi_mock.h"
#include "avformat_capi_mock.h"
#include "avmemory_capi_mock.h"
#include "surface_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
std::mutex VideoDecCapiMock::mutex_;
std::map<OH_AVCodec *, std::shared_ptr<AVCodecCallbackMock>> VideoDecCapiMock::mockCbMap_;
std::map<OH_AVCodec *, std::shared_ptr<MediaCodecCallbackMock>> VideoDecCapiMock::mockCbExtMap_;
void VideoDecCapiMock::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoDecCapiMock::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoDecCapiMock::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = data == nullptr ? nullptr : std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNeedInputData(index, memMock);
    }
}

void VideoDecCapiMock::OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                       void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = data == nullptr ? nullptr : std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNewOutputData(index, memMock, *attr);
    }
}

void VideoDecCapiMock::OnErrorExt(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoDecCapiMock::OnStreamChangedExt(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoDecCapiMock::OnNeedInputDataExt(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock = data == nullptr ? nullptr : std::make_shared<AVBufferCapiMock>(data);
        mockCb->OnNeedInputData(index, bufMock);
    }
}

void VideoDecCapiMock::OnNewOutputDataExt(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock = data == nullptr ? nullptr : std::make_shared<AVBufferCapiMock>(data);
        mockCb->OnNewOutputData(index, bufMock);
    }
}

std::shared_ptr<AVCodecCallbackMock> VideoDecCapiMock::GetCallback(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

std::shared_ptr<MediaCodecCallbackMock> VideoDecCapiMock::GetCallbackExt(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbExtMap_.find(codec) != mockCbExtMap_.end()) {
        return mockCbExtMap_.at(codec);
    }
    return nullptr;
}

void VideoDecCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void VideoDecCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<MediaCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbExtMap_[codec] = cb;
}

void VideoDecCapiMock::DelCallback(OH_AVCodec *codec)
{
    auto it = mockCbMap_.find(codec);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
    auto itExt = mockCbExtMap_.find(codec);
    if (itExt != mockCbExtMap_.end()) {
        mockCbExtMap_.erase(itExt);
    }
}

int32_t VideoDecCapiMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    SetCallback(codec_, cb);
    struct OH_AVCodecAsyncCallback callback;
    callback.onError = VideoDecCapiMock::OnError;
    callback.onStreamChanged = VideoDecCapiMock::OnStreamChanged;
    callback.onNeedInputData = VideoDecCapiMock::OnNeedInputData;
    callback.onNeedOutputData = VideoDecCapiMock::OnNewOutputData;
    return OH_VideoDecoder_SetCallback(codec_, callback, NULL);
}

int32_t VideoDecCapiMock::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    SetCallback(codec_, cb);
    struct OH_AVCodecCallback callback;
    callback.onError = VideoDecCapiMock::OnErrorExt;
    callback.onStreamChanged = VideoDecCapiMock::OnStreamChangedExt;
    callback.onNeedInputBuffer = VideoDecCapiMock::OnNeedInputDataExt;
    callback.onNewOutputBuffer = VideoDecCapiMock::OnNewOutputDataExt;
    return OH_VideoDecoder_RegisterCallback(codec_, callback, NULL);
}

int32_t VideoDecCapiMock::SetOutputSurface(std::shared_ptr<SurfaceMock> surface)
{
    if (surface != nullptr) {
        auto surfaceMock = std::static_pointer_cast<SurfaceCapiMock>(surface);
        OHNativeWindow *nativeWindow = surfaceMock->GetSurface();
        if (nativeWindow != nullptr) {
            return OH_VideoDecoder_SetSurface(codec_, nativeWindow);
        }
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecCapiMock::Configure(std::shared_ptr<FormatMock> format)
{
    auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
    OH_AVFormat *avFormat = formatMock->GetFormat();
    if (avFormat != nullptr) {
        return OH_VideoDecoder_Configure(codec_, avFormat);
    }
    return AV_ERR_UNKNOWN;
}

int32_t VideoDecCapiMock::Start()
{
    return OH_VideoDecoder_Start(codec_);
}

int32_t VideoDecCapiMock::Stop()
{
    return OH_VideoDecoder_Stop(codec_);
}

int32_t VideoDecCapiMock::Flush()
{
    return OH_VideoDecoder_Flush(codec_);
}

int32_t VideoDecCapiMock::Reset()
{
    return OH_VideoDecoder_Reset(codec_);
}

int32_t VideoDecCapiMock::Release()
{
    DelCallback(codec_);
    int32_t ret = OH_VideoDecoder_Destroy(codec_);
    codec_ = nullptr;
    return ret;
}

std::shared_ptr<FormatMock> VideoDecCapiMock::GetOutputDescription()
{
    OH_AVFormat *format = OH_VideoDecoder_GetOutputDescription(codec_);
    return std::make_shared<AVFormatCapiMock>(format);
}

int32_t VideoDecCapiMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
    return OH_VideoDecoder_SetParameter(codec_, formatMock->GetFormat());
}

int32_t VideoDecCapiMock::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    return OH_VideoDecoder_PushInputData(codec_, index, attr);
}

int32_t VideoDecCapiMock::RenderOutputData(uint32_t index)
{
    return OH_VideoDecoder_RenderOutputData(codec_, index);
}

int32_t VideoDecCapiMock::FreeOutputData(uint32_t index)
{
    return OH_VideoDecoder_FreeOutputData(codec_, index);
}

int32_t VideoDecCapiMock::PushInputBuffer(uint32_t index)
{
    return OH_VideoDecoder_PushInputBuffer(codec_, index);
}

int32_t VideoDecCapiMock::RenderOutputBuffer(uint32_t index)
{
    return OH_VideoDecoder_RenderOutputBuffer(codec_, index);
}

int32_t VideoDecCapiMock::FreeOutputBuffer(uint32_t index)
{
    return OH_VideoDecoder_FreeOutputBuffer(codec_, index);
}

bool VideoDecCapiMock::IsValid()
{
    bool isValid = false;
    (void)OH_VideoDecoder_IsValid(codec_, &isValid);
    return isValid;
}
} // namespace MediaAVCodec
} // namespace OHOS