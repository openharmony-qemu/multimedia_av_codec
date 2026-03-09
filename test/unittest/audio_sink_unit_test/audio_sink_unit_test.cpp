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

#include <limits>
#include <vector>
#include "audio_sink_unit_test.h"
#include "plugin/plugin_manager_v2.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {
constexpr const int32_t INVALID_NUM = -3;
constexpr const int32_t TEST_NUM = 12;
constexpr const int32_t LENGTH_NUM = 128;
constexpr const int32_t VALUE1_NUM = 9;
constexpr const int32_t VALUE2_NUM = -50;
constexpr const int32_t VALUE3_NUM = -30;
constexpr const int32_t TIME_NUM = 20;
constexpr const int32_t BOOT_APP_UID_TEST = 1003;
constexpr const int32_t NON_BOOT_APP_UID_TEST = 0;
constexpr const int32_t EOS_WAIT_TIME_MS = 10;
constexpr const int32_t AVAIL_DATA_SIZE_FACTOR = 2;
constexpr const int32_t SAMPLE_8_BIT_BYTES = 1;
constexpr const int32_t SAMPLE_24_BIT_BYTES = 3;
constexpr const int32_t SAMPLE_32_BIT_BYTES = 4;
constexpr const int32_t SAMPLE_BYTES_UNKNOWN = 0;
constexpr const char TEST_THREAD_GROUP_ID[] = "AudioSinkUnitTestGroup";
constexpr const char INVALID_AUDIO_SINK_MIME[] = "audio/not_exist_sink";
constexpr const int64_t LARGE_DURATION_TEST = std::numeric_limits<int64_t>::max() / 4;
constexpr const int64_t TEST_WRITE_DURATION_US = 50000;
constexpr const int64_t TIME_OFFSET_US = 100000;
constexpr const int32_t SAMPLE_RATE_TEST = 48000;

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
class TestAudioSinkResumeMock : public TestAudioSinkUnitMock {
public:
    explicit TestAudioSinkResumeMock(std::string name) : TestAudioSinkUnitMock(std::move(name)) {}

    Status Resume() override
    {
        return Status::OK;
    }
};
class TestAudioSinkGetDurationMock : public TestAudioSinkUnitMock {
public:
    explicit TestAudioSinkGetDurationMock(std::string name) : TestAudioSinkUnitMock(std::move(name)) {}

    Status GetParameter(std::shared_ptr<Meta>& meta) override
    {
        if (meta == nullptr) {
            meta = std::make_shared<Meta>();
        }
        meta->SetData(Tag::AUDIO_SAMPLE_RATE, SAMPLE_RATE_TEST);
        return Status::OK;
    }
};

/**
 * @tc.name Test CopyDataToBufferDesc API
 * @tc.number CopyDataToBufferDesc_001
 * @tc.desc Test swapOutputBuffers_.empty()
 */
HWTEST(TestAudioSink, CopyDataToBufferDesc_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AudioStandard::BufferDesc bufferDesc;

    AVBufferConfig config;
    config.size = VALUE1_NUM;
    config.capacity = VALUE1_NUM;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(config);
    MessageParcel parcel;
    avBuffer->memory_ = AVMemory::CreateAVMemory(parcel, true);
    avBuffer->flag_ = BUFFER_FLAG_EOS;
    audioSink->swapOutputBuffers_.push(avBuffer);
    audioSink->availOutputBuffers_.push(avBuffer);
    audioSink->plugin_ = nullptr;

    size_t size = LENGTH_NUM;
    bool isAudioVividsize = true;
    bool result = audioSink->CopyDataToBufferDesc(size, isAudioVividsize, bufferDesc);
    ASSERT_EQ(result, false);
}

/*
 * @tc.name Test WriteDataToRender API
 * @tc.number WriteDataToRender_001
 * @tc.desc Test IsEosBuffer(filledOutputBuffer)
*/
HWTEST(TestAudioSink, WriteDataToRender_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AVBufferConfig config;
    config.size = VALUE1_NUM;
    config.capacity = VALUE1_NUM;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(config);
    audioSink->isApe_ = false;
    avBuffer->flag_ = BUFFER_FLAG_EOS;

    audioSink->WriteDataToRender(avBuffer);
    ASSERT_EQ(audioSink->isApe_, false);
}

/*
 * @tc.name Test WriteDataToRender API
 * @tc.number WriteDataToRender_002
 * @tc.desc Test calMaxAmplitudeCbStatus_ == false
 */
