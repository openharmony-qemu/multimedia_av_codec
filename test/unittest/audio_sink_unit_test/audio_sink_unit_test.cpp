/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "audio_sink.h"
#include "audio_effect.h"
#include "filter/filter.h"
#include "common/log.h"
#include "sink/media_synchronous_sink.h"
#include "buffer/avsharedmemory.h"
#include "buffer/avsharedmemorybase.h"
#include "buffer/avbuffer_queue_consumer.h"


namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioSinkUnitTest" };
}

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {
static const int32_t INVALID_NUM = -3;
static const int32_t TEST_NUM = 12;
static const int32_t SLEEP_TIME = 60;

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        MEDIA_LOG_I("TestEventReceiver ctor ");
    }

    void OnEvent(const Event &event)
    {
        MEDIA_LOG_I("TestEventReceiver OnEvent " PUBLIC_LOG_S, event.srcFilter.c_str());
    }
};

class TestAudioSinkUnitMock : public AudioSinkPlugin {
public:

    explicit TestAudioSinkUnitMock(std::string name): AudioSinkPlugin(std::move(name)) {}

    Status Start() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
        return Status::ERROR_UNKNOWN;
    };

    Status Stop() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status PauseTransitent() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
        return Status::ERROR_UNKNOWN;
    };

    Status Pause() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
        return Status::ERROR_UNKNOWN;
    };

    Status Resume() override
    {
        return Status::ERROR_UNKNOWN;
    };

    Status GetLatency(uint64_t &hstTime) override
    {
        (void)hstTime;
        return Status::ERROR_UNKNOWN;
    };

    Status SetAudioEffectMode(int32_t effectMode) override
    {
        (void)effectMode;
        return Status::ERROR_UNKNOWN;
    };

    Status GetAudioEffectMode(int32_t &effectMode) override
    {
        (void)effectMode;
        return Status::ERROR_UNKNOWN;
    };

    int64_t GetPlayedOutDurationUs(int64_t nowUs) override
    {
        (void)nowUs;
        return 0;
    }
    Status GetMute(bool& mute) override
    {
        (void)mute;
        return Status::ERROR_UNKNOWN;
    }
    Status SetMute(bool mute) override
    {
        (void)mute;
        return Status::ERROR_UNKNOWN;
    }

    Status GetVolume(float& volume) override
    {
        (void)volume;
        return Status::ERROR_UNKNOWN;
    }
    Status SetVolume(float volume) override
    {
        (void)volume;
        return Status::ERROR_UNKNOWN;
    }
    Status SetVolumeMode(int32_t mode) override
    {
        (void)mode;
        return Status::ERROR_UNKNOWN;
    }
    Status GetSpeed(float& speed) override
    {
        (void)speed;
        return Status::ERROR_UNKNOWN;
    }
    Status SetSpeed(float speed) override
    {
        (void)speed;
        return Status::ERROR_UNKNOWN;
    }
    Status GetFrameSize(size_t& size) override
    {
        (void)size;
        return Status::ERROR_UNKNOWN;
    }
    Status GetFrameCount(uint32_t& count) override
    {
        (void)count;
        return Status::ERROR_UNKNOWN;
    }
    Status Write(const std::shared_ptr<AVBuffer>& input) override
    {
        (void)input;
        return Status::ERROR_UNKNOWN;
    }
    Status Flush() override
    {
        return Status::ERROR_UNKNOWN;
    }
    Status Drain() override
    {
        return Status::ERROR_UNKNOWN;
    }
    Status GetFramePosition(int32_t &framePosition) override
    {
        (void)framePosition;
        return Status::ERROR_UNKNOWN;
    }
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver) override
    {
        (void)receiver;
    }
    int32_t SetVolumeWithRamp(float targetVolume, int32_t duration) override
    {
        (void)targetVolume;
        (void)duration;
        return 0;
    }
    Status SetMuted(bool isMuted) override
    {
        return Status::OK;
    }
    Status SetRequestDataCallback(const std::shared_ptr<AudioSinkDataCallback> &callback) override
    {
        (void)callback;
        return Status::OK;
    }
    bool GetAudioPosition(timespec &time, uint32_t &framePosition) override
    {
        (void)time;
        (void)framePosition;
        return true;
    }
    Status GetBufferDesc(AudioStandard::BufferDesc &bufDesc) override
    {
        (void)bufDesc;
        return Status::OK;
    }
    Status EnqueueBufferDesc(const AudioStandard::BufferDesc &bufDesc) override
    {
        (void)bufDesc;
        return Status::OK;
    }
    bool IsFormatSupported(const std::shared_ptr<Meta> &meta)
    {
        (void)meta;
        return true;
    }
};

std::shared_ptr<AudioSink> AudioSinkUnitCreate()
{
    auto audioSink = std::make_shared<AudioSink>();
    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    auto meta = std::make_shared<Meta>();
    auto initStatus = audioSink->Init(meta, testEventReceiver);
    if (initStatus == Status::OK) {
        return audioSink;
    } else {
        return nullptr;
    }
}

