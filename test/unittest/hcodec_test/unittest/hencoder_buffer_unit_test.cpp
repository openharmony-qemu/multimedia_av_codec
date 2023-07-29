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
#include "tester_common.h"
#include "hcodec_log.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace testing::ext;

bool CreateFakeYuv(const string& dstPath, uint32_t w, uint32_t h, uint32_t frameCnt)
{
    ofstream ofs(dstPath, ios::binary);
    if (!ofs.is_open()) {
        LOGE("cannot create %s", dstPath.c_str());
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

HWTEST(HEncoderBufferUnitTest, encode_surface_264_codecbase, TestSize.Level1)
{
    uint32_t w = 176;
    uint32_t h = 144;
    string dst = "/data/test/media/176x144.yuv";
    if (!CreateFakeYuv(dst, w, h, 4)) {
        return;
    }
    CommandOpt opt = {
        .testCodecBaseApi = true,
        .isEncoder = true,
        .inputFile = dst,
        .dispW = w,
        .dispH = h,
        .protocol = H264,
        .pixFmt = NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = false,
        .numIdrFrame = 2
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HEncoderBufferUnitTest, encode_surface_264_capi, TestSize.Level1)
{
    uint32_t w = 176;
    uint32_t h = 144;
    string dst = "/data/test/media/176x144.yuv";
    if (!CreateFakeYuv(dst, w, h, 4)) {
        return;
    }
    CommandOpt opt = {
        .testCodecBaseApi = false,
        .isEncoder = true,
        .inputFile = dst,
        .dispW = w,
        .dispH = h,
        .protocol = H264,
        .pixFmt = NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = false,
        .numIdrFrame = 2
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HEncoderBufferUnitTest, encode_buffer_265_codecbase, TestSize.Level1)
{
    uint32_t w = 176;
    uint32_t h = 144;
    string dst = "/data/test/media/176x144.yuv";
    if (!CreateFakeYuv(dst, w, h, 4)) {
        return;
    }
    CommandOpt opt = {
        .testCodecBaseApi = true,
        .isEncoder = true,
        .inputFile = dst,
        .dispW = w,
        .dispH = h,
        .protocol = H265,
        .pixFmt = NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}

HWTEST(HEncoderBufferUnitTest, encode_buffer_265_capi, TestSize.Level1)
{
    uint32_t w = 176;
    uint32_t h = 144;
    string dst = "/data/test/media/176x144.yuv";
    if (!CreateFakeYuv(dst, w, h, 4)) {
        return;
    }
    CommandOpt opt = {
        .testCodecBaseApi = false,
        .isEncoder = true,
        .inputFile = dst,
        .dispW = w,
        .dispH = h,
        .protocol = H265,
        .pixFmt = NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = true,
    };
    bool ret = TesterCommon::Run(opt);
    ASSERT_TRUE(ret);
}
} // namespace OHOS::MediaAVCodec