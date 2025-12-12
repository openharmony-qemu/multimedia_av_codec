/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "statistics_event_handler.h"
#include <mutex>
#include <ctime>
#include "cJSON.h"
#include "hisysevent.h"
#include "avcodec_log.h"
#include "avcodec_server_manager.h"
#include "event_info_extented_key.h"
#include "avcodec_info.h"
#include "avcodec_xcollie.h"
#include "syspara/parameters.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace std::string_literals;
using AVCodecSpecifiedType = std::pair<bool, std::string>;
using VCodecSpecifiedType = std::pair<VideoCodecType, std::string>;

namespace {
constexpr std::size_t kHashMagicConstant = static_cast<std::size_t>(0x9e3779b97f4a7c15ULL);
constexpr unsigned kHashShiftLeft = 6u;
constexpr unsigned kHashShiftRight = 2u;

class EventStrings final {
public:
    static constexpr char DOMAIN[] = "AV_CODEC";
    static constexpr char EVENT_STATISTICS_INFO[] = "STATISTICS_INFO";

    static constexpr char QUERY_CAP_TIMES[] = "QueryCapTimes";
    static constexpr char CREATE_CODEC_TIMES[] = "CreateCodecTimes";

    static constexpr char CODEC_SPECIFIED_INFO[] = "CodecSpecifiedInfo";
    static constexpr char APP_NAME_DICT[] = "AppNameDict";
    static constexpr char CAP_UNSUPPORTED_INFO[] = "CapUnsupportedInfo";
    static constexpr char QUERY_CAP_UNSUPPORTED_INFO[] = "QueryCapUnsupportedInfo";
    static constexpr char CREATE_CODEC_UNSUPPORTED_INFO[] = "CreateCodecUnsupportedInfo";
    static constexpr char DEC_ABNORMAL_OCCUPATION_INFO[] = "DecAbnormalOccupationInfo";
    static constexpr char SPEED_DECODING_INFO[] = "SpeedDecodingInfo";
    static constexpr char CODEC_ERROR_INFO[] = "CodecErrorInfo";

    EventStrings() = delete;
    ~EventStrings() = delete;
};

inline void HashCombine(std::size_t &seed, std::size_t value)
{
    seed ^= value + kHashMagicConstant + (seed << kHashShiftLeft) + (seed >> kHashShiftRight);
}

const std::unordered_map<VideoCodecType, std::string> VIDEO_CODEC_TYPE_TO_STRING = {
    { VideoCodecType::DECODER_HARDWARE, "HDec" },
    { VideoCodecType::DECODER_SOFTWARE, "SDec" },
    { VideoCodecType::ENCODER_HARDWARE, "HEnc" },
    { VideoCodecType::ENCODER_SOFTWARE, "SEnc" },
};

class ScopedJsonString {
public:
    ScopedJsonString() = default;
    explicit ScopedJsonString(const std::shared_ptr<cJSON> &jsonObj)
    {
        if (jsonObj != nullptr) {
            str_ = cJSON_PrintUnformatted(jsonObj.get());
        }
    }

    ScopedJsonString(const ScopedJsonString &) = delete;
    ScopedJsonString &operator=(const ScopedJsonString &) = delete;

    ScopedJsonString(ScopedJsonString &&other) noexcept
    {
        str_ = other.str_;
        other.str_ = nullptr;
    }

    ScopedJsonString &operator=(ScopedJsonString &&other) noexcept
    {
        if (this != &other) {
            Reset();
            str_ = other.str_;
            other.str_ = nullptr;
        }
        return *this;
    }

    ~ScopedJsonString()
    {
        Reset();
    }

    const char *c_str() const
    {
        return (str_ != nullptr) ? str_ : "";
    }

private:
    void Reset()
    {
        if (str_ != nullptr) {
            cJSON_free(str_);
            str_ = nullptr;
        }
    }

    char *str_ = nullptr;
};
} // namespace

