/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "audio_codec_adapter.h"
#include <malloc.h>
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_description.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

namespace OHOS {
namespace MediaAVCodec {

class AdapterCallback : public AVCodecCallback {
public:
    AdapterCallback() = default;
    ~AdapterCallback() = default;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;
};

void AdapterCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
}

void AdapterCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
}

void AdapterCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    (void)index;
    (void)buffer;
}

void AdapterCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                              shared_ptr<AVSharedMemory> buffer)
{
    (void)index;
    (void)info;
    (void)flag;
    (void)buffer;
}

class AudioCodecAdapterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    shared_ptr<AudioCodecAdapter> adapter_ = nullptr;
    Format format_;
};

void AudioCodecAdapterUnitTest::SetUpTestCase(void)
{
}

void AudioCodecAdapterUnitTest::TearDownTestCase(void)
{
}

void AudioCodecAdapterUnitTest::SetUp(void)
{
    adapter_ = make_shared<AudioCodecAdapter>("OH.Media.Codec.Decoder.Audio.AAC");
    ASSERT_NE(nullptr, adapter_);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 16000); // 16000: test sample rate
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, Plugins::AudioSampleFormat::SAMPLE_S16LE);
}

void AudioCodecAdapterUnitTest::TearDown(void)
{
    adapter_ = nullptr;
}

HWTEST_F(AudioCodecAdapterUnitTest, InvalidProcess_01, TestSize.Level1)
{
    shared_ptr<AdapterCallback> callback = make_shared<AdapterCallback>();
    EXPECT_EQ(adapter_->SetCallback(callback), 0);
    EXPECT_NE(adapter_->Start(), 0);
}

HWTEST_F(AudioCodecAdapterUnitTest, InvalidProcess_02, TestSize.Level1)
{
    adapter_ = make_shared<AudioCodecAdapter>("invalid");
    ASSERT_EQ(adapter_->Release(), 0);
}

HWTEST_F(AudioCodecAdapterUnitTest, InvalidProcess_03, TestSize.Level1)
{
    adapter_ = make_shared<AudioCodecAdapter>("");
    Meta callerInfo;
    ASSERT_NE(adapter_->Init(callerInfo), 0);
    ASSERT_NE(adapter_->Configure(format_), 0);
}

} // MediaAVCodec
} // OHOS