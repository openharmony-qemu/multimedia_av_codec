/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef VCODEC_MOCK_H
#define VCODEC_MOCK_H

#include <string>
#include <map>
#include "avbuffer_mock.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "avformat_mock.h"
#include "codeclist_mock.h"
#include "common_mock.h"
#include "iconsumer_surface.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "nocopyable.h"
#include "surface/window.h"
#include "unittest_log.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecCallbackMock : public NoCopyable {
public:
    virtual ~AVCodecCallbackMock() = default;
    virtual void OnError(int32_t errorCode) = 0;
    virtual void OnStreamChanged(std::shared_ptr<FormatMock> format) = 0;
    virtual void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) = 0;
    virtual void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr) = 0;
};

class MediaCodecCallbackMock : public NoCopyable {
public:
    virtual ~MediaCodecCallbackMock() = default;
    virtual void OnError(int32_t errorCode) = 0;
    virtual void OnStreamChanged(std::shared_ptr<FormatMock> format) = 0;
    virtual void OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data) = 0;
    virtual void OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data) = 0;
};

class MediaCodecParameterCallbackMock : public NoCopyable {
public:
    virtual ~MediaCodecParameterCallbackMock() = default;
    virtual void OnInputParameterAvailable(uint32_t index, std::shared_ptr<FormatMock> parameter) = 0;
};

class VideoDecMock : public NoCopyable {
public:
    virtual ~VideoDecMock() = default;
    virtual int32_t Release() = 0;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
    virtual int32_t SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb) = 0;
    virtual int32_t SetOutputSurface(std::shared_ptr<SurfaceMock> surface) = 0;
    virtual int32_t Configure(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual std::shared_ptr<FormatMock> GetOutputDescription() = 0;
    virtual int32_t SetParameter(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr) = 0;
    virtual int32_t RenderOutputData(uint32_t index) = 0;
    virtual int32_t FreeOutputData(uint32_t index) = 0;
    virtual int32_t PushInputBuffer(uint32_t index) = 0;
    virtual int32_t RenderOutputBuffer(uint32_t index) = 0;
    virtual int32_t FreeOutputBuffer(uint32_t index) = 0;
    virtual bool IsValid() = 0;
};

class VideoEncMock : public NoCopyable {
public:
    virtual ~VideoEncMock() = default;
    virtual int32_t Release() = 0;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
    virtual int32_t SetCallback(std::shared_ptr<MediaCodecCallbackMock> cb) = 0;
    virtual int32_t SetCallback(std::shared_ptr<MediaCodecParameterCallbackMock> cb) = 0;
    virtual int32_t Configure(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual std::shared_ptr<FormatMock> GetOutputDescription() = 0;
    virtual std::shared_ptr<FormatMock> GetInputDescription() = 0;
    virtual int32_t SetParameter(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t FreeOutputData(uint32_t index) = 0;
    virtual int32_t NotifyEos() = 0;
    virtual int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr) = 0;
    virtual int32_t PushInputBuffer(uint32_t index) = 0;
    virtual int32_t PushInputParameter(uint32_t index) = 0;
    virtual int32_t FreeOutputBuffer(uint32_t index) = 0;
    virtual std::shared_ptr<SurfaceMock> CreateInputSurface() = 0;
    virtual bool IsValid() = 0;
};

class __attribute__((visibility("default"))) VCodecMockFactory {
public:
    static std::shared_ptr<VideoDecMock> CreateVideoDecMockByMime(const std::string &mime);
    static std::shared_ptr<VideoDecMock> CreateVideoDecMockByName(const std::string &name);
    static std::shared_ptr<VideoEncMock> CreateVideoEncMockByMime(const std::string &mime);
    static std::shared_ptr<VideoEncMock> CreateVideoEncMockByName(const std::string &name);

private:
    VCodecMockFactory() = delete;
    ~VCodecMockFactory() = delete;
};

namespace VCodecTestParam {
enum VCodecTestCode : int32_t { HW_AVC, HW_HEVC, HW_HDR, SW_AVC };
const std::map<int32_t, std::string> decSourcePathMap_ = {{HW_AVC, "/data/test/media/720_1280_25_avcc.h264"},
                                                          {HW_HEVC, "/data/test/media/720_1280_25_avcc.h265"},
                                                          {HW_HDR, "/data/test/media/720_1280_25_avcc.hdr.h265"},
                                                          {SW_AVC, "/data/test/media/720_1280_25_avcc.h264"}};
constexpr uint32_t DEFAULT_BITRATE = 12000;

constexpr uint32_t DEFAULT_WIDTH = 720;
constexpr uint32_t DEFAULT_HEIGHT = 1280;
constexpr uint32_t DEFAULT_FRAME_RATE = 20;

constexpr uint32_t DEFAULT_WIDTH_VENC = 1280;
constexpr uint32_t DEFAULT_HEIGHT_VENC = 720;

constexpr uint32_t SAMPLE_TIMEOUT = 100;
} // namespace VCodecTestParam
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VCODEC_MOCK_H