struct HashPair {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        std::size_t seed = 0;
        HashCombine(seed, std::hash<T1>{}(p.first));
        HashCombine(seed, std::hash<T2>{}(p.second));
        return seed;
    }
};
struct HashTuple {
    template <class T1, class T2, class T3>
    std::size_t operator()(const std::tuple<T1, T2, T3>& t) const
    {
        std::size_t seed = 0;
        HashCombine(seed, std::hash<T1>{}(std::get<0>(t)));
        HashCombine(seed, std::hash<T2>{}(std::get<1>(t)));
        HashCombine(seed, std::hash<T3>{}(std::get<2>(t)));
        return seed;
    }
};

template<typename T1, typename T2>
inline void CreateOrIncrementMapCount(T1 &map, const T2 &key)
{
    auto iter = map.find(key);
    if (iter != map.end()) {
        iter->second++;
    } else {
        map.emplace(key, 1);
    }
}

using AppNameIndex = int32_t;
constexpr AppNameIndex INVALID_APP_NAME_INDEX = -1;
class AppNameIndexInfo {
public:
    constexpr static size_t maxAppNameCount = 50;

    static AppNameIndexInfo &GetInstance()
    {
        static AppNameIndexInfo instance;
        return instance;
    }

    AppNameIndex GetIndexByAppName(const std::string &appName)
    {
        constexpr size_t maxAppNameLength = 127;
        AppNameIndex appIndex = INVALID_APP_NAME_INDEX;
        if (appName.size() > maxAppNameLength) {
            return INVALID_APP_NAME_INDEX;
        }
        auto dictIter = appNameDict_.find(appName);
        if (dictIter == appNameDict_.end()) {
            if (appNameDict_.size() < maxAppNameCount) {
                appIndex = static_cast<AppNameIndex>(appNameDict_.size());
                appNameDict_.emplace(appName, appIndex);
            }
        } else {
            appIndex = dictIter->second;
        }
        return appIndex;
    }

    std::string GetAppNameByIndex(AppNameIndex appIndex)
    {
        for (const auto &[name, index] : appNameDict_) {
            if (index == appIndex) {
                return name;
            }
        }
        return "";
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo)
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateArray(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return;
        }
        for (const auto &[name, index] : appNameDict_) {
            cJSON_AddNumberToObject(cJSON_AddObjectToObject(jsonObj.get(), name.c_str()), name.c_str(), index);
        }
        submitInfo.jsonObjects[EventStrings::APP_NAME_DICT] = std::move(jsonObj);
    };

    void Reset()
    {
        appNameDict_.clear();
    }

private:
    std::unordered_map<std::string, AppNameIndex> appNameDict_; // appName, AppNameIndex
};

class StatisticsMainEventInfoBase : public StatisticsEventInfoBase {
public:
    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        auto iter = eventInfoMap_.find(eventType & StatisticsEventType::SUB_EVENT_TYPE_MASK);
        if (iter != eventInfoMap_.end()) {
            iter->second->OnAddEventInfo(eventType, eventMeta);
        }
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        for (auto &[_, eventInfo] : eventInfoMap_) {
            eventInfo->OnSummateEventInfo(submitInfo);
        }
    }

    void ResetEventInfo() override
    {
        for (auto &eventInfo : eventInfoMap_) {
            eventInfo.second->ResetEventInfo();
        }
    }

protected:
    std::mutex mutex_;
};

/*----------------------------------- BasicInfo -----------------------------------*/

