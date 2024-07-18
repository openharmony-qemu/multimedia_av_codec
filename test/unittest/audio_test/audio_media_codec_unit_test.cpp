/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "media_codec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 48000;
constexpr uint32_t AUDIO_AAC_IS_ADTS = 1;
constexpr uint32_t DEFAULT_BUFFER_NUM = 1;
const std::string AAC_MIME_TYPE = "audio/mp4a-latm";
const std::string UNKNOW_MIME_TYPE = "audio/unknow";
const std::string AAC_DEC_CODEC_NAME = "OH.Media.Codec.Decoder.Audio.AAC";
}  // namespace

namespace OHOS {
namespace MediaAVCodec {

class AudioCodecCallback : public Media::AudioBaseCodecCallback {
public:
    AudioCodecCallback()
    {}
    virtual ~AudioCodecCallback()
    {}

    void OnError(Media::CodecErrorType errorType, int32_t errorCode) override;

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;
};

void AudioCodecCallback::OnError(Media::CodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
}

void AudioCodecCallback::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    (void)outputBuffer;
}

class TestCodecCallback : public Media::CodecCallback {
public:
    TestCodecCallback()
    {}
    virtual ~TestCodecCallback()
    {}

    void OnError(Media::CodecErrorType errorType, int32_t errorCode) override;

    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) override;
};

void TestCodecCallback::OnError(Media::CodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
}

void TestCodecCallback::OnOutputFormatChanged(const std::shared_ptr<Meta> &format)
{
    (void)format;
}

class MediaCodecUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void MediaCodecUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void MediaCodecUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void MediaCodecUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void MediaCodecUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

HWTEST_F(MediaCodecUnitTest, Test_Init_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, true));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Init_02, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, true));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Init_03, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Init_04, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_NE(0, mediaCodec->Init(UNKNOW_MIME_TYPE, true));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Init_05, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_NE(0, mediaCodec->Init(UNKNOW_MIME_TYPE, false));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Init_06, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    EXPECT_NE(0, mediaCodec->Init(UNKNOW_MIME_TYPE, false));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Init_07, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_NE(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Configure_08, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_NE(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_Prepare_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_NE(0, mediaCodec->Prepare());
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_SetDumpInfo_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    mediaCodec->SetDumpInfo(false, 0);
    mediaCodec->SetDumpInfo(true, 0);
    mediaCodec->SetDumpInfo(false, 0);
    mediaCodec->SetDumpInfo(true, 1);
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_OnDumpInfo_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_AAC_IS_ADTS>(AUDIO_AAC_IS_ADTS);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(0, mediaCodec->Configure(meta));
    auto implBufferQueue_ =
        Media::AVBufferQueue::Create(DEFAULT_BUFFER_NUM, Media::MemoryType::SHARED_MEMORY, "UT-TEST");
    EXPECT_EQ(0, mediaCodec->SetOutputBufferQueue(implBufferQueue_->GetProducer()));
    EXPECT_EQ(0, mediaCodec->Prepare());
    mediaCodec->OnDumpInfo(0);
    mediaCodec->OnDumpInfo(1);
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_ProcessInputBuffer_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    mediaCodec->ProcessInputBuffer();
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_SetCodecCallback_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    std::shared_ptr<Media::CodecCallback> codecCallback = std::make_shared<TestCodecCallback>();
    EXPECT_EQ(0, mediaCodec->SetCodecCallback(codecCallback));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_SetCodecCallback_02, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    std::shared_ptr<Media::AudioBaseCodecCallback> mediaCallback = std::make_shared<AudioCodecCallback>();
    EXPECT_EQ(0, mediaCodec->SetCodecCallback(mediaCallback));
    mediaCodec = nullptr;
}
HWTEST_F(MediaCodecUnitTest, Test_SetOutputSurface_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    sptr<Surface> surface = nullptr;
    EXPECT_EQ(0, mediaCodec->SetOutputSurface(surface));
    mediaCodec = nullptr;
}

HWTEST_F(MediaCodecUnitTest, Test_GetInputSurface_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_EQ(nullptr, mediaCodec->GetInputSurface());
    mediaCodec = nullptr;
}
}  // namespace MediaAVCodec
}  // namespace OHOS