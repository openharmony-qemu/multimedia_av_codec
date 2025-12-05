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
#include "native_averrors.h"
#include "ret_code.h"
#include "statistics_event_handler.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing;
using namespace testing::ext;

namespace {
constexpr int32_t QUERY_INTERVAL_TIME = 2;
constexpr int32_t MAX_EVENT_ADD_COUNT = 100000;
constexpr int32_t BEHAVIORSINFO_EVENT_ADD_COUNT = 200;
constexpr int32_t ELAPSEDTIME_THREADSHOLD = 600;
const char *TEST_DOMAIN = "AV_CODEC";
enum class RANDOM_MIME_TYPE : int32_t {
    VALID_VIDEO,
    VALID_AUDIO,
    INVALID_CHAR_1,
    INVALID_CHAR_2,
    INVALID_CHAR_3,
    INVALID_CHAR_4,
    INVALID_CHAR_5,
    INVALID_SEPARATOR_POS_1,
    INVALID_SEPARATOR_POS_2,
    MIN_CHAR_LENGTH,
    MAX_CHAR_LENGTH,
};
const std::vector<std::string> FORMAT_COMPONENTS = {
    "mpeg", "avc",          "hevc", "vp",  "av1",    "divx",  "xvid",    "flash", "quicktime",
    "real", "windowsmedia", "ogg",  "web", "stream", "media", "digital", "high",  "ultra",
};

std::string GenerateRandomString(size_t length, std::string_view charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
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

int32_t GetRandomNum(int32_t min, int32_t max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

std::string GenerateRandomMime()
{
    std::string mimeType("");
    int32_t switchIndex = GetRandomNum(0, 10); // 10: max length
    switch (switchIndex) {
        case RANDOM_MIME_TYPE::VALID_VIDEO:
            mimeType = "video/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::VALID_AUDIO:
            mimeType = "audio/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::INVALID_CHAR_1:
            mimeType = "video/-" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::INVALID_CHAR_2:
            mimeType = "video/+" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::INVALID_CHAR_3:
            mimeType = "video//" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::INVALID_CHAR_4:
            mimeType = "video/." + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::INVALID_CHAR_5:
            mimeType = "video/*" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::INVALID_SEPARATOR_POS_1:
            mimeType = "/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case RANDOM_MIME_TYPE::INVALID_SEPARATOR_POS_2:
            mimeType = FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)] + "/";
            break;
        case RANDOM_MIME_TYPE::MIN_CHAR_LENGTH:
            mimeType = GenerateRandomString(200); // 200: more than mime type length
            break;
        case RANDOM_MIME_TYPE::MAX_CHAR_LENGTH:
            mimeType = GenerateRandomString(5); // 5: less than mime type length
            break;
        default:
            break;
    }
    return mimeType;
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

void InitWatcher(HiSysEventWatcher &watcher)
{
    watcher.OnEvent = OnEventTest;
    watcher.OnServiceDied = OnServiceDiedTest;
}

template <size_t N>
void CheckHiSysEventWatcher(HiSysEventWatcher watcher, const char *name, HiSysEventWatchRule (&rules)[N])
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
    std::string mime = GenerateRandomString(10); // 10: length
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
    std::string mime = GenerateRandomString(10); // 10: length
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
    std::string mime = GenerateRandomString(10); // 10: length
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

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_001
 * @tc.desc: 1. eventType is APP_SPECIFICATIONS_INFO
 *           2. mimeType is video/avc, codecType decoder hardware
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_001, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_SPECIFICATIONS_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

void AddSpecificationInfoEvent(std::shared_ptr<Media::Meta> meta, std::string mime)
{
    meta->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_SPECIFICATIONS_INFO, *meta);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_002
 * @tc.desc: 1. eventType is APP_SPECIFICATIONS_INFO
 *           2. mimeType is ivalid, codecType decoder hardware
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_002, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string mime = "video/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = "audio/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = "video/-" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = "video/+" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = "video//" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = "video/." + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = "video/*" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = "/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
    AddSpecificationInfoEvent(meta_, mime);
    mime = FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)] + "/";
    AddSpecificationInfoEvent(meta_, mime);
    mime = GenerateRandomString(200); // 200: more than mime type length
    AddSpecificationInfoEvent(meta_, mime);
    mime = GenerateRandomString(50); // 50: less than mime type length
    AddSpecificationInfoEvent(meta_, mime);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_003
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_INFO
 *           2. mimeType is video/avc, codecType decoder hardware
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_003, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_004
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is video/avc, codecType decoder hardware
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_004, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_005
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is empty, codecType decoder hardware
 *           3. event file is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_005, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_006
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is video/avc, codecType encoder hardware
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_006, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_007
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is empty, codecType encoder hardware
 *           3. event file is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_007, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_008
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is random, codecType decoder hardware, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_008, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_009
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is random, codecType encoder hardware, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_009, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_010
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType is video/avc, codecType decoder hardware
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_010, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_011
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType is empty, codecType decoder hardware
 *           3. event file is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_011, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_012
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType is video/avc, codecType encoder hardware
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_012, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_013
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType is empty, codecType encoder hardware
 *           3. event file is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_013, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_014
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType is random, codecType decoder hardware, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_014, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO,
                                                          *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_015
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is random, codecType encoder hardware, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_015, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_SPECIFICATIONS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO,
                                                          *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_SPECIFICATIONS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_001
 * @tc.desc: 1. eventType is APP_BEHAVIORS_INFO
 *           2. forward process and caller process exist
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_001, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string forwardProcessName = "forwardProcess";
    std::string callerProcessName = "callerProcess";
    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_002
 * @tc.desc: 1. eventType is APP_BEHAVIORS_INFO
 *           2. forward process exist and caller process exist't
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_002, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string forwardProcessName = "forwardProcess";
    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_003
 * @tc.desc: 1. eventType is APP_BEHAVIORS_INFO
 *           2. forward process exit't and caller process exist
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_003, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string callerProcessName = "callerProcess";
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_004
 * @tc.desc: 1. eventType is APP_BEHAVIORS_INFO
 *           2. forward process and caller process exist't
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_004, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_005
 * @tc.desc: 1. eventType is APP_BEHAVIORS_INFO
 *           2. caller process name out of length
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_005, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string callerProcessName = GenerateRandomString(200); // 200: clller name length
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_006
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO
 *           2. caller process name is same, excute 200 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_006, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int32_t i = 0; i < BEHAVIORSINFO_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = "callerProcess";
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_007
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO
 *           2. caller process name is different, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_007, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int32_t i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = GenerateRandomString(20); // 20: clller name length
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_008
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process exist, elapsed time exist't
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_008, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string callerProcessName = "callerProcess";
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO,
                                                      *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_009
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process exist, elapsed time less than threadshold
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_009, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string callerProcessName = "callerProcess";
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD / 2);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO,
                                                      *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_010
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process exist, elapsed time more than threadshold
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_010, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    std::string callerProcessName = "callerProcess";
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD * 2);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO,
                                                      *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_008
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process name is same, elapsed time less than threadshold, excute 200 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_008, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int32_t i = 0; i < BEHAVIORSINFO_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = "callerProcess";
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD / 2);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_009
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process name is different, elapsed time less than threadshold, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_009, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int32_t i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = GenerateRandomString(20); // 20: clller name length
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD / 2);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_009
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process name is different, elapsed time less than threadshold, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_009, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "APP_BEHAVIORS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    for (int32_t i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = GenerateRandomString(20); // 20: clller name length
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD / 2);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "APP_BEHAVIORS_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_001
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is empty, codec type is valid, error type is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_001, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_002
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is empty, codec type is invalid, error type is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_002, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1); // -1: codec type is invalid
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_003
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is empty, codec type is invalid, error type is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_003, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_004
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is empty, codec type is valid, error type is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_004, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), AV_ERR_INVALID_VAL);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_005
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is empty, codec type is invalid, error type is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_005, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1); // -1: codec type is invalid
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), AV_ERR_INVALID_VAL);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_006
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is empty, codec type is invalid, error type is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_006, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), AV_ERR_INVALID_VAL);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_007
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is video/avc, codec type is valid, error type is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_007, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_008
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is video/avc, codec type is invalid, error type is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_008, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1); // -1: codec type is invalid
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_009
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is video/avc, codec type is invalid, error type is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_009, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_010
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is video/avc, codec type is valid, error type is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_010, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), AV_ERR_INVALID_VAL);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_011
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is video/avc, codec type is invalid, error type is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_005, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1); // -1: codec type is invalid
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), AV_ERR_INVALID_VAL);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}

/**
 * @tc.name: AddEventInfo_CodecAbnormalInfo_012
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is video/avc, codec type is invalid, error type is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecAbnormalInfo_012, TestSize.Level1)
{
    InitWatcher(watcher_);
    HiSysEventWatchRule rule = {"AV_CODEC", "CODEC_ABNORMAL_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), AV_ERR_INVALID_VAL);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckHiSysEventWatcher(watcher_, "CODEC_ABNORMAL_INFO", rules);
}
} // namespace