class BasicInfo : public StatisticsMainEventInfoBase {
public:
    BasicInfo() {}

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        switch (eventType) {
            case StatisticsEventType::BASIC_QUERY_CAP_INFO:
                queryCapTimes_++;
                break;
            case StatisticsEventType::BASIC_CREATE_CODEC_INFO:
                createCodecTimes_++;
                break;
            case StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO: {
                VideoCodecType vcodecType = VideoCodecType::UNKNOWN;
                std::string mimeType;
                if (eventMeta.GetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), vcodecType) &&
                    eventMeta.GetData(Media::Tag::MIME_TYPE, mimeType) &&
                    (vcodecType > VideoCodecType::UNKNOWN && vcodecType < VideoCodecType::END) &&
                    !mimeType.empty()) {
                    auto type = std::make_pair(vcodecType, mimeType);
                    CreateOrIncrementMapCount(codecSpecInfo_, type);
                }
                break;
            }
            default:
                break;
        }
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        submitInfo.queryCapTimes = queryCapTimes_;
        submitInfo.createCodecTimes = createCodecTimes_;

        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateArray(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return;
        }
        for (const auto &[codecSpecType, count] : codecSpecInfo_) {
            const auto &[vcodecType, mimeType] = codecSpecType;
            if (vcodecType <= VideoCodecType::UNKNOWN || vcodecType >= VideoCodecType::END || mimeType.empty()) {
                continue;
            }
            auto typeIter = VIDEO_CODEC_TYPE_TO_STRING.find(vcodecType);
            if (typeIter == VIDEO_CODEC_TYPE_TO_STRING.end()) {
                continue;
            }
            std::string key = typeIter->second + "_" + mimeType;
            cJSON_AddNumberToObject(cJSON_AddObjectToObject(jsonObj.get(), key.c_str()), key.c_str(), count);
        }
        submitInfo.jsonObjects[EventStrings::CODEC_SPECIFIED_INFO] = std::move(jsonObj);
        AppNameIndexInfo::GetInstance().OnSummateEventInfo(submitInfo);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queryCapTimes_ = 0;
        createCodecTimes_ = 0;
        codecSpecInfo_.clear();
    }

private:
    uint32_t queryCapTimes_ { 0 };
    uint32_t createCodecTimes_ { 0 };
    std::unordered_map<VCodecSpecifiedType, uint32_t, HashPair> codecSpecInfo_; // <VideoCodecType, mimeType>, count
};

/*----------------------------------- AppSpecificationsInfo -----------------------------------*/

class CapUnsupportedInfo;
class AppSpecificationsInfo : public StatisticsMainEventInfoBase {
public:
    AppSpecificationsInfo()
    {
        eventInfoMap_.emplace(StatisticsEventType::CAP_UNSUPPORTED_INFO, std::make_shared<CapUnsupportedInfo>());
    }
};

class CapUnsupportedInfo : public StatisticsMainEventInfoBase {
public:
    CapUnsupportedInfo() = default;

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string mimeType;
        bool isEncoder = false;
        if (!eventMeta.GetData(Media::Tag::MIME_TYPE, mimeType) || !IsValidMimeType(mimeType) ||
            !eventMeta.GetData(EventInfoExtentedKey::IS_ENCODER.data(), isEncoder)) {
            return;
        }
        auto type = std::make_pair(isEncoder, mimeType);
        switch (eventType) {
            case StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO:
                if (queryCapUnsupportedInfo_.size() >= maxUnsupportedTypeCount) {
                    break;
                }
                CreateOrIncrementMapCount(queryCapUnsupportedInfo_, type);
                break;
            case StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO:
                if (createCodecUnsupportedInfo_.size() >= maxUnsupportedTypeCount) {
                    break;
                }
                CreateOrIncrementMapCount(createCodecUnsupportedInfo_, type);
                break;
            default:
                break;
        }
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        auto addInfoToJsonObj = [](const std::shared_ptr<cJSON> &parentJsonObj, const std::string &objName,
                                   const std::unordered_map<AVCodecSpecifiedType, uint32_t, HashPair> &infoMap) {
            if (parentJsonObj == nullptr) {
                return;
            }
            auto infoJsonObj = cJSON_AddObjectToObject(parentJsonObj.get(), objName.c_str());
            for (const auto &[codecSpecType, count] : infoMap) {
                const auto &[isEncoder, mimeType] = codecSpecType;
                if (mimeType.empty()) {
                    continue;
                }
                std::string key = (isEncoder ? "Enc"s : "Dec"s) + "_" + mimeType;
                cJSON_AddNumberToObject(infoJsonObj, key.c_str(), count);
            }
        };

        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        addInfoToJsonObj(jsonObj, EventStrings::QUERY_CAP_UNSUPPORTED_INFO, queryCapUnsupportedInfo_);
        addInfoToJsonObj(jsonObj, EventStrings::CREATE_CODEC_UNSUPPORTED_INFO, createCodecUnsupportedInfo_);
        submitInfo.jsonObjects[EventStrings::CAP_UNSUPPORTED_INFO] = std::move(jsonObj);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queryCapUnsupportedInfo_.clear();
        createCodecUnsupportedInfo_.clear();
    }

