/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <string>
#include <vector>
#include "media_demuxer_audio_support_test.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;

void MediaDemuxerAudioSupportTest::SetUpTestCase(void) {}
void MediaDemuxerAudioSupportTest::TearDownTestCase(void) {}
void MediaDemuxerAudioSupportTest::SetUp(void) {}
void MediaDemuxerAudioSupportTest::TearDown(void) {}

// FilterCallback mock whose dolby list and passthrough result are configurable per test.
class AudioSupportFilterCallback : public Pipeline::FilterCallback {
public:
    AudioSupportFilterCallback() = default;
    ~AudioSupportFilterCallback() override = default;

    Status OnCallback(const std::shared_ptr<Pipeline::Filter>& filter, Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType) override
    {
        (void)filter;
        (void)cmd;
        (void)outType;
        return Status::OK;
    }

    std::vector<std::string> GetDolbyListCallback() override
    {
        return dolbyList;
    }

    bool IsAudioPassthroughCallback(const char* mimeType) override
    {
        (void)mimeType;
        return isPassthrough;
    }

    std::vector<std::string> dolbyList;
    bool isPassthrough = false;
};

// AVCodecList mock whose capability query result is configurable per test.
class AudioSupportTestCodecList : public MediaAVCodec::AVCodecList {
public:
    AudioSupportTestCodecList() = default;
    ~AudioSupportTestCodecList() override = default;

    std::string FindDecoder(const MediaAVCodec::Format& format) override
    {
        (void)format;
        return "";
    }

    std::string FindEncoder(const MediaAVCodec::Format& format) override
    {
        (void)format;
        return "";
    }

    MediaAVCodec::CapabilityData* GetCapability(const std::string& mime, const bool isEncoder,
        const MediaAVCodec::AVCodecCategory& category) override
    {
        (void)mime;
        (void)isEncoder;
        (void)category;
        return capability;
    }

    void* GetBuffer(const std::string& name, uint32_t sizeOfCap) override
    {
        (void)name;
        (void)sizeOfCap;
        return nullptr;
    }

    void* NewBuffer(size_t bufSize) override
    {
        (void)bufSize;
        return nullptr;
    }

    std::vector<std::shared_ptr<MediaAVCodec::CapabilityData>> GetCapabilityList(int32_t codecType) override
    {
        (void)codecType;
        return {};
    }

    MediaAVCodec::CapabilityData* capability = nullptr;
};

/**
 * @tc.name  : Test IsMimeInDolbyList
 * @tc.number: MediaDemuxer_IsMimeInDolbyList_001
 * @tc.desc  : Test IsMimeInDolbyList with null callback, dolby list hit/miss and cached list
 */
HWTEST_F(MediaDemuxerAudioSupportTest, MediaDemuxer_IsMimeInDolbyList_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    // no hiplayer callback: mime is never in the dolby list
    EXPECT_FALSE(demuxer->IsMimeInDolbyList("audio/eac3"));

    auto callback = std::make_shared<AudioSupportFilterCallback>();
    demuxer->SetHiplayerCallback(callback);
    // callback returns an empty dolby list
    EXPECT_FALSE(demuxer->IsMimeInDolbyList("audio/eac3"));
    // callback returns a list containing the mime
    callback->dolbyList = {"audio/eac3", "audio/ac3"};
    EXPECT_TRUE(demuxer->IsMimeInDolbyList("audio/eac3"));
    EXPECT_FALSE(demuxer->IsMimeInDolbyList("audio/mp4a-latm"));
    // the dolby list is cached after the first fetch: clearing the callback list has no effect
    callback->dolbyList = {};
    EXPECT_TRUE(demuxer->IsMimeInDolbyList("audio/eac3"));
}

/**
 * @tc.name  : Test IsAudioSupport
 * @tc.number: MediaDemuxer_IsAudioSupport_001
 * @tc.desc  : Test raw audio is always supported and dolby passthrough is supported
 */
