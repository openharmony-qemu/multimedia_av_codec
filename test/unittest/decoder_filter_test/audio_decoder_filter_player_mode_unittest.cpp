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

#ifdef SUPPORT_DRM
#include "imedia_key_session_service.h"
#endif
#include "audio_decoder_filter_unit_test.h"

using namespace testing::ext;

namespace OHOS::Media {
namespace {
const std::string UNSUPPORTED_AUDIO_MIME = "audio/unsupported_mime_for_test";
}

/**
 * @tc.name: AudioDecoderFilter_UnLinkNext_0100
 * @tc.desc: Test UnLinkNext with a valid filter resets nextFilter_
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterUnitTest, AudioDecoderFilter_UnLinkNext_0100, TestSize.Level1)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("AudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_ADEC);
    std::shared_ptr<Pipeline::AudioDecoderFilter> nextDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("NextAudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_ADEC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    audioDecoder->Init(eventReceive, filterCallback);
    audioDecoder->nextFilter_ = nextDecoder;
    EXPECT_EQ(audioDecoder->UnLinkNext(nextDecoder, Pipeline::StreamType::STREAMTYPE_ENCODED_AUDIO), Status::OK);
    EXPECT_EQ(audioDecoder->nextFilter_, nullptr);
}

/**
 * @tc.name: AudioDecoderFilter_OnLinkedPlayerMode_0100
 * @tc.desc: Test OnLinked returns ERROR_UNSUPPORTED_FORMAT when decoder init fails in player mode
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterUnitTest, AudioDecoderFilter_OnLinkedPlayerMode_0100, TestSize.Level1)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("AudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_ADEC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    audioDecoder->Init(eventReceive, filterCallback);
    audioDecoder->SetPlayerMode();
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::MIME_TYPE, UNSUPPORTED_AUDIO_MIME);
    // decoder init fails for the unsupported mime: player mode degrades instead of failing playback
    EXPECT_EQ(audioDecoder->OnLinked(Pipeline::StreamType::STREAMTYPE_ENCODED_AUDIO, meta, nullptr),
        Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: AudioDecoderFilter_OnLinkedNonPlayerMode_0100
 * @tc.desc: Test OnLinked reports error and returns ERROR_INVALID_PARAMETER when decoder init fails
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderFilterUnitTest, AudioDecoderFilter_OnLinkedNonPlayerMode_0100, TestSize.Level1)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("AudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_ADEC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    audioDecoder->Init(eventReceive, filterCallback);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::MIME_TYPE, UNSUPPORTED_AUDIO_MIME);
    // non-player mode: error event is reported and the link fails with ERROR_INVALID_PARAMETER
    EXPECT_EQ(audioDecoder->OnLinked(Pipeline::StreamType::STREAMTYPE_ENCODED_AUDIO, meta, nullptr),
        Status::ERROR_INVALID_PARAMETER);
}
}  // namespace OHOS::Media
