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

#ifndef VIDEO_DEC_INNER_MOCK_H
#define VIDEO_DEC_INNER_MOCK_H

#include "vcodec_mock.h"
#include "avcodec_common.h"
#include "avcodec_video_decoder.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoDecInnerMock : public VideoDecMock {
public:
    explicit VideoDecInnerMock(std::shared_ptr<AVCodecVideoDecoder> videoDec) : videoDec_(videoDec) {}
    ~VideoDecInnerMock() = default;
    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) override;
    int32_t SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb) override;
    int32_t SetOutputSurface(std::shared_ptr<SurfaceMock> surface) override;
    int32_t Configure(std::shared_ptr<FormatMock> format) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    std::shared_ptr<FormatMock> GetOutputDescription() override;
    int32_t SetParameter(std::shared_ptr<FormatMock> format) override;
    int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr) override;
    int32_t RenderOutputData(uint32_t index) override;
    int32_t FreeOutputData(uint32_t index) override;
    int32_t PushInputBuffer(uint32_t index) override;
    int32_t RenderOutputBuffer(uint32_t index) override;
    int32_t FreeOutputBuffer(uint32_t index) override;
    bool IsValid() override;

private:
    std::shared_ptr<AVCodecVideoDecoder> videoDec_ = nullptr;
};

class VideoDecCallbackMock : public AVCodecCallback {
public:
    explicit VideoDecCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb);
    ~VideoDecCallbackMock() = default;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;

private:
    std::shared_ptr<AVCodecCallbackMock> mockCb_ = nullptr;
};

class VideoDecCallbackExtMock : public MediaCodecCallback {
public:
    explicit VideoDecCallbackExtMock(std::shared_ptr<MediaCodecCallbackMock> cb);
    ~VideoDecCallbackExtMock() = default;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;

private:
    std::shared_ptr<MediaCodecCallbackMock> mockCb_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VIDEO_DEC_INNER_MOCK_H