private:
    bool IsValidMimeType(const std::string &mimeType)
    {
        constexpr size_t minMediaMimeTypeLength = 7;
        constexpr size_t maxMediaMimeTypeLength = 127;
        if (mimeType.size() < minMediaMimeTypeLength ||
            mimeType.size() > maxMediaMimeTypeLength) {
            return false;
        }
        auto pos = mimeType.find('/');
        if (pos == std::string::npos || pos == 0 || pos == mimeType.size() - 1) {
            return false;
        }
        std::string top = mimeType.substr(0, pos);
        // to lower case
        for (auto &ch : top) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (top != "video" && top != "audio") {
            return false;
        }
        std::string sub = mimeType.substr(pos + 1);
        for (const auto &ch : sub) {
            if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '+' || ch == '/' || ch == '.')) {
                return false;
            }
        }
        return true;
    }

    constexpr static size_t maxUnsupportedTypeCount = 50;
    std::unordered_map<AVCodecSpecifiedType, uint32_t, HashPair> queryCapUnsupportedInfo_;
    std::unordered_map<AVCodecSpecifiedType, uint32_t, HashPair> createCodecUnsupportedInfo_;
};

/*----------------------------------- AppBehaviorsInfo -----------------------------------*/

class DecAbnormalOccupationInfo;
class SpeedDecodingInfo;
class AppBehaviorsInfo : public StatisticsMainEventInfoBase {
public:
    AppBehaviorsInfo()
    {
        eventInfoMap_.emplace(StatisticsEventType::DEC_ABNORMAL_OCCUPATION_INFO,
                              std::make_shared<DecAbnormalOccupationInfo>());
        eventInfoMap_.emplace(StatisticsEventType::SPEED_DECODING_INFO, std::make_shared<SpeedDecodingInfo>());
    }
};

