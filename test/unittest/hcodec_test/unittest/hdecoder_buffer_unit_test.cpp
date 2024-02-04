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

#include "gtest/gtest.h"
#include "tester_common.h"
#include "syspara/parameters.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace testing::ext;
static constexpr int32_t TIME_OUT = 300;

HWTEST(HDecoderBufferUnitTest, decode_surface_264_codecbase, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0000");
    CommandOpt opt = {
        .apiType = ApiType::TEST_CODEC_BASE,
        .isEncoder = false,
        .inputFile = "/data/test/media/out_320_240_10s.h264",
        .dispW = 320,
        .dispH = 240,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = TIME_OUT,
        .isBufferMode = false,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HDecoderBufferUnitTest, decode_surface_264_capi_new, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0001");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_NEW,
        .isEncoder = false,
        .inputFile = "/data/test/media/out_320_240_10s.h264",
        .dispW = 320,
        .dispH = 240,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = TIME_OUT,
        .isBufferMode = false,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HDecoderBufferUnitTest, decode_surface_264_capi_old, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0010");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_OLD,
        .isEncoder = false,
        .inputFile = "/data/test/media/out_320_240_10s.h264",
        .dispW = 320,
        .dispH = 240,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = TIME_OUT,
        .isBufferMode = false,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HDecoderBufferUnitTest, decode_buffer_264_codecbase, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0011");
    CommandOpt opt = {
        .apiType = ApiType::TEST_CODEC_BASE,
        .isEncoder = false,
        .inputFile = "/data/test/media/out_320_240_10s.h264",
        .dispW = 320,
        .dispH = 240,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = TIME_OUT,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HDecoderBufferUnitTest, decode_buffer_264_capi_new, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "1100");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_NEW,
        .isEncoder = false,
        .inputFile = "/data/test/media/out_320_240_10s.h264",
        .dispW = 320,
        .dispH = 240,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = TIME_OUT,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HDecoderBufferUnitTest, decode_buffer_264_capi_old, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0100");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_OLD,
        .isEncoder = false,
        .inputFile = "/data/test/media/out_320_240_10s.h264",
        .dispW = 320,
        .dispH = 240,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = TIME_OUT,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}
} // OHOS::MediaAVCodec