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

#include "videoenc_capi_mock.h"
#include <iostream>
#include "avbuffer_capi_mock.h"
#include "avformat_capi_mock.h"
#include "avmemory_capi_mock.h"
#include "surface_capi_mock.h"

using namespace std;

namespace OHOS {
namespace MediaAVCodec {
std::mutex VideoEncCapiMock::mutex_;
std::map<OH_AVCodec *, std::shared_ptr<AVCodecCallbackMock>> VideoEncCapiMock::mockCbMap_;
std::map<OH_AVCodec *, std::shared_ptr<MediaCodecCallbackMock>> VideoEncCapiMock::mockCbExtMap_;
std::map<OH_AVCodec *, std::shared_ptr<MediaCodecParameterCallbackMock>> VideoEncCapiMock::mockParamCbMap_;
void VideoEncCapiMock::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoEncCapiMock::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoEncCapiMock::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = data == nullptr ? nullptr : std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNeedInputData(index, memMock);
    }
}

void VideoEncCapiMock::OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                       void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = data == nullptr ? nullptr : std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNewOutputData(index, memMock, *attr);
    }
}

void VideoEncCapiMock::OnErrorExt(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoEncCapiMock::OnStreamChangedExt(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoEncCapiMock::OnNeedInputDataExt(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock = data == nullptr ? nullptr : std::make_shared<AVBufferCapiMock>(data);
        mockCb->OnNeedInputData(index, bufMock);
    }
}

void VideoEncCapiMock::OnNewOutputDataExt(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecCallbackMock> mockCb = GetCallbackExt(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVBufferMock> bufMock = data == nullptr ? nullptr : std::make_shared<AVBufferCapiMock>(data);
        mockCb->OnNewOutputData(index, bufMock);
    }
}

void VideoEncCapiMock::OnNeedInputParameter(OH_AVCodec *codec, uint32_t index, OH_AVFormat *parameter, void *userData)
{
    (void)userData;
    std::shared_ptr<MediaCodecParameterCallbackMock> mockCb = GetParamCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(parameter);
        mockCb->OnInputParameterAvailable(index, formatMock);
    }
}

std::shared_ptr<AVCodecCallbackMock> VideoEncCapiMock::GetCallback(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

std::shared_ptr<MediaCodecCallbackMock> VideoEncCapiMock::GetCallbackExt(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbExtMap_.find(codec) != mockCbExtMap_.end()) {
        return mockCbExtMap_.at(codec);
    }
    return nullptr;
}

std::shared_ptr<MediaCodecParameterCallbackMock> VideoEncCapiMock::GetParamCallback(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockParamCbMap_.find(codec) != mockParamCbMap_.end()) {
        return mockParamCbMap_.at(codec);
    }
    return nullptr;
}

void VideoEncCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void VideoEncCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<MediaCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbExtMap_[codec] = cb;
}

void VideoEncCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<MediaCodecParameterCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockParamCbMap_[codec] = cb;
}

void VideoEncCapiMock::DelCallback(OH_AVCodec *codec)
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

int32_t VideoEncCapiMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr && codec_ != nullptr) {
        SetCallback(codec_, cb);
        struct OH_AVCodecAsyncCallback callback;
        callback.onError = VideoEncCapiMock::OnError;
        callback.onStreamChanged = VideoEncCapiMock::OnStreamChanged;
        callback.onNeedInputData = VideoEncCapiMock::OnNeedInputData;
        callback.onNeedOutputData = VideoEncCapiMock::OnNewOutputData;
        return OH_VideoEncoder_SetCallback(codec_, callback, NULL);
    }
    return AV_ERR_UNKNOWN;
};

int32_t VideoEncCapiMock::SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb)
{
    SetCallback(codec_, cb);
    struct OH_AVCodecCallback callback;
    callback.onError = VideoEncCapiMock::OnErrorExt;
    callback.onStreamChanged = VideoEncCapiMock::OnStreamChangedExt;
    callback.onNeedInputBuffer = VideoEncCapiMock::OnNeedInputDataExt;
    callback.onNewOutputBuffer = VideoEncCapiMock::OnNewOutputDataExt;
    return OH_VideoEncoder_RegisterCallback(codec_, callback, NULL);
}