class DecAbnormalOccupationInfo : public StatisticsMainEventInfoBase {
public:
    DecAbnormalOccupationInfo() = default;

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string callerName;
        if (!(eventMeta.GetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, callerName) ||
              eventMeta.GetData(Media::Tag::AV_CODEC_CALLER_PROCESS_NAME, callerName)) ||
            callerName.empty()) {
            return;
        }
        AppNameIndex callerNameIndex = AppNameIndexInfo::GetInstance().GetIndexByAppName(callerName);
        if (callerNameIndex == INVALID_APP_NAME_INDEX) {
            return;
        }

        switch (eventType) {
            case StatisticsEventType::DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO: {
                if (hdecLimitExceededInfo_.size() < maxOccupationHDecRecordCount &&
                    !isInOccupationHDecEvent_.load()) {
                    auto usageStatistics = AVCodecServerManager::GetInstance().GetHDecUsageStatistics();
                    for (const auto& [holder, count] : usageStatistics) {
                        AppNameIndex appIndex = AppNameIndexInfo::GetInstance().GetIndexByAppName(holder);
                        lastOccupationHDecAppInfoVec_.emplace_back(appIndex, count);
                    }

                    isInOccupationHDecEvent_.store(true);
                    RegisterOccupationHDecLimitExceededEventHook(callerNameIndex);
                }
                break;
            }
            case StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO: {
                int32_t elapsedTime = 0;
                if (longTimeInBgInfo_.size() < maxOccupationHDecRecordCount &&
                    eventMeta.GetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), elapsedTime)) {
                    longTimeInBgInfo_.emplace(callerNameIndex, static_cast<uint32_t>(elapsedTime));
                }
                break;
            }
            default:
                break;
        }
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return;
        }

        auto hdecLimitExceededInfoJsonObj = cJSON_AddObjectToObject(jsonObj.get(), "HDecLimitExceededInfo");
        for (const auto &[appIndex, appInfoVec] : hdecLimitExceededInfo_) {
            auto appInfoArrayJsonObj = cJSON_AddArrayToObject(
                hdecLimitExceededInfoJsonObj, std::to_string(appIndex).c_str());
            for (const auto &[holderAppIndex, count] : appInfoVec) {
                std::string holderAppIdxStr = std::to_string(holderAppIndex);
                cJSON_AddNumberToObject(cJSON_AddObjectToObject(appInfoArrayJsonObj, holderAppIdxStr.c_str()),
                    holderAppIdxStr.c_str(), count);
            }
        }

        auto longTimeInBgInfoJsonObj = cJSON_AddObjectToObject(jsonObj.get(), "LongTimeInBgInfo");
        for (const auto &[appIndex, elapsedTime] : longTimeInBgInfo_) {
            cJSON_AddNumberToObject(longTimeInBgInfoJsonObj, std::to_string(appIndex).c_str(), elapsedTime);
        }

        submitInfo.jsonObjects[EventStrings::DEC_ABNORMAL_OCCUPATION_INFO] = std::move(jsonObj);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isInOccupationHDecEvent_.store(false);
        lastOccupationHDecAppInfoVec_.clear();
        hdecLimitExceededInfo_.clear();
        longTimeInBgInfo_.clear();
    }

private:
    void RegisterOccupationHDecLimitExceededEventHook(AppNameIndex callerNameIndex)
    {
        StatisticsEventInfo::GetInstance().RegisterEventHook(
            StatisticsEventType::APP_BEHAVIORS_RELEASE_HDEC_INFO,
            [this, callerNameIndex] (const Media::Meta &eventMeta) -> bool
            {
                if (!isInOccupationHDecEvent_.load())
                {
                    return true;
                }

                std::lock_guard<std::mutex> lock(mutex_);
                hdecLimitExceededInfo_.emplace(callerNameIndex, lastOccupationHDecAppInfoVec_);
                isInOccupationHDecEvent_.store(false);
                return true;
            });
    }

    using HDecLimitExceededAppInfoVector = std::vector<std::pair<AppNameIndex, uint32_t>>; // AppNameIndex, count
    constexpr static size_t maxOccupationHDecRecordCount = 200;
    std::atomic<bool> isInOccupationHDecEvent_{ false };
    HDecLimitExceededAppInfoVector lastOccupationHDecAppInfoVec_;
    std::unordered_multimap<AppNameIndex, HDecLimitExceededAppInfoVector> hdecLimitExceededInfo_;
    std::unordered_multimap<AppNameIndex, uint32_t> longTimeInBgInfo_;  // AppNameIndex, elapsedTime
};

