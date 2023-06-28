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

#ifndef VCODEC_MOCK_H
#define VCODEC_MOCK_H

#include <string>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "avformat_mock.h"
#include "buffer_queue_producer.h"
#include "common_mock.h"
#include "consumer_surface.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "nocopyable.h"
#include "surface/window.h"
#include "unittest_log.h"


namespace OHOS {
namespace MediaAVCodec {
class VideoDecMock : public NoCopyable {
public:
    virtual ~VideoDecMock() = default;
    virtual int32_t Release() = 0;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
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
    virtual bool IsValid() = 0;
};

class VideoEncMock : public NoCopyable {
public:
    virtual ~VideoEncMock() = default;
    virtual int32_t Release() = 0;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
    virtual int32_t Configure(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual std::shared_ptr<FormatMock> GetOutputDescription() = 0;
    virtual int32_t SetParameter(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t FreeOutputData(uint32_t index) = 0;
    virtual int32_t NotifyEos() = 0;
    virtual int32_t PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr) = 0;
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
std::string VENC_AVC_NAME = "OMX.hisi.video.encoder.avc";

const std::string VDEC_AVC_NAME = std::string(AVCodecCodecName::VIDEO_DECODER_AVC_NAME);
const std::map<std::string, std::string> VDEC_SOURCE = {{VDEC_AVC_NAME, "/data/test/media/avc_320_240_10s.dat"}};
constexpr uint32_t DEFAULT_BITRATE = 12000;

constexpr uint32_t SAMPLE_TIMEOUT = 10;
constexpr uint32_t DEFAULT_WIDTH = 320;
constexpr uint32_t DEFAULT_HEIGHT = 240;
constexpr uint32_t DEFAULT_FRAME_RATE = 20;
constexpr bool NEED_DUMP = true;

constexpr uint32_t EOS_COUNT = 100;
} // namespace VCodecTestParam
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif // VCODEC_MOCK_H