HWTEST_F(MediaDemuxerAudioSupportTest, MediaDemuxer_IsAudioSupport_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    // raw audio is always supported
    EXPECT_TRUE(demuxer->IsAudioSupport("audio/raw"));

    auto callback = std::make_shared<AudioSupportFilterCallback>();
    callback->dolbyList = {"audio/eac3"};
    callback->isPassthrough = true;
    demuxer->SetHiplayerCallback(callback);
    // dolby mime with passthrough enabled is supported
    EXPECT_TRUE(demuxer->IsAudioSupport("audio/eac3"));
}

/**
 * @tc.name  : Test IsAudioSupport
 * @tc.number: MediaDemuxer_IsAudioSupport_002
 * @tc.desc  : Test IsAudioSupport with codec list whose capability query fails
 */
HWTEST_F(MediaDemuxerAudioSupportTest, MediaDemuxer_IsAudioSupport_002, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    auto codecList = std::make_shared<AudioSupportTestCodecList>();
    codecList->capability = nullptr;
    demuxer->SetCodecListCreator([codecList]() { return codecList; });
    // capability query returns null: the mime is not supported
    EXPECT_FALSE(demuxer->IsAudioSupport("audio/ape"));
}

/**
 * @tc.name  : Test IsAudioSupport
 * @tc.number: MediaDemuxer_IsAudioSupport_003
 * @tc.desc  : Test IsAudioSupport with codec list whose capability query succeeds
 */
HWTEST_F(MediaDemuxerAudioSupportTest, MediaDemuxer_IsAudioSupport_003, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    auto codecList = std::make_shared<AudioSupportTestCodecList>();
    MediaAVCodec::CapabilityData capabilityData;
    codecList->capability = &capabilityData;
    demuxer->SetCodecListCreator([codecList]() { return codecList; });
    // capability query returns valid data: the mime is supported
    EXPECT_TRUE(demuxer->IsAudioSupport("audio/ape"));
}

/**
 * @tc.name  : Test SetOutputBufferQueue
 * @tc.number: MediaDemuxer_SetOutputBufferQueue_002
 * @tc.desc  : Test replacing an existing bufferQueue while demuxer is running and
 *             rejecting a brand-new bufferQueue while running
 */
HWTEST_F(MediaDemuxerAudioSupportTest, MediaDemuxer_SetOutputBufferQueue_002, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    auto pluginManager = std::make_shared<DemuxerPluginManager>();
    auto plugin = std::make_shared<DemuxerPluginMock>("StatusOK");
    MediaStreamInfo streamInfo;
    streamInfo.streamID = 0;
    streamInfo.plugin = plugin;
    pluginManager->streamInfoMap_[0] = streamInfo;
    MediaTrackMap trackMap;
    trackMap.trackID = 0;
    trackMap.streamID = 0;
    trackMap.innerTrackIndex = 0;
    pluginManager->trackInfoMap_[0] = trackMap;
    demuxer->demuxerPluginManager_ = pluginManager;

    demuxer->mediaMetaData_.trackMetas = {
        std::make_shared<Meta>(),
        std::make_shared<Meta>(),
        std::make_shared<Meta>(),
    };
    demuxer->isThreadExit_ = true;

    std::shared_ptr<AVBufferQueue> queue1 = AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testAudioTrackQueue1");
    sptr<AVBufferQueueProducer> producer1 = queue1->GetProducer();
    std::shared_ptr<AVBufferQueue> queue2 = AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testAudioTrackQueue2");
    sptr<AVBufferQueueProducer> producer2 = queue2->GetProducer();

    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, producer1), Status::OK);

    // audio pipeline rebuild while the demuxer thread is still running: the stale queue is replaced
    demuxer->isThreadExit_ = false;
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, producer2), Status::OK);
    std::map<int32_t, sptr<AVBufferQueueProducer>> queueMap = demuxer->GetBufferQueueProducerMap();
    EXPECT_EQ(queueMap.size(), 1);
    EXPECT_EQ(queueMap[0], producer2);

    // a brand-new track is still rejected while running
    EXPECT_EQ(demuxer->SetOutputBufferQueue(2, producer2), Status::ERROR_WRONG_STATE);
    demuxer->isThreadExit_ = true;
    // the rejected set does not register the queue
    EXPECT_EQ(demuxer->GetBufferQueueProducerMap().size(), 1);
}
}  // namespace OHOS::Media
