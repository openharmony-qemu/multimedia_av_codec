/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <unistd.h>
#include "avcodec_info.h"
#include "avcodec_log.h"
#include "dump_usage.h"
#include "hisysevent_manager_c.h"
#include "hisysevent_record_c.h"
#include "instance_info.h"
#include "ret_code.h"
#include "statistics_event_handler.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing;
using namespace testing::ext;

namespace {
constexpr int32_t QUERY_INTERVAL_TIME = 2;
// constexpr int32_t QUERY_MAX_EVENTS = 100000;
const char * TEST_DOMAIN = "AV_CODEC";
// constexpr int32_t MAX_LEN_OF_EVENT_DOMAIN = 16;
// constexpr int32_t MAX_LEN_OF_EVENT_NAME = 64;
// constexpr int32_t DEFAULT_COUNT = 100000;
// constexpr int32_t SPECIFICATIONS_INFO_THRESHOLD = 50;
// constexpr int32_t BEHAVIORS_INFO_THRESHOLD = 200;
// const std::vector<std::string> FORMAT_COMPONENTS = {
//     "mpeg",
//     "avc",
//     "hevc",
//     "vp",
//     "av1",
//     "divx",
//     "xvid",
//     "flash",
//     "quicktime",
//     "real",
//     "windowsmedia",
//     "ogg"
//     "web",
//     "stream",
//     "media",
//     "digital",
//     "high",
//     "ultra",
// };

// const std::vector<std::string> SUFFIXES = {
//     "video",
//     "media",
//     "stream",
//     "format",
//     "container",
//     "codec",
//     "mpeg",
//     "av",
//     "mv",
//     "film",
// };

// int32_t getRandomIndex(int32_t min, int32_t max)
// {
//     std::random_device rd;
//     std::mt19937 gen(rd());
//     std::uniform_int_distribution<int> dist(min, max);
//     return dist(gen);
// }

// std::string generateRandomMime()
// {
//     std::string mimeType("");
//     int32_t switchIndex = getRandomIndex(0, 1);
//     switch (switchIndex) {
//         case 0:
//             // video/custom-format
//             mimeType = "video/" + FORMAT_COMPONENTS[getRandomIndex(0, FORMAT_COMPONENTS.size() - 1)] +
//                        SUFFIXES[getRandomIndex(0, SUFFIXES.size() - 1)];
//             break;
//         case 1:
//             // video/x-custom-format
//             mimeType = "video/x-" + FORMAT_COMPONENTS[getRandomIndex(0, FORMAT_COMPONENTS.size() - 1)] +
//                        SUFFIXES[getRandomIndex(0, SUFFIXES.size() - 1)];
//             break;
//         default:
//             break;
//     }
//     return mimeType;
// }

std::string generateRandomString(size_t length, std::string_view charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                                           "abcdefghijklmnopqrstuvwxyz"
                                                                           "0123456789")
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}

class DfxStatisticsEventTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<Media::Meta> meta_ = nullptr;
    HiSysEventWatcher watcher_{};
    HiviewDFX::DumpUsage dumpUsage_;
    static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, STRINGFY(DfxStatisticsEventTest)};
};

void DfxStatisticsEventTest::SetUpTestCase(void) {}

void DfxStatisticsEventTest::TearDownTestCase(void) {}

void DfxStatisticsEventTest::SetUp(void)
{
    pid_t pid = getpid();
    std::cout << "start memory = " << dumpUsage_.GetPss(pid) << std::endl;

    meta_ = std::make_shared<Media::Meta>();
    ASSERT_NE(nullptr, meta_);

    sleep(QUERY_INTERVAL_TIME);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void DfxStatisticsEventTest::TearDown(void)
{
    pid_t pid = getpid();
    std::cout << "end memory = " << dumpUsage_.GetPss(pid) << std::endl;
}

void OnEventTest(HiSysEventRecordC record)
{
    ASSERT_GT(strlen(record.jsonStr), 0);
    std::cout << "OnEvent: event = " << record.jsonStr << std::endl;
}

void OnServiceDiedTest()
{
    std::cout << "OnServiceDied" << std::endl;
}

void InitWatcher(HiSysEventWatcher& watcher)
{
    watcher.OnEvent = OnEventTest;
    watcher.OnServiceDied = OnServiceDiedTest;
}

template<size_t N>
void CheckHiSysEventWatcher(HiSysEventWatcher watcher, const char* name, HiSysEventWatchRule (&rules)[N])
{
    auto ret = OH_HiSysEvent_Add_Watcher(&watcher, rules, N);
    ASSERT_EQ(ret, HiviewDFX::IPC_CALL_SUCCEED);
    ret = OH_HiSysEvent_Write(TEST_DOMAIN, name, HISYSEVENT_BEHAVIOR, nullptr, 0);
    ASSERT_EQ(ret, HiviewDFX::IPC_CALL_SUCCEED);
    sleep(QUERY_INTERVAL_TIME);
    ret = OH_HiSysEvent_Remove_Watcher(&watcher);
    ASSERT_EQ(ret, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_001
 * @tc.desc: eventType is BASIC_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_001, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_002
 * @tc.desc: eventType is BASIC_QUERY_CAP_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_002, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"BASIC_INFO", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_003
 * @tc.desc: eventType is BASIC_CREATE_CODEC_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_003, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_004
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is invalid, codecType is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_004, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string mime = generateRandomString(10); // 10: length
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_005
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is invalid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_005, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string mime = generateRandomString(10); // 10: length
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1);
    meta_->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_006
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is invalid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_006, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string mime = generateRandomString(10); // 10: length
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    meta_->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_007
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is valid, codecType is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_007, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_008
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is valid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_008, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_009
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is valid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_009, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_010
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is empty, codecType is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_010, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_011
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is empty, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_011, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_012
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is empty, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_012, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "BASIC_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "BASIC_INFO", rules);
}
} // namespace