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
#include "filter/filter.h"
 
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
using namespace testing;

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
    int32_t Start()
    {
        return start_;
    }
    int32_t Stop()
    {
        return stop_;
    }
    int32_t Flush()
    {
        return flush_;
    }
    int32_t Release()
    {
        return release_;
    }
    int32_t NotifyEos()
    {
        return notifyEos_;
    }
    int32_t SetParameter(const std::shared_ptr<Meta> &parameter)
    {
        (void)parameter;
        return 0;
    }
    int32_t GetOutputFormat(std::shared_ptr<Meta> &parameter)
    {
        int32_t ret = 0;
        if (getOutputFormat_) {
            int32_t frameSize = 1024;
            parameter->Set<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize);
        } else {
            ret = -1;
        }
        return ret;
    }
    int32_t SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer)
    {
        (void)bufferQueueProducer;
        return 0;
    }
    int32_t Prepare()
    {
        return 0;
    }
    sptr<AVBufferQueueProducer> GetInputBufferQueue()
    {
        uint32_t size = 8;
        inputBufferQueue_ = AVBufferQueue::Create(size);
        if (inputBufferQueue_ == nullptr) {
            return nullptr;
        }
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        return inputBufferQueueProducer_;
    }
protected:
    int32_t configure_ = 0;
    int32_t start_ = 0;
    int32_t stop_ = 0;
    int32_t flush_ = 0;
    int32_t release_ = 0;
    int32_t notifyEos_ = 0;
    bool getOutputFormat_ = false;
    std::shared_ptr<AVBufferQueue> inputBufferQueue_ = nullptr;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_ = nullptr;
};

class FilterMock : public Filter {
public:
    FilterMock():Filter("filterMock", FilterType::FILTERTYPE_SOURCE) {}
    ~FilterMock() = default;
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
                            const std::shared_ptr<FilterLinkCallback>& callback)
    {
        (void)inType;
        (void)meta;
        (void)callback;
        return onLinked_;
    }
protected:
    Status onLinked_;
};

