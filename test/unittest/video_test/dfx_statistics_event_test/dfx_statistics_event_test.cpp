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
constexpr int32_t QUERY_MAX_EVENTS = 100000;
constexpr char TEST_DOMAIN[] = "AV_CODEC";
constexpr int32_t MAX_LEN_OF_EVENT_DOMAIN = 16;
constexpr int32_t MAX_LEN_OF_EVENT_NAME = 64;
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
    void InitHiSysEvent(HiSysEventQueryArg &arg, HiSysEventQueryRule &rule, HiSysEventQueryCallback &callback,
                        const char* name);

    std::shared_ptr<Media::Meta> meta_ = nullptr;
    HiSysEventQueryArg arg_{};
    HiSysEventQueryRule rule_{};
    HiSysEventQueryCallback callback_{};
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

int64_t GetMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

void InitQueryArg(HiSysEventQueryArg &arg)
{
    arg.beginTime = 0;
    arg.endTime = GetMilliseconds();
    arg.maxEvents = QUERY_MAX_EVENTS;
}

void OnHiSysEventQuery(HiSysEventRecord records[], size_t size)
{
    std::cout << "OnQuery size of records is " << size;
    for (size_t i = 0; i < size; i++) {
        HiSysEventRecord record = records[i];
        std::cout << record.jsonStr << std::endl;
    }
}

void OnHiSysEventComplete(int32_t reason, int32_t total)
{
    std::cout << "OnComplete: res = " << reason << " total = " << total << std::endl;
}

void InitHiSysEventCallback(HiSysEventQueryCallback &callback)
{
    callback.OnQuery = OnHiSysEventQuery;
    callback.OnComplete = OnHiSysEventComplete;
}

int CopyCString(char *dst, const std::string &src, size_t len)
{
    if (src.length() > len) {
        return -1;
    }
    return strcpy_s(dst, src.length() + 1, src.c_str());
}

void DfxStatisticsEventTest::InitHiSysEvent(HiSysEventQueryArg &arg, HiSysEventQueryRule &rule,
                                            HiSysEventQueryCallback &callback, const char* name)
{
    InitQueryArg(arg);
    (void)CopyCString(rule.domain, TEST_DOMAIN, MAX_LEN_OF_EVENT_DOMAIN);
    (void)CopyCString(rule.eventList[0], name, MAX_LEN_OF_EVENT_NAME);
    rule.eventListSize = 1;
    InitHiSysEventCallback(callback);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_001
 * @tc.desc: eventType is BASIC_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_001, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_002
 * @tc.desc: eventType is BASIC_QUERY_CAP_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_002, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_003
 * @tc.desc: eventType is BASIC_CREATE_CODEC_INFO
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_003, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_004
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is invalid, codecType is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_004, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    std::string mime = generateRandomString(10); // 10: length
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_005
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is invalid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_005, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    std::string mime = generateRandomString(10); // 10: length
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1);
    meta_->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_006
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is invalid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_006, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    std::string mime = generateRandomString(10); // 10: length
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    meta_->SetData(Media::Tag::MIME_TYPE, mime);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_007
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is valid, codecType is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_007, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_008
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is valid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_008, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_009
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is valid, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_009, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    meta_->SetData(Media::Tag::MIME_TYPE, CodecMimeType::VIDEO_AVC);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_010
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is empty, codecType is valid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_010, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::DECODER_HARDWARE);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_011
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is empty, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_011, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), -1);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}

/**
 * @tc.name: AddEventInfo_BasicInfo_012
 * @tc.desc: 1. eventType is BASIC_CREATE_CODEC_SPEC_INFO
 *           2. mimeType is empty, codecType is invalid
 * @tc.type: FUNC
 */
HWTEST_F(DfxStatisticsEventTest, AddEventInfo_BasicInfo_012, TestSize.Level1)
{
    constexpr char TEST_NAME[] = "BASIC_INFO";
    InitHiSysEvent(arg_, rule_, callback_, TEST_NAME);
    HiSysEventQueryRule rules[] = {rule_};

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), INT16_MAX);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();

    auto res = OH_HiSysEvent_Query(&arg_, rules, sizeof(rules) / sizeof(HiSysEventQueryRule), &callback_);
    ASSERT_EQ(res, HiviewDFX::IPC_CALL_SUCCEED);
}
} // namespace