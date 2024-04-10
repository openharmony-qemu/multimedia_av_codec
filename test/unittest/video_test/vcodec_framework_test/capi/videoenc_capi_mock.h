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

#ifndef VIDEOENC_CAPI_MOCK_H
#define VIDEOENC_CAPI_MOCK_H

#include <mutex>
#include <map>
#include "vcodec_mock.h"
#include "native_avcodec_videoencoder.h"


namespace OHOS {
namespace MediaAVCodec {
class VideoEncCapiMock : public VideoEncMock {
public:
    explicit VideoEncCapiMock(OH_AVCodec *codec) : codec_(codec) {}
    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) override;
    int32_t SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb) override;
    int32_t SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb) override;
    int32_t Configure(std::shared_ptr<FormatMock> format) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    std::shared_ptr<FormatMock> GetOutputDescription() override;
    std::shared_ptr<FormatMock> GetInputDescription() override;
    int32_t SetParameter(std::shared_ptr<FormatMock> format) override;
    int32_t FreeOutputData(uint32_t index) override;
    int32_t NotifyEos() override;
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr) override;
    int32_t PushInputBuffer(uint32_t index) override;
    int32_t PushInputParameter(uint32_t index) override;
    int32_t FreeOutputBuffer(uint32_t index) override;
    std::shared_ptr<SurfaceMock> CreateInputSurface() override;
    bool IsValid() override;

private:
    static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
    static void OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
        OH_AVCodecBufferAttr *attr, void *userData);

    static void OnErrorExt(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChangedExt(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputDataExt(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData);
    static void OnNewOutputDataExt(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData);
    static void OnNeedInputParameter(OH_AVCodec *codec, uint32_t index, OH_AVFormat *parameter, void *userData);

    static void SetCallback(OH_AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb);
    static void SetCallback(OH_AVCodec *codec, std::shared_ptr<MediaCodecCallbackMock> cb);
    static void SetCallback(OH_AVCodec *codec, std::shared_ptr<MediaCodecParameterCallbackMock> cb);
    static void DelCallback(OH_AVCodec *codec);
    static std::shared_ptr<AVCodecCallbackMock> GetCallback(OH_AVCodec *codec);
    static std::shared_ptr<MediaCodecCallbackMock> GetCallbackExt(OH_AVCodec *codec);
    static std::shared_ptr<MediaCodecParameterCallbackMock> GetParamCallback(OH_AVCodec *codec);

    static std::mutex mutex_;
    static std::map<OH_AVCodec *, std::shared_ptr<AVCodecCallbackMock>> mockCbMap_;
    static std::map<OH_AVCodec *, std::shared_ptr<MediaCodecCallbackMock>> mockCbExtMap_;
    static std::map<OH_AVCodec *, std::shared_ptr<MediaCodecParameterCallbackMock>> mockParamCbMap_;
    OH_AVCodec *codec_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VIDEOENC_CAPI_MOCK_H