/**
 * @tc.name Test CopyAudioVividBufferData API
 * @tc.number CopyAudioVividBufferData_001
 * @tc.desc Test bufferDesc.buffer != nullptr
 */
HWTEST(TestAudioSink, CopyAudioVividBufferData_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    AudioStandard::BufferDesc bufferDesc;
    bufferDesc.buffer = nullptr;

    size_t size = 0;
    size_t cacheBufferSize = 0;
    int64_t bufferPts = 0;
    std::shared_ptr<AVBuffer> buffer = nullptr;
    bool result = audioSink->CopyAudioVividBufferData(bufferDesc, buffer, size, cacheBufferSize, bufferPts);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name Test CopyAudioVividBufferData API
 * @tc.number CopyAudioVividBufferData_002
 * @tc.desc Test buffer != nullptr
 */
HWTEST(TestAudioSink, CopyAudioVividBufferData_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AudioStandard::BufferDesc bufferDesc;
    bufferDesc.bufLength = 1024;
    bufferDesc.buffer = new uint8_t[bufferDesc.bufLength];
    bufferDesc.metaLength = 128;
    bufferDesc.metaBuffer = new uint8_t[bufferDesc.metaLength];

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    size_t size = 0;
    size_t cacheBufferSize = 0;
    int64_t bufferPts = 0;
    bool result = audioSink->CopyAudioVividBufferData(bufferDesc, avBuffer, size, cacheBufferSize, bufferPts);
    delete[] bufferDesc.buffer;
    delete[] bufferDesc.metaBuffer;

    ASSERT_EQ(result, false);
}

/**
 * @tc.name Test CopyBuffer API
 * @tc.number CopyBuffer_001
 * @tc.desc Test buffer == nullptr
 */
HWTEST(TestAudioSink, CopyBuffer_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    auto result = audioSink->CopyBuffer(avBuffer);
    ASSERT_EQ(result, nullptr);
}

/*
 * @tc.name Test DetectAudioUnderrun API
 * @tc.number DetectAudioUnderrun_001
 * @tc.desc Test lastClkTime_ == HST_TIME_NONE
 */
HWTEST(TestAudioSink, DetectAudioUnderrun_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    int64_t clkTime = 0;
    int64_t latency = 0;
    audioSink->underrunDetector_.lastClkTime_ = HST_TIME_NONE;

    audioSink->underrunDetector_.DetectAudioUnderrun(clkTime, latency);
    EXPECT_EQ(audioSink->underrunDetector_.lastClkTime_, 0);
}

/*
 * @tc.name Test DetectAudioUnderrun API
 * @tc.number DetectAudioUnderrun_002
 * @tc.desc Test underrunTimeUs <= 0
 */
HWTEST(TestAudioSink, DetectAudioUnderrun_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    int64_t clkTime = INVALID_NUM;
    int64_t latency = 0;
    int64_t durationUs = 1;
    audioSink->underrunDetector_.Reset();
    audioSink->underrunDetector_.UpdateBufferTimeNoLock(clkTime, latency);
    audioSink->underrunDetector_.SetLastAudioBufferDuration(durationUs);

    audioSink->underrunDetector_.DetectAudioUnderrun(clkTime, latency);
    int64_t underrunTimeUs = clkTime - audioSink->underrunDetector_.lastClkTime_ -
        (audioSink->underrunDetector_.lastLatency_ + audioSink->underrunDetector_.lastBufferDuration_);
    ASSERT_TRUE(underrunTimeUs < 0);
}

/*
 * @tc.name Test DetectAudioUnderrun API
 * @tc.number DetectAudioUnderrun_003
 * @tc.desc Test underrunTimeUs > 0 && eventReceiver == nullptr
 */
HWTEST(TestAudioSink, DetectAudioUnderrun_003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    int64_t clkTime = TEST_NUM;
    int64_t latency = 0;
    int64_t durationUs = 1;
    audioSink->underrunDetector_.Reset();
    audioSink->underrunDetector_.UpdateBufferTimeNoLock(clkTime, latency);
    audioSink->underrunDetector_.SetLastAudioBufferDuration(durationUs);
    int64_t clkTimeTemp = 10;

    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = nullptr;
    audioSink->underrunDetector_.SetEventReceiver(testEventReceiver);

    audioSink->underrunDetector_.DetectAudioUnderrun(clkTimeTemp, latency);
    int64_t underrunTimeUs = clkTime - audioSink->underrunDetector_.lastClkTime_ -
        (audioSink->underrunDetector_.lastLatency_ + audioSink->underrunDetector_.lastBufferDuration_);
    EXPECT_GE(underrunTimeUs, 0);
    EXPECT_EQ(testEventReceiver, nullptr);
}

/*
 * @tc.name Test DetectAudioUnderrun API
 * @tc.number DetectAudioUnderrun_004
 * @tc.desc Test underrunTimeUs > 0 && eventReceiver != nullptr
 */
HWTEST(TestAudioSink, DetectAudioUnderrun_004, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    int64_t clkTime = TEST_NUM;
    int64_t latency = 0;
    int64_t durationUs = 1;
    audioSink->underrunDetector_.Reset();
    audioSink->underrunDetector_.UpdateBufferTimeNoLock(clkTime, latency);
    audioSink->underrunDetector_.SetLastAudioBufferDuration(durationUs);
    int64_t clkTimeTemp = 10;

    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    audioSink->underrunDetector_.SetEventReceiver(testEventReceiver);

    audioSink->underrunDetector_.DetectAudioUnderrun(clkTimeTemp, latency);
    int64_t underrunTimeUs = clkTime - audioSink->underrunDetector_.lastClkTime_ -
        (audioSink->underrunDetector_.lastLatency_ + audioSink->underrunDetector_.lastBufferDuration_);
    EXPECT_GE(underrunTimeUs, 0);
    EXPECT_NE(testEventReceiver, nullptr);
}

/*
 * @tc.name Test CalcLag API
 * @tc.number CalcLag_001
 * @tc.desc Test maxMediaTime < currentMediaTime
 */
HWTEST(TestAudioSink, CalcLag_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AudioSink::AudioLagDetector::AudioDrainTimeGroup group;
    group.lastAnchorPts = 1;
    group.anchorDuration = 1;
    group.writeDuration = 1;
    group.nowClockTime = 10;

    audioSink->lagDetector_.Reset();
    audioSink->lagDetector_.UpdateDrainTimeGroup(group);

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    bool result = audioSink->lagDetector_.CalcLag(avBuffer);
    ASSERT_EQ(result, true);
}

/*
 * @tc.name Test CalcLag API
 * @tc.number CalcLag_002
 * @tc.desc Test maxMediaTime >= currentMediaTime
 */
HWTEST(TestAudioSink, CalcLag_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AudioSink::AudioLagDetector::AudioDrainTimeGroup group;
    group.lastAnchorPts = 1;
    group.anchorDuration = 1;
    group.writeDuration = 1;
    group.nowClockTime = 1;

    audioSink->lagDetector_.Reset();
    audioSink->lagDetector_.UpdateDrainTimeGroup(group);

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    bool result = audioSink->lagDetector_.CalcLag(avBuffer);
    ASSERT_EQ(result, false);
}

/*
 * @tc.name Test CalcLag API
 * @tc.number CalcLag_003
 * @tc.desc Test drainTimeDiff > DRAIN_TIME_DIFF_WARN_MS
 */
HWTEST(TestAudioSink, CalcLag_003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AudioSink::AudioLagDetector::AudioDrainTimeGroup group;
    group.lastAnchorPts = 1;
    group.anchorDuration = 1;
    group.writeDuration = -50;
    group.nowClockTime = 20;

    audioSink->lagDetector_.Reset();
    audioSink->lagDetector_.UpdateDrainTimeGroup(group);

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    bool result = audioSink->lagDetector_.CalcLag(avBuffer);
    ASSERT_EQ(result, true);
}

/*
 * @tc.name Test CalcLag API
 * @tc.number CalcLag_004
 * @tc.desc Test drainTimeDiff > 20
 */
HWTEST(TestAudioSink, CalcLag_004, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AudioSink::AudioLagDetector::AudioDrainTimeGroup group;
    group.lastAnchorPts = 1;
    group.anchorDuration = 1;
    group.writeDuration = -30;
    group.nowClockTime = 20;

    audioSink->lagDetector_.Reset();
    audioSink->lagDetector_.UpdateDrainTimeGroup(group);

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    bool result = audioSink->lagDetector_.CalcLag(avBuffer);
    ASSERT_EQ(result, true);
}

/*
 * @tc.name Test CalcLag API
 * @tc.number CalcLag_005
 * @tc.desc Test drainTimeDiff < 20
 */
HWTEST(TestAudioSink, CalcLag_005, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AudioSink::AudioLagDetector::AudioDrainTimeGroup group;
    group.lastAnchorPts = 1;
    group.anchorDuration = 1;
    group.writeDuration = 0;
    group.nowClockTime = 20;

    audioSink->lagDetector_.Reset();
    audioSink->lagDetector_.UpdateDrainTimeGroup(group);

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    bool result = audioSink->lagDetector_.CalcLag(avBuffer);
    ASSERT_EQ(result, true);
}

/*
 * @tc.name Test GetSyncCenterClockTime API
 * @tc.number GetSyncCenterClockTime_001
 * @tc.desc Test syncCenter == nullpt
 */
HWTEST(TestAudioSink, GetSyncCenterClockTime_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    int64_t clockTime = 1;
    audioSink->SetSyncCenter(nullptr);

    bool result = audioSink->GetSyncCenterClockTime(clockTime);
    ASSERT_EQ(result, false);
}

/*
 * @tc.name Test GetSyncCenterClockTime API
 * @tc.number GetSyncCenterClockTime_002
 * @tc.desc Test syncCenter != nullpt
 */
HWTEST(TestAudioSink, GetSyncCenterClockTime_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    int64_t clockTime = 1;
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    audioSink->SetSyncCenter(syncCenter);

    bool result = audioSink->GetSyncCenterClockTime(clockTime);
    ASSERT_EQ(result, true);
}
} // namespace Test
} // namespace Media
} // namespace OHOS
