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

#include <fstream>
#include <numeric>
#include "gtest/gtest.h"
#include "syspara/parameters.h"
#include "tester_common.h"
#include "hcodec_log.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace testing::ext;

class HEncoderBufferUnitTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        ASSERT_TRUE(CreateFakeYuv(INPUT_FILE_PATH, W, H, 4));  // 4 frames
    }

    void SetUp() override
    {
        OHOS::system::SetParameter("hcodec.debug", "1");
    }

    void TearDown() override
    {
        OHOS::system::SetParameter("hcodec.debug", "0");
        OHOS::system::SetParameter("hcodec.dump", "0");
    }

    static bool CreateFakeYuv(const string& dstPath, uint32_t w, uint32_t h, uint32_t frameCnt)
    {
        ofstream ofs(dstPath, ios::binary);
        if (!ofs.is_open()) {
            TLOGE("cannot create %s", dstPath.c_str());
            return false;
        }
        vector<char> line(w);
        std::iota(line.begin(), line.end(), 0);
        for (uint32_t n = 0; n < frameCnt; n++) {
            for (uint32_t i = 0; i < h; i++) {
                ofs.write(line.data(), line.size());
            }
            for (uint32_t i = 0; i < h / 2; i++) { // 2: yuvsp ratio
                ofs.write(line.data(), line.size());
            }
        }
        return true;
    }

protected:
    static constexpr uint32_t W = 176;
    static constexpr uint32_t H = 144;
    static constexpr char INPUT_FILE_PATH[] = "/data/test/media/176x144.yuv";
};

HWTEST_F(HEncoderBufferUnitTest, encode_surface_264_codecbase, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0000");
    CommandOpt opt = {
        .apiType = ApiType::TEST_CODEC_BASE,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = false,
        .isHighPerfMode = 1,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST_F(HEncoderBufferUnitTest, encode_surface_264_codecbase_perframe, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "1100");
    CommandOpt opt = {
        .apiType = ApiType::TEST_CODEC_BASE,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = false,
        .perFrameParamsMap = {{2, PerFrameParams{
            .requestIdr = true,
            .qpRange = QPRange{13, 42},
        }}},
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST_F(HEncoderBufferUnitTest, encode_surface_265_capi_new, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0100");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_NEW,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H265,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = false,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST_F(HEncoderBufferUnitTest, encode_surface_265_capi_old, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "1000");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_OLD,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H265,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = false,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST_F(HEncoderBufferUnitTest, encode_buffer_264_codecbase, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "1100");
    CommandOpt opt = {
        .apiType = ApiType::TEST_CODEC_BASE,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST_F(HEncoderBufferUnitTest, encode_buffer_264_codecbase_setparam, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "1100");
    CommandOpt opt = {
        .apiType = ApiType::TEST_CODEC_BASE,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H264,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = true,
        .setParameterParamsMap = {{2, SetParameterParams{
            .requestIdr = true,
            .qpRange = QPRange{13, 42},
        }}},
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST_F(HEncoderBufferUnitTest, encode_buffer_265_capi_new, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0011");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_NEW,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H265,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST_F(HEncoderBufferUnitTest, encode_buffer_265_capi_old, TestSize.Level1)
{
    OHOS::system::SetParameter("hcodec.dump", "0010");
    CommandOpt opt = {
        .apiType = ApiType::TEST_C_API_OLD,
        .isEncoder = true,
        .inputFile = INPUT_FILE_PATH,
        .dispW = W,
        .dispH = H,
        .protocol = H265,
        .pixFmt = VideoPixelFormat::NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}
} // namespace OHOS::MediaAVCodec