int32_t VideoEncCapiMock::SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb)
{
    SetCallback(codec_, cb);
    OH_VideoEncoder_OnNeedInputParameter callback = VideoEncCapiMock::OnNeedInputParameter;
    return OH_VideoEncoder_RegisterParameterCallback(codec_, callback, NULL);
}

int32_t VideoEncCapiMock::Configure(std::shared_ptr<FormatMock> format)
{
    auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
    OH_AVFormat *avFormat = formatMock->GetFormat();
    if (avFormat != nullptr) {
        return OH_VideoEncoder_Configure(codec_, avFormat);
    } else {
        cout << "VideoEncCapiMock::Configure: avFormat is null" << endl;
        return AV_ERR_UNKNOWN;
    }
}

int32_t VideoEncCapiMock::Start()
{
    return OH_VideoEncoder_Start(codec_);
}

int32_t VideoEncCapiMock::Stop()
{
    return OH_VideoEncoder_Stop(codec_);
}

int32_t VideoEncCapiMock::Flush()
{
    return OH_VideoEncoder_Flush(codec_);
}

int32_t VideoEncCapiMock::Reset()
{
    return OH_VideoEncoder_Reset(codec_);
}

int32_t VideoEncCapiMock::Release()
{
    DelCallback(codec_);
    int32_t ret = OH_VideoEncoder_Destroy(codec_);
    codec_ = nullptr;
    return ret;
}

std::shared_ptr<FormatMock> VideoEncCapiMock::GetOutputDescription()
{
    OH_AVFormat *format = OH_VideoEncoder_GetOutputDescription(codec_);
    return std::make_shared<AVFormatCapiMock>(format);
}

std::shared_ptr<FormatMock> VideoEncCapiMock::GetInputDescription()
{
    OH_AVFormat *format = OH_VideoEncoder_GetInputDescription(codec_);
    return std::make_shared<AVFormatCapiMock>(format);
}

int32_t VideoEncCapiMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
    return OH_VideoEncoder_SetParameter(codec_, formatMock->GetFormat());
}

int32_t VideoEncCapiMock::FreeOutputData(uint32_t index)
{
    return OH_VideoEncoder_FreeOutputData(codec_, index);
}

int32_t VideoEncCapiMock::NotifyEos()
{
    return OH_VideoEncoder_NotifyEndOfStream(codec_);
}

int32_t VideoEncCapiMock::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    return OH_VideoEncoder_PushInputData(codec_, index, attr);
}

int32_t VideoEncCapiMock::PushInputBuffer(uint32_t index)
{
    return OH_VideoEncoder_PushInputBuffer(codec_, index);
}

int32_t VideoEncCapiMock::PushInputParameter(uint32_t index)
{
    return OH_VideoEncoder_PushInputParameter(codec_, index);
}

int32_t VideoEncCapiMock::FreeOutputBuffer(uint32_t index)
{
    return OH_VideoEncoder_FreeOutputBuffer(codec_, index);
}

std::shared_ptr<SurfaceMock> VideoEncCapiMock::CreateInputSurface()
{
    OHNativeWindow *window;
    int32_t ret = OH_VideoEncoder_GetSurface(codec_, &window);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, nullptr, "VideoEncCapiMock CreateInputSurface failed!");
    if (window != nullptr) {
        return std::make_shared<SurfaceCapiMock>(window);
    }
    return nullptr;
}

bool VideoEncCapiMock::IsValid()
{
    bool isValid = false;
    int32_t ret = OH_VideoEncoder_IsValid(codec_, &isValid);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, false, "VideoEncCapiMock IsValid failed!");
    return isValid;
}
} // namespace MediaAVCodec
} // namespace OHOS