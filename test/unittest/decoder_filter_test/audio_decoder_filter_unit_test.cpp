/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "audio_decoder_filter.h"
#include "filter/filter_factory.h"
#include "common/log.h"
#include "common/media_core.h"
#include "avcodec_sysevent.h"

#include "audio_decoder_filter_unit_test.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
void AudioDecoderFilterUnitTest::SetUpTestCase(void)
{
}

void AudioDecoderFilterUnitTest::TearDownTestCase(void)
{
}

void AudioDecoderFilterUnitTest::SetUp()
{
}

void AudioDecoderFilterUnitTest::TearDown()
{
}

HWTEST_F(AudioDecoderFilterUnitTest, AudioDecoderFilter_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("AudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    audioDecoder->Init(eventReceive, filterCallback);
    EXPECT_EQ(audioDecoder->DoPrepare(), Status::OK);
    audioDecoder->filterType_ = Pipeline::FilterType::FILTERTYPE_ADEC;
    EXPECT_EQ(audioDecoder->DoPrepare(), Status::OK);

    std::shared_ptr<Meta> format = std::make_shared<Meta>();
    audioDecoder->SetParameter(format);
    audioDecoder->GetParameter(format);
    audioDecoder->OnUpdatedResult(format);
    audioDecoder->OnUnlinkedResult(format);

    EXPECT_EQ(audioDecoder->DoStart(), Status::ERROR_INVALID_STATE);
    EXPECT_EQ(audioDecoder->DoPause(), Status::OK);
    EXPECT_EQ(audioDecoder->DoFlush(), Status::ERROR_INVALID_STATE);
    EXPECT_EQ(audioDecoder->DoResume(), Status::ERROR_INVALID_STATE);
    EXPECT_EQ(audioDecoder->DoStop(), Status::OK);
    EXPECT_EQ(audioDecoder->DoRelease(), Status::OK);

    std::shared_ptr<Pipeline::AudioDecoderCallback> audioDecoderCallback =
        std::make_shared<Pipeline::AudioDecoderCallback>(audioDecoder);
    audioDecoderCallback->OnOutputBufferDone(nullptr);
    audioDecoderCallback->OnError(CodecErrorType::CODEC_DRM_DECRYTION_FAILED, 111);

    EXPECT_EQ(audioDecoder->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(audioDecoder->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
}

HWTEST_F(AudioDecoderFilterUnitTest, AudioDecoderFilter_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("AudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    audioDecoder->Init(eventReceive, filterCallback);
    EXPECT_EQ(audioDecoder->DoPrepareFrame(false), Status::ERROR_INVALID_STATE);
    audioDecoder->SetDumpFlag(false);
    audioDecoder->OnDumpInfo(-1);
    audioDecoder->SetCallerInfo(2, "test");
    audioDecoder->SetCallerInfo(2, "test");
    EXPECT_EQ(audioDecoder->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, nullptr), Status::OK);
    EXPECT_EQ(audioDecoder->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, nullptr, nullptr), Status::OK);
    EXPECT_EQ(audioDecoder->GetFilterType(), Pipeline::FilterType::FILTERTYPE_AENC);
}

HWTEST_F(AudioDecoderFilterUnitTest, AudioDecoderFilter_003, TestSize.Level1)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("AudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    audioDecoder->Init(eventReceive, filterCallback);
    audioDecoder->OnOutputBufferDone(nullptr);
    audioDecoder->OnError(CodecErrorType::CODEC_DRM_DECRYTION_FAILED, 111);
    audioDecoder->eventReceiver_ = nullptr;
    audioDecoder->OnError(CodecErrorType::CODEC_ERROR_EXTEND_START, 111);
    
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	    AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    audioDecoder->inputBufferQueueProducer_ = inputBufferQueue->GetProducer();
    std::shared_ptr<AVBuffer> inputBuffer;
    audioDecoder->OnBufferFilled(inputBuffer);

    audioDecoder->GetInputBufferQueue();
    EXPECT_EQ(audioDecoder->SetDecryptionConfig(nullptr, true), Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(AudioDecoderFilterUnitTest, AudioDecoderFilter_004, TestSize.Level1)
{
    std::shared_ptr<Pipeline::AudioDecoderFilter> audioDecoder =
        std::make_shared<Pipeline::AudioDecoderFilter>("AudioDecoderFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    audioDecoder->Init(eventReceive, filterCallback);

    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
    EXPECT_EQ(audioDecoder->ChangePlugin(meta), Status::ERROR_UNSUPPORTED_FORMAT);
    EXPECT_EQ(audioDecoder->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, meta, nullptr),
        Status::ERROR_UNSUPPORTED_FORMAT);
}

}