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

#include "audio_sink_unit_test.h"

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
} // namespace Test
} // namespace Media
} // namespace OHOS