HWTEST(TestAudioSink, WriteDataToRender_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    AVBufferConfig config;
    config.size = VALUE1_NUM;
    config.capacity = VALUE1_NUM;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(config);
    audioSink->isApe_ = false;
    avBuffer->flag_ = 2;
    audioSink->playRangeEndTime_ = INVALID_NUM;

    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;
    audioSink->calMaxAmplitudeCbStatus_ = false;

    audioSink->WriteDataToRender(avBuffer);
    ASSERT_EQ(audioSink->calMaxAmplitudeCbStatus_, false);
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
    int64_t clkTimeTemp = VALUE1_NUM;

    std::shared_ptr<Pipeline::EventReceiver> testEventReceiver = nullptr;
    audioSink->underrunDetector_.SetEventReceiver(testEventReceiver);

    audioSink->underrunDetector_.DetectAudioUnderrun(clkTimeTemp, latency);
    int64_t underrunTimeUs = clkTime - audioSink->underrunDetector_.lastClkTime_ -
        (audioSink->underrunDetector_.lastLatency_ + audioSink->underrunDetector_.lastBufferDuration_);
    EXPECT_GE(underrunTimeUs, 0);
    EXPECT_EQ(testEventReceiver, nullptr);
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
    group.nowClockTime = VALUE1_NUM;

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
    group.writeDuration = VALUE2_NUM;
    group.nowClockTime = TIME_NUM;

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
    group.writeDuration = VALUE3_NUM;
    group.nowClockTime = TIME_NUM;

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
    group.nowClockTime = TIME_NUM;

    audioSink->lagDetector_.Reset();
    audioSink->lagDetector_.UpdateDrainTimeGroup(group);

    std::shared_ptr<AVBuffer> avBuffer = nullptr;

    bool result = audioSink->lagDetector_.CalcLag(avBuffer);
    ASSERT_EQ(result, true);
}

/*
 * @tc.name Test HandleAudioRenderRequestPost API
 * @tc.number HandleAudioRenderRequestPost_001
 * @tc.desc appUid_ != BOOT_APP_UID, isEosBuffer_ = true, queue non-empty → outer if not entered, normal EOS handling
 */
HWTEST(TestAudioSink, HandleAudioRenderRequestPost_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = NON_BOOT_APP_UID_TEST;
    audioSink->isEosBuffer_ = true;
    AVBufferConfig config;
    config.size = VALUE1_NUM;
    config.capacity = VALUE1_NUM;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> eosBuffer = AVBuffer::CreateAVBuffer(config);
    eosBuffer->flag_ = BUFFER_FLAG_EOS;
    audioSink->availOutputBuffers_.push(eosBuffer);
    audioSink->HandleAudioRenderRequestPost();
    EXPECT_FALSE(audioSink->hangeOnEosCb_);
}

/*
 * @tc.name Test HandleAudioRenderRequestPost API
 * @tc.number HandleAudioRenderRequestPost_002
 * @tc.desc appUid_ == BOOT_APP_UID, isEosBuffer_ = false → outer if short-circuits on isEosBuffer_, early return
 */
HWTEST(TestAudioSink, HandleAudioRenderRequestPost_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = BOOT_APP_UID_TEST;
    audioSink->isEosBuffer_ = false;
    AVBufferConfig config;
    config.size = VALUE1_NUM;
    config.capacity = VALUE1_NUM;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    audioSink->availOutputBuffers_.push(buffer);
    audioSink->HandleAudioRenderRequestPost();
    EXPECT_FALSE(audioSink->availOutputBuffers_.empty());
}

/*
 * @tc.name Test HandleAudioRenderRequestPost API
 * @tc.number HandleAudioRenderRequestPost_003
 * @tc.desc appUid_ == BOOT_APP_UID, isEosBuffer_ = true, queue non-empty → outer if not entered on empty() false
 */
HWTEST(TestAudioSink, HandleAudioRenderRequestPost_003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = BOOT_APP_UID_TEST;
    audioSink->isEosBuffer_ = true;
    AVBufferConfig config;
    config.size = VALUE1_NUM;
    config.capacity = VALUE1_NUM;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> eosBuffer = AVBuffer::CreateAVBuffer(config);
    eosBuffer->flag_ = BUFFER_FLAG_EOS;
    audioSink->availOutputBuffers_.push(eosBuffer);
    audioSink->HandleAudioRenderRequestPost();
    EXPECT_FALSE(audioSink->hangeOnEosCb_);
}

