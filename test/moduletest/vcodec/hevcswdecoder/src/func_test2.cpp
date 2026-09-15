/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ONLY_FOR_FLAGSHIP_CHIP
#define ONLY_FOR_FLAGSHIP_CHIP 0
#endif

#include <string>
#include "gtest/gtest.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_sample.h"
#include "native_avformat.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HevcSwdecFunc2NdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

protected:
    const char *ginpDir1080 = "/data/test/media/1920_1080_30.h265";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap_hevc = nullptr;
static string g_codecNameHevc = "";
} // namespace

void HevcSwdecFunc2NdkTest::SetUpTestCase()
{
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, SOFTWARE);
    g_codecNameHevc = OH_AVCapability_GetName(cap_hevc);
    cout << "g_codecNameHevc: " << g_codecNameHevc << endl;
}

void HevcSwdecFunc2NdkTest::TearDownTestCase() {}
void HevcSwdecFunc2NdkTest::SetUp() {}
void HevcSwdecFunc2NdkTest::TearDown() {}

struct DmabufSuffixItem {
    std::string input;
    std::string expected;
};

const std::vector<DmabufSuffixItem> &GetDmabufSuffixCat1()
{
    static const std::vector<DmabufSuffixItem> cat1 = {
        { "!", "!" },
        { "abc123", "abc123" },
        { "a_b-c!a1bcX2YZ3!@#$%^&*", "a_b-c!a1bcX2YZ3!@#$%^&*" },
    };
    return cat1;
}

const std::vector<std::string> &GetDmabufSuffixCat2()
{
    static const std::vector<std::string> cat2 = {
        "测试 / cafe /  ", "12 3a", "abc\n \t \r \0", "0123456789", "\n \t",
    };
    return cat2;
}

static void ResetDmabufTestState(const std::shared_ptr<VDecAPI11Sample> &vDecSample)
{
    vDecSample->Flush_buffer();
    vDecSample->errCount = 0;
    vDecSample->frameCount_ = 0;
    vDecSample->outCount = 0;
    vDecSample->dmabufSuffixChecked = false;
}

static int RunDmabufSuffixCat1(const std::shared_ptr<VDecAPI11Sample> &vDecSample)
{
    const auto &cat1 = GetDmabufSuffixCat1();
    vDecSample->needCheckDmabufNameSuffix = true;
    for (size_t i = 0; i < cat1.size(); i++) {
        if (i > 0) {
            if (vDecSample->Reset() != AV_ERR_OK) { return -1; }
            ResetDmabufTestState(vDecSample);
        }
        vDecSample->dmabufNameSuffix = cat1[i].input;
        vDecSample->expectedDmabufSuffix = cat1[i].expected;
        if (vDecSample->ConfigureVideoDecoder() != AV_ERR_OK) { return -1; }
        if (vDecSample->StartVideoDecoder() != AV_ERR_OK) { return -1; }
        vDecSample->WaitForEOS();
        if (vDecSample->errCount != AV_ERR_OK) { return -1; }
    }
    return 0;
}

static int RunDmabufSuffixCat2(const std::shared_ptr<VDecAPI11Sample> &vDecSample)
{
    const auto &cat2 = GetDmabufSuffixCat2();
    for (size_t i = 0; i < cat2.size(); i++) {
        if (vDecSample->Reset() != AV_ERR_OK) { return -1; }
        ResetDmabufTestState(vDecSample);
        vDecSample->dmabufNameSuffix = cat2[i];
        int32_t ret = vDecSample->ConfigureVideoDecoder();
        if (ret != AV_ERR_INVALID_VAL) {
            return -1;
        }
    }
    return 0;
}

namespace {
/**
 * @tc.number    : VIDEO_DECODE_S265_DMABUF_SUFFIX_0010
 * @tc.name      : 265软解config设置dma标签, 有效输入及无效输入
 * @tc.desc      : function test
 */
HWTEST_F(HevcSwdecFunc2NdkTest, VIDEO_DECODE_S265_DMABUF_SUFFIX_0010, TestSize.Level2)
{
    if (cap_hevc != nullptr && ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = ginpDir1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->enableDmabufNameSuffix = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHevc));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(0, RunDmabufSuffixCat1(vDecSample));
        ASSERT_EQ(0, RunDmabufSuffixCat2(vDecSample));
    }
}
} // namespace