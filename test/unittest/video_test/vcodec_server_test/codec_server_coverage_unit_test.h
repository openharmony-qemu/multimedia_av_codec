/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file expect in compliance with the License.
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
#ifndef CODEC_SERVER_COVERAGE_UNIT_TEST
#define CODEC_SERVER_COVERAGE_UNIT_TEST

#include "gtest/gtest.h"
#include "av_common.h"
#include "codecbase.h"
#include "codec_server.h"

namespace OHOS {
namespace MediaAVCodec {
constexpr uint32_t DEFAULT_WIDTH = 4096;
constexpr uint32_t DEFAULT_HEIGHT = 4096;
class CodecServerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(void);
    void TearDown(void);

    void CreateCodecByMime();
    void CreateFCodecByName();
    void CreateFCodecByMime();
    void CreateHCodecByName();
    void CreateHCodecByMime();
    std::shared_ptr<CodecBaseMock> codecBaseMock_ = nullptr;
    std::shared_ptr<CodecServer> server_ = nullptr;
private:
    Format validFormat_;
};

class CodecParamCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(void){};
    void TearDown(void){};
};

inline void CodecServerUnitTest::SetUp(void)
{
    codecBaseMock_ = std::make_shared<CodecBaseMock>();
    CodecBase::RegisterMock(codecBaseMock_);

    server_ = std::static_pointer_cast<CodecServer>(CodecServer::Create());
    EXPECT_NE(server_, nullptr);

    validFormat_.PutIntValue(Tag::VIDEO_WIDTH, DEFAULT_WIDTH);
    validFormat_.PutIntValue(Tag::VIDEO_HEIGHT, DEFAULT_HEIGHT);
    validFormat_.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    auto pixelFormats = capability_->GetVideoSupportedPixelFormats();
    if (std::find(pixelFormats.begin(), pixelFormats.end(), VideoPixelFormat::YUVI420) != pixelFormats.end()) {
        GTEST_SKIP() << "Unsupport pixel format of YUVI420";
    }
}

inline void CodecServerUnitTest::TearDown(void)
{
    server_->Release();
    server_ = nullptr;
    codecBaseMock_ = nullptr;
    validFormat_ = Format();
}
} // name space MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVER_COVERAGE_UNIT_TEST