/*
 * @tc.name Test HandleAudioRenderRequestPost API
 * @tc.number HandleAudioRenderRequestPost_004
 * @tc.desc appUid_ == BOOT_APP_UID, isEosBuffer_ = true,
 *          queue empty → enter wait_for branch and then return on empty queue
 */
HWTEST(TestAudioSink, HandleAudioRenderRequestPost_004, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = BOOT_APP_UID_TEST;
    audioSink->isEosBuffer_ = true;
    std::thread notifier([audioSink]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(EOS_WAIT_TIME_MS));
        {
            std::unique_lock<std::mutex> eosLock(audioSink->eosCbMutex_);
            audioSink->hangeOnEosCb_ = false;
        }
        audioSink->eosCbCond_.notify_all();
    });
    audioSink->HandleAudioRenderRequestPost();
    if (notifier.joinable()) {
        notifier.join();
    }
    EXPECT_TRUE(audioSink->availOutputBuffers_.empty());
}

/*
 * @tc.name Test Pause API
 * @tc.number Pause_Boot_EosTaskNull_001
 * @tc.desc appUid_ == BOOT_APP_UID, eosTask_ == nullptr
 */
HWTEST(TestAudioSink, Pause_Boot_EosTaskNull_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = BOOT_APP_UID_TEST;
    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;

    Status ret = audioSink->Pause();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(audioSink->state_.load(), Pipeline::FilterState::PAUSED);
}

/*
 * @tc.name Test Pause API
 * @tc.number Pause_Boot_EosTaskNotNull_002
 * @tc.desc appUid_ == BOOT_APP_UID, eosTask_ != nullptr
 */
HWTEST(TestAudioSink, Pause_Boot_EosTaskNotNull_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = BOOT_APP_UID_TEST;
    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;
    audioSink->SetThreadGroupId(TEST_THREAD_GROUP_ID);

    Status ret = audioSink->Pause();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(audioSink->state_.load(), Pipeline::FilterState::PAUSED);
}

/*
 * @tc.name Test Pause API
 * @tc.number Pause_NonBoot_Transient_003
 * @tc.desc appUid_ != BOOT_APP_UID, isTransitent_ = true
 */