class FilterLinkCallbackTest : public FilterLinkCallback {
public:
    FilterLinkCallbackTest() = default;
    ~FilterLinkCallbackTest() = default;
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta) override
    {
        (void)queue;
        (void)meta;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta>& meta) override
    {
        (void)meta;
    }
    void OnUpdatedResult(std::shared_ptr<Meta>& meta) override
    {
        (void)meta;
    }
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
    ASSERT_NE(audioEncoderFilter_->mediaCodec_, nullptr);
    std::shared_ptr<EventReceiver> receiver = std::make_shared<EventReceiverTest>();
    std::shared_ptr<FilterCallback> callback = std::make_shared<FilterCallbackTest>();
    audioEncoderFilter_->Init(receiver, callback);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_Configure_0100, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mediaCodec;
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    //ret != 0
    EXPECT_NE(audioEncoderFilter_->Configure(parameter), Status::OK);
    //ret = 0
    auto mockCodecPlugin = std::make_shared<MockCodecPlugin>("video/avc");
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::INITIALIZED;
    audioEncoderFilter_->mediaCodec_->codecPlugin_ = mockCodecPlugin;
    EXPECT_CALL(*mockCodecPlugin, SetParameter(testing::_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockCodecPlugin, SetDataCallback(testing::_)).WillOnce(Return(Status::OK));
    EXPECT_EQ(audioEncoderFilter_->Configure(parameter), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_DoPrepare_0100, TestSize.Level1)
{
    auto mediaCodecMock = std::make_shared<MediaCodecMock>();
    audioEncoderFilter_->mediaCodec_ = mediaCodecMock;
    auto filterCallback = std::make_shared<FilterCallbackTest>();

    // filterType_ == FilterType::FILTERTYPE_AENC
    audioEncoderFilter_->filterType_ = FilterType::FILTERTYPE_AENC;
    audioEncoderFilter_->filterCallback_ = filterCallback;
    EXPECT_EQ(audioEncoderFilter_->DoPrepare(), Status::OK);

    // filterType_ != FilterType::FILTERTYPE_AENC
    audioEncoderFilter_->filterType_ = FilterType::FILTERTYPE_ADEC;
    EXPECT_EQ(audioEncoderFilter_->DoPrepare(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_DoStart_0100, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mediaCodec;

    //ret != 0
    EXPECT_NE(audioEncoderFilter_->DoStart(), Status::OK);
    //ret = 0
    auto mockCodecPlugin = std::make_shared<MockCodecPlugin>("video/avc");
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::PREPARED;
    audioEncoderFilter_->mediaCodec_->codecPlugin_ = mockCodecPlugin;
    EXPECT_CALL(*mockCodecPlugin, Start()).WillOnce(Return(Status::OK));
    EXPECT_EQ(audioEncoderFilter_->DoStart(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_DoPause_0100, TestSize.Level1)
{
    EXPECT_EQ(audioEncoderFilter_->DoPause(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_DoResume_0100, TestSize.Level1)
{
    EXPECT_EQ(audioEncoderFilter_->DoResume(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_DoStop_0100, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mediaCodec;

    //ret != 0
    auto mockCodecPlugin = std::make_shared<MockCodecPlugin>("video/avc");
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::RUNNING;
    audioEncoderFilter_->mediaCodec_->codecPlugin_ = mockCodecPlugin;
    EXPECT_CALL(*mockCodecPlugin, Stop()).WillOnce(Return(Status::ERROR_UNKNOWN));
    EXPECT_NE(audioEncoderFilter_->DoStop(), Status::OK);
    //ret = 0
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::PREPARED;
    EXPECT_EQ(audioEncoderFilter_->DoStop(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_DoFlush_0100, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mediaCodec;

    //ret != 0
    EXPECT_NE(audioEncoderFilter_->DoFlush(), Status::OK);
    //ret = 0
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::FLUSHED;
    EXPECT_EQ(audioEncoderFilter_->DoFlush(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_DoRelease_0100, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mediaCodec;

    //ret != 0
    auto mockCodecPlugin = std::make_shared<MockCodecPlugin>("video/avc");
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::RUNNING;
    audioEncoderFilter_->mediaCodec_->codecPlugin_ = mockCodecPlugin;
    EXPECT_CALL(*mockCodecPlugin, Release()).WillOnce(Return(Status::ERROR_UNKNOWN));
    EXPECT_NE(audioEncoderFilter_->DoRelease(), Status::OK);
    //ret = 0
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::RELEASING;
    EXPECT_EQ(audioEncoderFilter_->DoRelease(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_NotifyEos_0100, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    audioEncoderFilter_->mediaCodec_ = mediaCodec;

    // ret != 0
    EXPECT_NE(audioEncoderFilter_->NotifyEos(), Status::OK);

    // ret == 0
    audioEncoderFilter_->mediaCodec_->state_ = CodecState::END_OF_STREAM;
    EXPECT_EQ(audioEncoderFilter_->NotifyEos(), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_SetParameter_0100, TestSize.Level1)
{
    auto mediaCodecMock = std::make_shared<MediaCodecMock>();
    audioEncoderFilter_->mediaCodec_ = mediaCodecMock;
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    ASSERT_NE(parameter, nullptr);
    audioEncoderFilter_->SetParameter(parameter);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_GetParameter_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    ASSERT_NE(parameter, nullptr);
    audioEncoderFilter_->GetParameter(parameter);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_LinkNext_0100, TestSize.Level1)
{
    auto mediaCodecMock = std::make_shared<MediaCodecMock>();
    auto filterMock = std::make_shared<FilterMock>();
    std::shared_ptr<Filter> nextFilter = filterMock;

    // mediaCodec_ == nullptr && ret != Status::OK
    audioEncoderFilter_->mediaCodec_ = nullptr;
    filterMock->onLinked_ = Status::ERROR_INVALID_PARAMETER;
    EXPECT_NE(audioEncoderFilter_->LinkNext(nextFilter, StreamType::STREAMTYPE_ENCODED_VIDEO), Status::OK);

    // mediaCodec_ == nullptr && ret == Status::OK
    audioEncoderFilter_->mediaCodec_ = nullptr;
    filterMock->onLinked_ = Status::OK;
    EXPECT_EQ(audioEncoderFilter_->LinkNext(nextFilter, StreamType::STREAMTYPE_ENCODED_VIDEO), Status::OK);

    // mediaCodec_ != nullptr && parameter->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != parameter->end() &&
    // parameter->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize) && ret != Status::OK
    audioEncoderFilter_->mediaCodec_ = mediaCodecMock;
    mediaCodecMock->getOutputFormat_ = true;
    filterMock->onLinked_ = Status::ERROR_INVALID_PARAMETER;
    EXPECT_NE(audioEncoderFilter_->LinkNext(nextFilter, StreamType::STREAMTYPE_ENCODED_VIDEO), Status::OK);

    // mediaCodec_ != nullptr && parameter->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != parameter->end() &&
    // parameter->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize) && ret != Status::OK
    audioEncoderFilter_->mediaCodec_ = mediaCodecMock;
    mediaCodecMock->getOutputFormat_ = false;
    filterMock->onLinked_ = Status::ERROR_INVALID_PARAMETER;
    EXPECT_NE(audioEncoderFilter_->LinkNext(nextFilter, StreamType::STREAMTYPE_ENCODED_VIDEO), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_UpdateNext_0100, TestSize.Level1)
{
    auto filterMock = std::make_shared<FilterMock>();
    std::shared_ptr<Filter> nextFilter = filterMock;
    EXPECT_EQ(audioEncoderFilter_->UpdateNext(nextFilter, StreamType::STREAMTYPE_ENCODED_VIDEO), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_UnLinkNext_0100, TestSize.Level1)
{
    auto filterMock = std::make_shared<FilterMock>();
    std::shared_ptr<Filter> nextFilter = filterMock;
    EXPECT_EQ(audioEncoderFilter_->UnLinkNext(nextFilter, StreamType::STREAMTYPE_ENCODED_VIDEO), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_GetFilterType_0100, TestSize.Level1)
{
    audioEncoderFilter_->filterType_ = FilterType::FILTERTYPE_AENC;
    EXPECT_EQ(audioEncoderFilter_->GetFilterType(), FilterType::FILTERTYPE_AENC);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_OnLinked_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    std::shared_ptr<FilterLinkCallback> callback = std::make_shared<FilterLinkCallbackTest>();
    EXPECT_EQ(audioEncoderFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, meta, callback), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_OnUpdated_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    std::shared_ptr<FilterLinkCallback> callback = std::make_shared<FilterLinkCallbackTest>();
    EXPECT_EQ(audioEncoderFilter_->OnUpdated(StreamType::STREAMTYPE_ENCODED_VIDEO, meta, callback), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_OnUnLinked_0100, TestSize.Level1)
{
    std::shared_ptr<FilterLinkCallback> callback = std::make_shared<FilterLinkCallbackTest>();
    EXPECT_EQ(audioEncoderFilter_->OnUnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, callback), Status::OK);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_OnLinkedResult_0100, TestSize.Level1)
{
    auto mediaCodecMock = std::make_shared<MediaCodecMock>();
    ASSERT_NE(mediaCodecMock, nullptr);
    audioEncoderFilter_->mediaCodec_ = mediaCodecMock;
    
    audioEncoderFilter_->onLinkedResultCallback_ = std::make_shared<FilterLinkCallbackTest>();
    ASSERT_NE(audioEncoderFilter_->onLinkedResultCallback_, nullptr);
    
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_NE(meta, nullptr);
    sptr<AVBufferQueueProducer> outputBufferQueue = nullptr;
    audioEncoderFilter_->OnLinkedResult(outputBufferQueue, meta);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_OnUpdatedResult_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_NE(meta, nullptr);
    audioEncoderFilter_->OnUpdatedResult(meta);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_OnUnlinkedResult_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_NE(meta, nullptr);
    audioEncoderFilter_->OnUnlinkedResult(meta);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_SetCallingInfo_0100, TestSize.Level1)
{
    int32_t appUid = 1000;
    int32_t appPid = 1001;
    uint64_t instanceId = 1002;
    std::string bundleName = "bundleName";
    audioEncoderFilter_->SetCallingInfo(appUid, appPid, bundleName, instanceId);
    EXPECT_EQ(audioEncoderFilter_->appUid_, appUid);
    EXPECT_EQ(audioEncoderFilter_->appPid_, appPid);
    EXPECT_EQ(audioEncoderFilter_->bundleName_, bundleName);
    EXPECT_EQ(audioEncoderFilter_->instanceId_, instanceId);
}

HWTEST_F(AudioEncoderFilterUnitest, AudioEncoderFilter_GetInputSurface_0100, TestSize.Level1)
{
    audioEncoderFilter_->mediaCodec_ = std::make_shared<MediaCodecMock>();
    sptr<Surface> surface = audioEncoderFilter_->GetInputSurface();
    EXPECT_EQ(surface, nullptr);
}

}
}
}