class SpeedDecodingInfo : public StatisticsMainEventInfoBase {
public:
    SpeedDecodingInfo() = default;

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_TOTAL.data(), decDurationTotal_, decCntTotal_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_0_75X.data(), decDuration075x_, decCnt075x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_1_00X.data(), decDuration100x_, decCnt100x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_1_25X.data(), decDuration125x_, decCnt125x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_1_50X.data(), decDuration150x_, decCnt150x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_2_00X.data(), decDuration200x_, decCnt200x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_3_00X.data(), decDuration300x_, decCnt300x_);
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return;
        }
        auto speedRootObj = cJSON_AddObjectToObject(jsonObj.get(), EventStrings::SPEED_DECODING_INFO);
        std::lock_guard<std::mutex> lock(mutex_);
        auto totalObj = cJSON_AddObjectToObject(speedRootObj, "Total");
        cJSON_AddNumberToObject(totalObj, "Duration", decDurationTotal_);
        cJSON_AddNumberToObject(totalObj, "Count", decCntTotal_);

        auto speed075xObj = cJSON_AddObjectToObject(speedRootObj, "0.75x");
        cJSON_AddNumberToObject(speed075xObj, "Duration", decDuration075x_);
        cJSON_AddNumberToObject(speed075xObj, "Count", decCnt075x_);

        auto speed100xObj = cJSON_AddObjectToObject(speedRootObj, "1.00x");
        cJSON_AddNumberToObject(speed100xObj, "Duration", decDuration100x_);
        cJSON_AddNumberToObject(speed100xObj, "Count", decCnt100x_);

        auto speed125xObj = cJSON_AddObjectToObject(speedRootObj, "1.25x");
        cJSON_AddNumberToObject(speed125xObj, "Duration", decDuration125x_);
        cJSON_AddNumberToObject(speed125xObj, "Count", decCnt125x_);

        auto speed150xObj = cJSON_AddObjectToObject(speedRootObj, "1.50x");
        cJSON_AddNumberToObject(speed150xObj, "Duration", decDuration150x_);
        cJSON_AddNumberToObject(speed150xObj, "Count", decCnt150x_);

        auto speed200xObj = cJSON_AddObjectToObject(speedRootObj, "2.00x");
        cJSON_AddNumberToObject(speed200xObj, "Duration", decDuration200x_);
        cJSON_AddNumberToObject(speed200xObj, "Count", decCnt200x_);

        auto speed300xObj = cJSON_AddObjectToObject(speedRootObj, "3.00x");
        cJSON_AddNumberToObject(speed300xObj, "Duration", decDuration300x_);
        cJSON_AddNumberToObject(speed300xObj, "Count", decCnt300x_);

        submitInfo.jsonObjects[EventStrings::SPEED_DECODING_INFO] = std::move(jsonObj);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        decDurationTotal_ = 0U;
        decCntTotal_ = 0U;
        decDuration075x_ = 0U;
        decCnt075x_ = 0U;
        decDuration100x_ = 0U;
        decCnt100x_ = 0U;
        decDuration125x_ = 0U;
        decCnt125x_ = 0U;
        decDuration150x_ = 0U;
        decCnt150x_ = 0U;
        decDuration200x_ = 0U;
        decCnt200x_ = 0U;
        decDuration300x_ = 0U;
        decCnt300x_ = 0U;
    }

private:
    void ParseAndAccumulateSpeedData(
        const Media::Meta &eventMeta, const std::string_view& key, uint32_t& duration, uint32_t& count)
    {
        uint64_t combinedKey = 0;
        if (eventMeta.GetData(key.data(), combinedKey)) {
            constexpr uint64_t kLower32Mask = 0xFFFFFFFFULL;
            constexpr unsigned kUpper32Shift = 32u;
            duration += static_cast<uint32_t>(combinedKey & kLower32Mask);
            count += static_cast<uint32_t>(combinedKey >> kUpper32Shift);
        }
    };

    uint32_t decDurationTotal_ = 0U;
    uint32_t decCntTotal_ = 0U;
    uint32_t decDuration075x_ = 0U;
    uint32_t decCnt075x_ = 0U;
    uint32_t decDuration100x_ = 0U;
    uint32_t decCnt100x_ = 0U;
    uint32_t decDuration125x_ = 0U;
    uint32_t decCnt125x_ = 0U;
    uint32_t decDuration150x_ = 0U;
    uint32_t decCnt150x_ = 0U;
    uint32_t decDuration200x_ = 0U;
    uint32_t decCnt200x_ = 0U;
    uint32_t decDuration300x_ = 0U;
    uint32_t decCnt300x_ = 0U;
};