HWTEST(TestAudioSink, Pause_NonBoot_Transient_003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = NON_BOOT_APP_UID_TEST;
    audioSink->isTransitent_ = true;
    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;

    Status ret = audioSink->Pause();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/*
 * @tc.name Test Pause API
 * @tc.number Pause_NonBoot_Normal_004
 * @tc.desc appUid_ != BOOT_APP_UID, isTransitent_ = false && !(isEos_ && isLoop_)
 */
HWTEST(TestAudioSink, Pause_NonBoot_Normal_004, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->appUid_ = NON_BOOT_APP_UID_TEST;
    audioSink->isTransitent_ = false;
    audioSink->isEos_ = false;
    audioSink->isLoop_ = false;
    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;

    Status ret = audioSink->Pause();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/*
 * @tc.name Test Resume API
 * @tc.number Resume_EosTaskSubmit_001
 * @tc.desc eosInterruptType_ == PAUSE, !eosDraining_ && eosTask_ != nullptr → SubmitJobOnce branch
 */
HWTEST(TestAudioSink, Resume_EosTaskSubmit_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkResumeMock> plugin = std::make_shared<TestAudioSinkResumeMock>("test");
    audioSink->plugin_ = plugin;
    audioSink->eosInterruptType_ = AudioSink::EosInterruptState::PAUSE;
    audioSink->eosDraining_ = false;
    audioSink->SetThreadGroupId(TEST_THREAD_GROUP_ID);

    Status ret = audioSink->Resume();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(audioSink->state_.load(), Pipeline::FilterState::RUNNING);
    EXPECT_EQ(audioSink->eosInterruptType_, AudioSink::EosInterruptState::RESUME);
}

/*
 * @tc.name Test IsInputBufferDataEnough API
 * @tc.number IsInputBufferDataEnough_AudioVivid_001
 * @tc.desc isAudioVivid = true else
 */
HWTEST(TestAudioSink, IsInputBufferDataEnough_AudioVivid_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    int32_t requestSize = LENGTH_NUM;
    audioSink->availDataSize_.store(static_cast<size_t>(LENGTH_NUM * AVAIL_DATA_SIZE_FACTOR));
    audioSink->maxCbDataSize_ = 0;
    bool isAudioVivid = true;
    bool ret = audioSink->IsInputBufferDataEnough(requestSize, isAudioVivid);
    EXPECT_TRUE(ret);
    EXPECT_EQ(audioSink->maxCbDataSize_, requestSize);
}

/*
 * @tc.name Test SetVolumeMode API
 * @tc.number SetVolumeMode_NullPlugin_001
 * @tc.desc plugin_ == nullptr → return Status::ERROR_NULL_POINTER
 */
HWTEST(TestAudioSink, SetVolumeMode_NullPlugin_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    audioSink->plugin_ = nullptr;
    int32_t mode = INVALID_NUM;
    Status ret = audioSink->SetVolumeMode(mode);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/*
 * @tc.name Test CreatePlugin API
 * @tc.number CreatePlugin_Null_001
 * @tc.desc PluginManagerV2::CreatePluginByMime returns nullptr for invalid mime
 */
HWTEST(TestAudioSink, CreatePlugin_Null_001, TestSize.Level1)
{
    auto pluginBase = OHOS::Media::Plugins::PluginManagerV2::Instance().CreatePluginByMime(
        OHOS::Media::Plugins::PluginType::AUDIO_SINK, INVALID_AUDIO_SINK_MIME);
    EXPECT_EQ(pluginBase, nullptr);
}

/*
 * @tc.name Test UpdateAudioWriteTimeMayWait API
 * @tc.number UpdateAudioWriteTimeMayWait_Clamp_001
 * @tc.desc latestBufferDuration_ > MAX_BUFFER_DURATION_US → clamped to smaller value
 */
HWTEST(TestAudioSink, UpdateAudioWriteTimeMayWait_Clamp_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->latestBufferDuration_ = LARGE_DURATION_TEST;
    audioSink->lastBufferWriteSuccess_ = true;

    audioSink->UpdateAudioWriteTimeMayWait();

    EXPECT_GT(audioSink->latestBufferDuration_, 0);
    EXPECT_LT(audioSink->latestBufferDuration_, LARGE_DURATION_TEST);
}

/*
 * @tc.name Test UpdateAudioWriteTimeMayWait API
 * @tc.number UpdateAudioWriteTimeMayWait_Sleep_002
 * @tc.desc lastBufferWriteSuccess_ = false, writeSleepTime > 0 → enter usleep branch
 */
HWTEST(TestAudioSink, UpdateAudioWriteTimeMayWait_Sleep_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->latestBufferDuration_ = TEST_WRITE_DURATION_US;
    audioSink->lastBufferWriteSuccess_ = false;

    int64_t nowUs = Plugins::HstTime2Us(SteadyClock::GetCurrentTimeNanoSec());
    int64_t initialLastTime = nowUs + TIME_OFFSET_US;
    audioSink->lastBufferWriteTime_ = initialLastTime;

    audioSink->UpdateAudioWriteTimeMayWait();

    EXPECT_GT(audioSink->lastBufferWriteTime_, initialLastTime);
}

/*
 * @tc.name Test UpdateAudioWriteTimeMayWait API
 * @tc.number UpdateAudioWriteTimeMayWait_NoSleep_003
 * @tc.desc lastBufferWriteSuccess_ = false, writeSleepTime <= 0 → skip usleep branch
 */
HWTEST(TestAudioSink, UpdateAudioWriteTimeMayWait_NoSleep_003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->latestBufferDuration_ = TEST_WRITE_DURATION_US;
    audioSink->lastBufferWriteSuccess_ = false;

    int64_t nowUs = Plugins::HstTime2Us(SteadyClock::GetCurrentTimeNanoSec());
    audioSink->lastBufferWriteTime_ = nowUs - TIME_OFFSET_US;

    audioSink->UpdateAudioWriteTimeMayWait();

    EXPECT_GE(audioSink->lastBufferWriteTime_, nowUs);
}

/*
 * @tc.name Test GetSampleFormatBytes API
 * @tc.number GetSampleFormatBytes_SampleU8_001
 * @tc.desc AudioSampleFormat::SAMPLE_U8 -> AUDIO_SAMPLE_8_BIT
 */
HWTEST(TestAudioSink, GetSampleFormatBytes_SampleU8_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;
    plugin->SetSampleFormat(AudioSampleFormat::SAMPLE_U8);

    int32_t formatBytes = audioSink->GetSampleFormatBytes();
    EXPECT_EQ(formatBytes, SAMPLE_8_BIT_BYTES);
}

/*
 * @tc.name Test GetSampleFormatBytes API
 * @tc.number GetSampleFormatBytes_SampleS24LE_002
 * @tc.desc AudioSampleFormat::SAMPLE_S24LE -> AUDIO_SAMPLE_24_BIT
 */
HWTEST(TestAudioSink, GetSampleFormatBytes_SampleS24LE_002, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;
    plugin->SetSampleFormat(AudioSampleFormat::SAMPLE_S24LE);

    int32_t formatBytes = audioSink->GetSampleFormatBytes();
    EXPECT_EQ(formatBytes, SAMPLE_24_BIT_BYTES);
}

/*
 * @tc.name Test GetSampleFormatBytes API
 * @tc.number GetSampleFormatBytes_SampleS32LE_003
 * @tc.desc AudioSampleFormat::SAMPLE_S32LE -> AUDIO_SAMPLE_32_BIT
 */
HWTEST(TestAudioSink, GetSampleFormatBytes_SampleS32LE_003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;
    plugin->SetSampleFormat(AudioSampleFormat::SAMPLE_S32LE);

    int32_t formatBytes = audioSink->GetSampleFormatBytes();
    EXPECT_EQ(formatBytes, SAMPLE_32_BIT_BYTES);
}

/*
 * @tc.name Test GetSampleFormatBytes API
 * @tc.number GetSampleFormatBytes_Default_004
 * @tc.desc default branch (e.g. AudioSampleFormat::INVALID_WIDTH) -> 0
 */
HWTEST(TestAudioSink, GetSampleFormatBytes_Default_004, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;
    plugin->SetSampleFormat(AudioSampleFormat::INVALID_WIDTH);

    int32_t formatBytes = audioSink->GetSampleFormatBytes();
    EXPECT_EQ(formatBytes, SAMPLE_BYTES_UNKNOWN);
}

/*
 * @tc.name Test getDurationUsPlayedAtSampleRate API
 * @tc.number GetDurationUsPlayedAtSampleRate_001
 * @tc.desc parameter != nullptr, sampleRate != 0 -> return computed duration branch
 */
HWTEST(TestAudioSink, GetDurationUsPlayedAtSampleRate_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    std::shared_ptr<TestAudioSinkGetDurationMock> plugin =
        std::make_shared<TestAudioSinkGetDurationMock>("test");
    audioSink->plugin_ = plugin;

    uint32_t numFrames = 480;
    int64_t durationUs = audioSink->getDurationUsPlayedAtSampleRate(numFrames);
    int64_t expectedUs = static_cast<int64_t>(static_cast<int32_t>(numFrames) * HST_MSECOND / SAMPLE_RATE_TEST);
    EXPECT_EQ(durationUs, expectedUs);
}

/*
 * @tc.name Test ResetInfo API
 * @tc.number ResetInfo_BootAppUid_NotifyEosCb_001
 * @tc.desc appUid_ == BOOT_APP_UID -> enter eosCb branch and clear hangeOnEosCb_
 */
HWTEST(TestAudioSink, ResetInfo_BootAppUid_NotifyEosCb_001, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);

    audioSink->appUid_ = BOOT_APP_UID_TEST;
    {
        std::unique_lock<std::mutex> eosCbLock(audioSink->eosCbMutex_);
        audioSink->hangeOnEosCb_ = true;
    }
    audioSink->ResetInfo();
    EXPECT_FALSE(audioSink->hangeOnEosCb_);
}

/*
 * @tc.name Test WriteDataToRender API
 * @tc.number WriteDataToRender_CalcMaxAmplitude_003
 * @tc.desc calMaxAmplitudeCbStatus_ == true -> CalcMaxAmplitude and UpdateAmplitude branch
 */
HWTEST(TestAudioSink, WriteDataToRender_CalcMaxAmplitude_003, TestSize.Level1)
{
    std::shared_ptr<AudioSink> audioSink = AudioSinkUnitCreate();
    ASSERT_TRUE(audioSink != nullptr);
    AVBufferConfig config;
    config.size = VALUE1_NUM;
    config.capacity = VALUE1_NUM;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(config);
    audioSink->isApe_ = false;
    avBuffer->flag_ = 0;
    audioSink->playRangeEndTime_ = INVALID_NUM;
    std::shared_ptr<TestAudioSinkUnitMock> plugin = std::make_shared<TestAudioSinkUnitMock>("test");
    audioSink->plugin_ = plugin;
    audioSink->calMaxAmplitudeCbStatus_ = true;
    audioSink->WriteDataToRender(avBuffer);
    EXPECT_TRUE(audioSink->calMaxAmplitudeCbStatus_);
}

} // namespace Test
} // namespace Media
} // namespace OHOS
