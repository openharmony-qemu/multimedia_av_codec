/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "audio_encoder_filter_unitest.h"
#include "common/log.h"
#include "parameters.h"
 
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
namespace Pipeline {

class EventReceiverTest : public EventReceiver {
public:
    EventReceiverTest() = default;
    ~EventReceiverTest() = default;
    void OnEvent(const Event &event) override
    {
        (void)event;
    }
};
 
class FilterCallbackTest : public FilterCallback {
public:
    FilterCallbackTest() = default;
    ~FilterCallbackTest() = default;
    Status OnCallback(const std::shared_ptr<Filter>& filter, FilterCallBackCommand cmd, StreamType outType) override
    {
        (void)filter;
        (void)cmd;
        (void)outType;
        return Status::OK;
    }
};

class MediaCodecMock : public OHOS::Media::MediaCodec {
public:
    
    int32_t Init(const std::string &mime, bool isEncoder)
    {
        (void)mime;
        (void)isEncoder;
        return 0;
    }
    int32_t Configure(const std::shared_ptr<Meta> &meta)
    {
        (void)meta;
        return configure_;
    }
protected:
    int32_t configure_ = 0;
};


void AudioEncoderFilterUnitest::SetUpTestCase(void) {}
 
void AudioEncoderFilterUnitest::TearDownTestCase(void) {}
 
void AudioEncoderFilterUnitest::SetUp(void)
{
    audioEncoderFilter_ =
        std::make_shared<AudioEncoderFilter>("testAudioEncoderFilter", FilterType::FILTERTYPE_AENC);
    ASSERT_NE(audioEncoderFilter_, nullptr);
}
 
void AudioEncoderFilterUnitest::TearDown(void)
{
    audioEncoderFilter_ = nullptr;
}
 

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_Destructor_0100, TestSize.Level1)
{
    audioEncoderFilter_ =
        std::make_shared<AudioEncoderFilter>("testAudioEncoderFilter", FilterType::FILTERTYPE_AENC);
    EXPECT_NE(audioEncoderFilter_, nullptr);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_SetCodecFormat_0100, TestSize.Level1)
{
    // format == nulllptr
    std::shared_ptr<Meta> format = std::make_shared<Meta>();
    EXPECT_EQ(audioEncoderFilter_->SetCodecFormat(format), Status::ERROR_INVALID_PARAMETER);

    // format != nullptr
    format->Set<Tag::MIME_TYPE>(std::string("audio/mp3"));
    EXPECT_EQ(audioEncoderFilter_->SetCodecFormat(format), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_Init_0100, TestSize.Level1)
{
    auto mediaCodecMock = std::make_shared<MediaCodecMock>();
    audioEncoderFilter_->mediaCodec_ = mediaCodecMock;
    std::shared_ptr<EventReceiver> receiver = std::make_shared<EventReceiverTest>();
    std::shared_ptr<FilterCallback> callback = std::make_shared<FilterCallbackTest>();
    audioEncoderFilter_->Init(receiver, callback);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_Configure_0100, TestSize.Level1)
{
    auto mediaCodecMock = std::make_shared<MediaCodecMock>();
    audioEncoderFilter_->mediaCodec_ = mediaCodecMock;
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    //ret = 0
    EXPECT_NE(audioEncoderFilter_->Configure(parameter), Status::OK);
    //ret != 0
    mediaCodecMock->configure_ = 1;
    EXPECT_NE(audioEncoderFilter_->Configure(parameter), Status::OK);
}

}
}
}