/*----------------------------------- CodecAbnormalInfo -----------------------------------*/

class CodecErrorInfo;
class CodecAbnormalInfo : public StatisticsMainEventInfoBase {
public:
    CodecAbnormalInfo()
    {
        eventInfoMap_.emplace(StatisticsEventType::CODEC_ERROR_INFO, std::make_shared<CodecErrorInfo>());
    }
};

class CodecErrorInfo : public StatisticsMainEventInfoBase {
public:
    CodecErrorInfo() = default;

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t errorCode = 0;
        std::string mimeType;
        VideoCodecType vcodecType = VideoCodecType::UNKNOWN;
        if (eventMeta.GetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), errorCode) &&
            eventMeta.GetData(Media::Tag::MIME_TYPE, mimeType) && !mimeType.empty() &&
            eventMeta.GetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), vcodecType) &&
            (vcodecType > VideoCodecType::UNKNOWN && vcodecType < VideoCodecType::END)) {
            CodecErrorInfoKey codecErrorKey = std::make_tuple(vcodecType, mimeType, errorCode);
            CreateOrIncrementMapCount(errorCodeInfo_, codecErrorKey);
        }
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return;
        }
        auto codecErrorRootObj = cJSON_AddObjectToObject(jsonObj.get(), EventStrings::CODEC_ERROR_INFO);
        for (const auto &[codecErrorKey, count] : errorCodeInfo_) {
            const auto &[vcodecType, mimeType, errorCode] = codecErrorKey;
            if (vcodecType <= VideoCodecType::UNKNOWN || vcodecType >= VideoCodecType::END || mimeType.empty()) {
                continue;
            }
            auto typeIter = VIDEO_CODEC_TYPE_TO_STRING.find(vcodecType);
            if (typeIter == VIDEO_CODEC_TYPE_TO_STRING.end()) {
                continue;
            }
            std::string key = typeIter->second + "_" + mimeType + "_Err_" + std::to_string(errorCode);
            cJSON_AddNumberToObject(codecErrorRootObj, key.c_str(), count);
        }

        submitInfo.jsonObjects[EventStrings::CODEC_ERROR_INFO] = std::move(jsonObj);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        errorCodeInfo_.clear();
    }

private:
    using CodecErrorInfoKey = std::tuple<VideoCodecType, std::string, int32_t>; // VideoCodecType, mimeType, errorCode
    std::unordered_map<CodecErrorInfoKey, uint32_t, HashTuple> errorCodeInfo_;  // CodecErrorInfoKey, count
};

/*----------------------------------- StatisticsEventInfo -----------------------------------*/

StatisticsEventInfo::StatisticsEventInfo()
{
    eventInfoMap_.emplace(StatisticsEventType::BASIC_INFO, std::make_shared<BasicInfo>());
    eventInfoMap_.emplace(StatisticsEventType::APP_SPECIFICATIONS_INFO, std::make_shared<AppSpecificationsInfo>());
    eventInfoMap_.emplace(StatisticsEventType::APP_BEHAVIORS_INFO, std::make_shared<AppBehaviorsInfo>());
    eventInfoMap_.emplace(StatisticsEventType::CODEC_ABNORMAL_INFO, std::make_shared<CodecAbnormalInfo>());
}

void StatisticsEventInfo::OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta)
{
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = eventInfoMap_.find(eventType & StatisticsEventType::MAIN_EVENT_TYPE_MASK);
        if (iter != eventInfoMap_.end() && iter->second != nullptr) {
            iter->second->OnAddEventInfo(eventType, eventMeta);
        }
    }

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto hookIter = eventHooks_.find(eventType);
        if (hookIter != eventHooks_.end()) {
            auto needErase = hookIter->second(eventMeta);
            if (needErase) {
                eventHooks_.erase(hookIter);
            }
        }
    }
}

