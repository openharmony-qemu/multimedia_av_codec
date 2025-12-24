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
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>
#include "avcodec_info.h"
#include "avcodec_log.h"
#include "cJSON.h"
#include "dump_usage.h"
#include "hisysevent.h"
#include "hisysevent_manager_c.h"
#include "hisysevent_record_c.h"
#include "instance_info.h"
#include "native_averrors.h"
#include "ret_code.h"
#include "statistics_event_handler.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing;
using namespace testing::ext;

namespace {
std::mutex g_mutex;
std::condition_variable g_cv;
constexpr int32_t QUERY_INTERVAL_TIME = 500;
constexpr int32_t STATISTIC_EVENT_INFO_TIMEOUT = 5000;
constexpr int32_t MAX_EVENT_ADD_COUNT = 100000;
constexpr int32_t BEHAVIORSINFO_EVENT_ADD_COUNT = 200;
constexpr int32_t ELAPSEDTIME_THREADSHOLD = 600;
constexpr char TEST_DOMAIN[] = "AV_CODEC";
std::string g_recordJson;
enum class RANDOM_MIME_TYPE : int {
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
    "mpeg",
    "avc",
    "hevc",
    "vp",
    "av1",
    "divx",
    "xvid",
    "flash",
    "quicktime",
    "real",
    "windowsmedia",
    "ogg",
    "web",
    "stream",
    "media",
    "digital",
    "high",
    "ultra",
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
    int switchIndex = static_cast<int>(GetRandomNum(0, 10)); // 10: max length
    switch (switchIndex) {
        case static_cast<int>(RANDOM_MIME_TYPE::VALID_VIDEO):
            mimeType = "video/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::VALID_AUDIO):
            mimeType = "audio/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::INVALID_CHAR_1):
            mimeType = "video/-" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::INVALID_CHAR_2):
            mimeType = "video/+" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::INVALID_CHAR_3):
            mimeType = "video//" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::INVALID_CHAR_4):
            mimeType = "video/." + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::INVALID_CHAR_5):
            mimeType = "video/*" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::INVALID_SEPARATOR_POS_1):
            mimeType = "/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::INVALID_SEPARATOR_POS_2):
            mimeType = FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)] + "/";
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::MIN_CHAR_LENGTH):
            mimeType = GenerateRandomString(200); // 200: more than mime type length
            break;
        case static_cast<int>(RANDOM_MIME_TYPE::MAX_CHAR_LENGTH):
            mimeType = GenerateRandomString(5); // 5: less than mime type length
            break;
        default:
            mimeType = "video/" + FORMAT_COMPONENTS[GetRandomNum(0, FORMAT_COMPONENTS.size() - 1)];
            break;
    }
    return mimeType;
}

void WaitForEventJson()
{
    std::unique_lock<std::mutex> lock(g_mutex);
    std::cv_status status = g_cv.wait_for(lock, std::chrono::milliseconds(STATISTIC_EVENT_INFO_TIMEOUT));
    EXPECT_NE(status, std::cv_status::timeout) << "Expected timeout after " << STATISTIC_EVENT_INFO_TIMEOUT << " ms";
}

void OnEventTest(HiSysEventRecordC record)
{
    ASSERT_GT(strlen(record.jsonStr), 0);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_recordJson = record.jsonStr;
    }
    g_cv.notify_one();
    std::cout << "OnEvent: event = " << record.jsonStr << std::endl;
}

void OnServiceDiedTest()
{
    std::cout << "OnServiceDied" << std::endl;
}

void CheckJsonValue(std::string firstKey, std::string secondKey)
{
    WaitForEventJson();
    std::cout << "first key = " << firstKey << std::endl;
    std::cout << "second key = " << secondKey << std::endl;
    if (!secondKey.empty()) {
        auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
        ASSERT_NE(nullptr, data);
        auto firstValue = cJSON_GetObjectItem(data.get(), firstKey.c_str());
        ASSERT_NE(nullptr, firstValue);
        ASSERT_TRUE(cJSON_IsString(firstValue));
        auto parsedFirstValue = std::shared_ptr<cJSON>(cJSON_Parse(firstValue->valuestring), cJSON_Delete);
        ASSERT_NE(nullptr, parsedFirstValue);
        auto secondValue = cJSON_GetObjectItem(parsedFirstValue.get(), secondKey.c_str());
        ASSERT_NE(nullptr, secondValue);
        ASSERT_NE(nullptr, secondValue->child);
    } else {
        auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
        ASSERT_NE(nullptr, data);
        auto value = cJSON_GetObjectItem(data.get(), firstKey.c_str());
        ASSERT_NE(nullptr, value);
        auto array = std::shared_ptr<cJSON>(cJSON_Parse(value->valuestring), cJSON_Delete);
        ASSERT_NE(nullptr, array);
        ASSERT_LE(1, cJSON_GetArraySize(array.get()));
    }
}

class DfxStatisticsEventTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<Media::Meta> meta_ = nullptr;
    HiviewDFX::DumpUsage dumpUsage_;
    static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, STRINGFY(DfxStatisticsEventTest)};
};

void DfxStatisticsEventTest::SetUpTestCase(void)
{
    HiSysEventWatcher watcher{};
    watcher.OnEvent = OnEventTest;
    watcher.OnServiceDied = OnServiceDiedTest;
    HiSysEventWatchRule rule = {"AV_CODEC", "STATISTICS_INFO", "", 1, 0};
    HiSysEventWatchRule rules[] = {rule};
    auto ret = OH_HiSysEvent_Add_Watcher(&watcher, rules, sizeof(rules) / sizeof(HiSysEventWatchRule));
    ASSERT_EQ(ret, HiviewDFX::IPC_CALL_SUCCEED);
}

void DfxStatisticsEventTest::TearDownTestCase(void) {}

void DfxStatisticsEventTest::SetUp(void)
{
    pid_t pid = getpid();
    std::cout << "start memory = " << dumpUsage_.GetPss(pid) << std::endl;

    g_recordJson.clear();
    meta_ = std::make_shared<Media::Meta>();
    ASSERT_NE(nullptr, meta_);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void DfxStatisticsEventTest::TearDown(void)
{
    StatisticsEventInfo::GetInstance().timer_ = nullptr;
    pid_t pid = getpid();
    std::cout << "end memory = " << dumpUsage_.GetPss(pid) << std::endl;
}

/**
 * @tc.name: RegisterEventHook_Test_001
 * @tc.desc: 1. eventType key in range
 *           2. EventHooker func not exist
 *           3. add EventHooker func
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, RegisterEventHook_Test_001, TestSize.Level1)
{
    bool isRegisterEventHook = false;
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        StatisticsEventType::MAIN_EVENT_TYPE_MASK, [&isRegisterEventHook](const Media::Meta &meta) -> bool {
            HiSysEventWrite(TEST_DOMAIN, "DFX_STATISTICS_EVENT_TEST", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                            "Test", "Success");
            isRegisterEventHook = true;
            return true;
        });
    std::this_thread::sleep_for(std::chrono::milliseconds(QUERY_INTERVAL_TIME));
    ASSERT_NE(true, isRegisterEventHook);
}

/**
 * @tc.name: AddEventInfo_Invalid_Key_001
 * @tc.desc: 1. eventType key out of range
 *           2. EventHooker func not exist
 *           3. add EventHooker func and report
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_Invalid_Key_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(static_cast<StatisticsEventType>(INT32_MAX), *meta_);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(static_cast<StatisticsEventType>(-1), *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_Invalid_Key_002
 * @tc.desc: 1. eventType key out of range
 *           2. EventHooker func exist, needErase is true
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_Invalid_Key_002, TestSize.Level1)
{
    bool isRegisterEventHook = false;
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        static_cast<StatisticsEventType>(INT32_MAX), [&isRegisterEventHook](const Media::Meta &meta) -> bool {
            HiSysEventWrite(TEST_DOMAIN, "DFX_STATISTICS_EVENT_TEST", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                            "Test", "Success");
            isRegisterEventHook = true;
            return true;
        });
    StatisticsEventInfo::GetInstance().OnAddEventInfo(static_cast<StatisticsEventType>(INT32_MAX), *meta_);
    std::this_thread::sleep_for(std::chrono::milliseconds(QUERY_INTERVAL_TIME));
    ASSERT_EQ(true, isRegisterEventHook);
}

/**
 * @tc.name: AddEventInfo_Invalid_Key_003
 * @tc.desc: 1. eventType key out of range
 *           2. EventHooker func exist, needErase is false
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_Invalid_Key_003, TestSize.Level1)
{
    bool isRegisterEventHook = false;
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        static_cast<StatisticsEventType>(INT32_MAX), [&isRegisterEventHook](const Media::Meta &meta) -> bool {
            HiSysEventWrite(TEST_DOMAIN, "DFX_STATISTICS_EVENT_TEST", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                            "Test", "Success");
            isRegisterEventHook = false;
            return false;
        });
    StatisticsEventInfo::GetInstance().OnAddEventInfo(static_cast<StatisticsEventType>(INT32_MAX), *meta_);
    std::this_thread::sleep_for(std::chrono::milliseconds(QUERY_INTERVAL_TIME));
    ASSERT_EQ(true, isRegisterEventHook);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_001
 * @tc.desc: eventType is BASIC_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_BasicQueryCapInfo_001
 * @tc.desc: eventType is BASIC_QUERY_CAP_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicQueryAndCreateInfo_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
    auto item = cJSON_GetObjectItem(data.get(), "QueryCapTimes");
    ASSERT_NE(nullptr, item);
    ASSERT_TRUE(cJSON_IsNumber(item));
    int32_t queryCapTimes = item->valueint;
    ASSERT_EQ(1, queryCapTimes);

    item = cJSON_GetObjectItem(data.get(), "CreateCodecTimes");
    ASSERT_NE(nullptr, item);
    ASSERT_TRUE(cJSON_IsNumber(item));
    int32_t createCodecTimes = item->valueint;
    ASSERT_EQ(1, createCodecTimes);
}

/**
 * @tc.name: AddEventInfo_BasicCreateCodecSpecInfo_001
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is exist, codecType is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicCreateCodecSpecInfo_001, TestSize.Level1)
{
    std::string mime = GenerateRandomString(10); // 10: length
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);

    meta_ = std::make_shared<Media::Meta>();
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckJsonValue("CodecSpecifiedInfo", "");
}

/**
 * @tc.name: AddEventInfo_BasicCreateCodecSpecInfo_002
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType and codecType parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicCreateCodecSpecInfo_002, TestSize.Level1)
{
    std::string mime;
    for (auto codecType : {-1, INT16_MAX, static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE)}) {
        meta_ = std::make_shared<Media::Meta>();
        mime = GenerateRandomString(10); // 10: length
        meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), codecType);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    }

    for (auto codecType : {-1, INT16_MAX, static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE)}) {
        meta_ = std::make_shared<Media::Meta>();
        meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), codecType);
        meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    }

    for (auto codecType : {-1, INT16_MAX, static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE)}) {
        meta_ = std::make_shared<Media::Meta>();
        meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), codecType);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_001
 * @tc.desc: 1. eventType is APP_SPECIFICATIONS_INFO
 *           2. mimeType and isEncoder parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_SPECIFICATIONS_INFO, *meta_);

    meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_SPECIFICATIONS_INFO, *meta_);

    meta_ = std::make_shared<Media::Meta>();
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_SPECIFICATIONS_INFO, *meta_);

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

void AddSpecificationInfoEvent(std::shared_ptr<Media::Meta> meta, std::string mime)
{
    meta->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    meta->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_SPECIFICATIONS_INFO, *meta);
}

/**
 * @tc.name: AddEventInfo_AppSpecificationsInfo_002
 * @tc.desc: 1. eventType is APP_SPECIFICATIONS_INFO
 *           2. mimeType is invalid, isEncoder is false
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppSpecificationsInfo_002, TestSize.Level1)
{
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
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_CapUnsupportedInfo_001
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_INFO
 *           2. mimeType is video/avc, isEncoder is false
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CapUnsupportedInfo_001, TestSize.Level1)
{
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_CapUnsupportedQueryCapInfo_001
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is video/avc, isEncoder parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CapUnsupportedQueryCapInfo_001, TestSize.Level1)
{
    for (auto isEncoder : {false, true}) {
        meta_ = std::make_shared<Media::Meta>();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), isEncoder);
        meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckJsonValue("CapUnsupportedInfo", "QueryCapUnsupportedInfo");
}

/**
 * @tc.name: AddEventInfo_CapUnsupportedQueryCapInfo_002
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is empty, isEncoder parameter test
 *           3. event file is empty
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CapUnsupportedQueryCapInfo_002, TestSize.Level1)
{
    for (auto isEncoder : {false, true}) {
        meta_ = std::make_shared<Media::Meta>();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), isEncoder);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_CapUnsupportedQueryCapInfo_003
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_QUERY_CAP_INFO
 *           2. mimeType is random, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CapUnsupportedQueryCapInfo_003, TestSize.Level1)
{
    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    }

    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_CapUnsupportedCreateCapInfo_001
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType and isEncoder parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CapUnsupportedCreateCapInfo_001, TestSize.Level1)
{
    for (auto isEncoder : {false, true}) {
        meta_ = std::make_shared<Media::Meta>();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), isEncoder);
        meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO,
                                                          *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckJsonValue("CapUnsupportedInfo", "CreateCodecUnsupportedInfo");
}

/**
 * @tc.name: AddEventInfo_CapUnsupportedCreateCapInfo_002
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType is empty, isEncoder parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CapUnsupportedCreateCapInfo_002, TestSize.Level1)
{
    for (auto isEncoder : {false, true}) {
        meta_ = std::make_shared<Media::Meta>();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), isEncoder);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO,
                                                          *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_CapUnsupportedCreateCapInfo_003
 * @tc.desc: 1. eventType is CAP_UNSUPPORTED_CREATE_CODEC_INFO
 *           2. mimeType is random, excute 100000 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CapUnsupportedCreateCapInfo_003, TestSize.Level1)
{
    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO,
                                                          *meta_);
    }

    for (int i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string mime = GenerateRandomMime();
        meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
        meta_->SetData(Media::Tag::MIME_TYPE, mime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO,
                                                          *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_AppBehaviorsInfo_001
 * @tc.desc: 1. eventType is APP_BEHAVIORS_INFO
 *           2. forward process and caller process parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_AppBehaviorsInfo_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);

    std::string forwardProcessName = "forwardProcess";
    std::string callerProcessName = "callerProcess";

    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);

    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);

    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);

    callerProcessName = GenerateRandomString(200); // 200: clller name length
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_INFO, *meta_);

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_DecLimitExceededInfo_001
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO
 *           2. excute 200 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_DecLimitExceededInfo_001, TestSize.Level1)
{
    for (int32_t i = 0; i < BEHAVIORSINFO_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = "callerProcess";
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO, *meta_);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_RELEASE_HDEC_INFO, *meta_);
    }

    for (int32_t i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = GenerateRandomString(20); // 20: caller name length
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO, *meta_);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::APP_BEHAVIORS_RELEASE_HDEC_INFO, *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckJsonValue("DecAbnormalOccupationInfo", "HDecLimitExceededInfo");
}

/**
 * @tc.name: AddEventInfo_DecAbnormalOccupationLongTimeInBGInfo_001
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process and elapsed time parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_DecAbnormalOccupationLongTimeInBGInfo_001, TestSize.Level1)
{
    std::string callerProcessName = "callerProcess";
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO,
                                                      *meta_);

    for (auto elapsedTime : {ELAPSEDTIME_THREADSHOLD / 2, ELAPSEDTIME_THREADSHOLD * 2}) {
        meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), elapsedTime);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO, *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_DecAbnormalOccupationLongTimeInBGInfo_004
 * @tc.desc: 1. eventType is DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO
 *           2. caller process name and elapsed time parameter test, excute 200 times
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_DecAbnormalOccupationLongTimeInBGInfo_004, TestSize.Level1)
{
    for (int32_t i = 0; i < BEHAVIORSINFO_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = "callerProcess";
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD * 2);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO, *meta_);
    }

    for (int32_t i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = GenerateRandomString(20); // 20: clller name length
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD * 2);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO, *meta_);
    }

    for (int32_t i = 0; i < MAX_EVENT_ADD_COUNT; i++) {
        std::string callerProcessName = GenerateRandomString(20); // 20: clller name length
        meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
        meta_->SetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), ELAPSEDTIME_THREADSHOLD / 2);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO, *meta_);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckJsonValue("DecAbnormalOccupationInfo", "LongTimeInBgInfo");
}

/**
 * @tc.name: AddEventInfo_SpeedDecodingInfo_001
 * @tc.desc: 1. eventType is SPEED_DECODING_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_SpeedDecodingInfo_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::SPEED_DECODING_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckJsonValue("SpeedDecodingInfo", "");
}

/**
 * @tc.name: AddEventInfo_CodecErrorInfo_001
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. codec type and error code parameter test
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecErrorInfo_001, TestSize.Level1)
{
    for (auto codecType : {-1, INT16_MAX, static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE)}) {
        auto meta = std::make_shared<Media::Meta>();
        meta->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), codecType);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta);
    }

    for (auto codecType : {-1, INT16_MAX, static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE)}) {
        auto meta = std::make_shared<Media::Meta>();
        meta->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), codecType);
        meta->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(AV_ERR_INVALID_VAL));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta);
    }

    for (auto codecType : {-1, INT16_MAX, static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE)}) {
        auto meta = std::make_shared<Media::Meta>();
        meta->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
        meta->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), codecType);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta);
    }

    for (auto codecType : {-1, INT16_MAX, static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE)}) {
        auto meta = std::make_shared<Media::Meta>();
        meta->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
        meta->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), codecType);
        meta->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(AV_ERR_INVALID_VAL));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta);
    }

    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    WaitForEventJson();
    auto data = std::shared_ptr<cJSON>(cJSON_Parse(g_recordJson.c_str()), cJSON_Delete);
    ASSERT_NE(nullptr, data);
}

/**
 * @tc.name: AddEventInfo_CodecErrorInfo_002
 * @tc.desc: 1. eventType is CODEC_ERROR_INFO
 *           2. mime type is video/avc, codec type is valid, error code is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_CodecErrorInfo_002, TestSize.Level1)
{
    meta_->SetData(Media::Tag::MIME_TYPE, static_cast<std::string>(CodecMimeType::VIDEO_AVC));
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(AV_ERR_INVALID_VAL));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
    CheckJsonValue("CodecErrorInfo", "");
}
} // namespace