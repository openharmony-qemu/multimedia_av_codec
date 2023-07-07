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

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace testing::ext;

HWTEST(HEncoderBufferUnitTest, encode_surface_264, TestSize.Level1)
{
    CommandOpt opt = {
        .isEncoder = true,
        .inputFile = "/data/test/media/1280_720_nv.yuv",
        .dispW = 1280,
        .dispH = 720,
        .protocol = H264,
        .pixFmt = NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = false,
        .numIdrFrame = 2
    };
    std::shared_ptr<TesterCommon> tester = TesterCommon::Create(opt);
    ASSERT_TRUE(tester != nullptr);
    bool ret = tester->Run();
    ASSERT_TRUE(ret);
}

HWTEST(HEncoderBufferUnitTest, encode_buffer_265, TestSize.Level1)
{
    CommandOpt opt = {
        .isEncoder = true,
        .inputFile = "/data/test/media/1280_720_nv.yuv",
        .dispW = 1280,
        .dispH = 720,
        .protocol = H265,
        .pixFmt = NV12,
        .frameRate = 30,
        .timeout = 100,
        .isBufferMode = true,
    };
    std::shared_ptr<TesterCommon> tester = TesterCommon::Create(opt);
    ASSERT_TRUE(tester != nullptr);
    bool ret = tester->Run();
    ASSERT_TRUE(ret);
}
} // namespace OHOS::MediaAVCodec