void StatisticsEventInfo::OnSubmitEventInfo()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);

    StatisticsSubmitInfo submitInfo;
    for (auto &[_, eventInfo] : eventInfoMap_) {
        if (eventInfo != nullptr) {
            eventInfo->OnSummateEventInfo(submitInfo);
        }
    }

    const auto getJsonField = [&submitInfo](const char *key) -> ScopedJsonString {
        auto it = submitInfo.jsonObjects.find(key);
        if (it == submitInfo.jsonObjects.end()) {
            return ScopedJsonString{};
        }
        return ScopedJsonString(it->second);
    };

    HiSysEventWrite(
        EventStrings::DOMAIN, EventStrings::EVENT_STATISTICS_INFO,
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        EventStrings::QUERY_CAP_TIMES,               submitInfo.queryCapTimes,
        EventStrings::CREATE_CODEC_TIMES,            submitInfo.createCodecTimes,
        EventStrings::CODEC_SPECIFIED_INFO,          getJsonField(EventStrings::CODEC_SPECIFIED_INFO).c_str(),
        EventStrings::APP_NAME_DICT,                 getJsonField(EventStrings::APP_NAME_DICT).c_str(),
        EventStrings::QUERY_CAP_UNSUPPORTED_INFO,    getJsonField(EventStrings::QUERY_CAP_UNSUPPORTED_INFO).c_str(),
        EventStrings::CREATE_CODEC_UNSUPPORTED_INFO, getJsonField(EventStrings::CREATE_CODEC_UNSUPPORTED_INFO).c_str(),
        EventStrings::DEC_ABNORMAL_OCCUPATION_INFO,  getJsonField(EventStrings::DEC_ABNORMAL_OCCUPATION_INFO).c_str(),
        EventStrings::SPEED_DECODING_INFO,           getJsonField(EventStrings::SPEED_DECODING_INFO).c_str(),
        EventStrings::CODEC_ERROR_INFO,              getJsonField(EventStrings::CODEC_ERROR_INFO).c_str()
    );

    ResetEventInfo();
    RegisterSubmitEventTimer();
}

void StatisticsEventInfo::ResetEventInfo()
{
    eventHooks_.clear();
    AppNameIndexInfo::GetInstance().Reset();
    for (auto &eventInfo : eventInfoMap_) {
        eventInfo.second->ResetEventInfo();
    }
}

void StatisticsEventInfo::RegisterEventHook(StatisticsEventType eventType, EventHook hook)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    eventHooks_[eventType] = std::move(hook);
}

void StatisticsEventInfo::RegisterSubmitEventTimer()
{
#ifdef BUILD_ENG_VERSION
    return;
#endif

    uint32_t timeout = 0;
    time_t now = time(nullptr);
    std::tm tmLocal{};
    if (localtime_r(&now, &tmLocal) != nullptr) {
        tmLocal.tm_hour = 0;
        tmLocal.tm_min = 0;
        tmLocal.tm_sec = 0;
        time_t today = mktime(&tmLocal);
        time_t target = today + 5 * 3600; // 5 * 3600 AM today
        // Seek to next day if already past 5 AM
        if (now >= target) {
            target += 24 * 3600; // 24 * 3600 seconds
        }
        timeout = static_cast<uint32_t>(target - now);
    } else {
        timeout = 24 * 3600; // Fallback to 24 * 3600 seconds if localtime_r failed
    }
    timer_ = std::make_shared<AVCodecXcollieTimer>("SubmitStatisticsEvent", false, false, timeout,
                                                   [this](void *) { OnSubmitEventInfo(); });
}
} // namespace MediaAVCodec
} // namespace OHOS