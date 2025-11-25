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
#include "cJSON.h"
#include "avcodec_log.h"
#include "avcodec_server_manager.h"
#include "event_info_extented_key.h"
#include "avcodec_info.h"
#include "avcodec_sysevent.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace std::string_literals;
using AVCodecSpecifiedType = std::pair<bool, std::string>;
using VCodecSpecifiedType = std::pair<VideoCodecType, std::string>;
struct HashPair {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
    }
};
struct HashTuple {
    template <class T1, class T2, class T3>
    std::size_t operator()(const std::tuple<T1, T2, T3>& t) const
    {
        auto &[a, b, c] = t;
        return std::hash<T1>{}(a) ^ std::hash<T2>{}(b) ^ std::hash<T3>{}(c);
    }
};

std::unordered_map<VideoCodecType, std::string> videoCodecTypeToString = {
    { VideoCodecType::DECODER_HARDWARE, "HDec" },
    { VideoCodecType::DECODER_SOFTWARE, "SDec" },
    { VideoCodecType::ENCODER_HARDWARE, "HEnc" },
    { VideoCodecType::ENCODER_SOFTWARE, "SEnc" },
};

template<typename T1, typename T2>
inline void CreateOrIncrementMapCount(T1 &map, T2 &key)
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
        AppNameIndex appIndex = INVALID_APP_NAME_INDEX;
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

    void OnSummateEventInfo(std::shared_ptr<cJSON> jsonObj)
    {
        auto appDictArrayJsonObj = cJSON_AddArrayToObject(jsonObj.get(), "AppNameDict");
        for (const auto &[name, index] : appNameDict_) {
            cJSON_AddNumberToObject(cJSON_AddObjectToObject(appDictArrayJsonObj, name.c_str()), name.c_str(), index);
        }
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

    void OnSubmitEventInfo() override
    {
        auto mainEventJsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        for (auto &[eventType, eventInfo] : eventInfoMap_) {
            std::static_pointer_cast<StatisticsMainEventInfoBase>(eventInfo)->OnSummateEventInfo(mainEventJsonObj);
        }
        std::string mainEventStr = mainEventName_.empty() ? "UnknownMainEvent" : mainEventName_;
        StatisticEventWrite(mainEventStr, cJSON_PrintUnformatted(mainEventJsonObj.get()));
    }

    virtual void OnSummateEventInfo(std::shared_ptr<cJSON> jsonObj) {};

    void ResetEventInfo() override
    {
        for (auto &eventInfo : eventInfoMap_) {
            eventInfo.second->ResetEventInfo();
        }
    }

protected:
    std::mutex mutex_;
    std::string mainEventName_;
};

/*----------------------------------- BasicInfo -----------------------------------*/

class BasicInfo : public StatisticsMainEventInfoBase {
public:
    BasicInfo()
    {
        mainEventName_ = "BASIC_INFO";
    }

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

    void OnSubmitEventInfo() override
    {
        std::shared_ptr<cJSON> basicInfoJsonObj(cJSON_CreateObject(), cJSON_Delete);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cJSON_AddNumberToObject(basicInfoJsonObj.get(), "QueryCapTimes", queryCapTimes_);
            cJSON_AddNumberToObject(basicInfoJsonObj.get(), "CreateCodecTimes", createCodecTimes_);

            auto codecSpecInfoJsonObj = cJSON_AddArrayToObject(basicInfoJsonObj.get(), "CodecSpecifiedInfo");
            for (const auto &[codecSpecType, count] : codecSpecInfo_) {
                const auto &[vcodecType, mimeType] = codecSpecType;
                if (vcodecType <= VideoCodecType::UNKNOWN || vcodecType >= VideoCodecType::END || mimeType.empty()) {
                    continue;
                }
                std::string key = videoCodecTypeToString[vcodecType] + "_" + mimeType;
                cJSON_AddNumberToObject(cJSON_AddObjectToObject(codecSpecInfoJsonObj, key.c_str()), key.c_str(), count);
            }
            AppNameIndexInfo::GetInstance().OnSummateEventInfo(basicInfoJsonObj);
        }
        StatisticEventWrite(mainEventName_, cJSON_PrintUnformatted(basicInfoJsonObj.get()));
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
        mainEventName_ = "APP_SPECIFICATIONS_INFO";
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

    void OnSummateEventInfo(std::shared_ptr<cJSON> jsonObj) override
    {
        auto addInfoToJsonObj = [](std::shared_ptr<cJSON> parentJsonObj, const std::string &objName,
                                       const std::unordered_map<AVCodecSpecifiedType, uint32_t, HashPair> &infoMap) {
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
        addInfoToJsonObj(jsonObj, "QueryCapUnsupportedInfo", queryCapUnsupportedInfo_);
        addInfoToJsonObj(jsonObj, "CreateCodecUnsupportedInfo", createCodecUnsupportedInfo_);
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
        mainEventName_ = "APP_BEHAVIORS_INFO";
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
                    RegisterOccupationHDecLimitExceededEventHooker(callerNameIndex);
                }
                break;
            }
            case StatisticsEventType::DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO: {
                int32_t elapsedTime = 0;
                if (longTimeInBgInfo_.size() < maxOccupationHDecRecordCount &&
                    eventMeta.GetData(EventInfoExtentedKey::APP_ELAPSED_TIME_IN_BG.data(), elapsedTime) &&
                    elapsedTime >= appInBackgroundElapsedTimeThreadshold) {
                    longTimeInBgInfo_.emplace(callerNameIndex, static_cast<uint32_t>(elapsedTime));
                }
                break;
            }
            default:
                break;
        }
    }

    void OnSummateEventInfo(std::shared_ptr<cJSON> jsonObj) override
    {
        auto decAbnormalOccupationInfoJsonObj = cJSON_AddObjectToObject(jsonObj.get(), "DecAbnormalOccupationInfo");

        auto hdecLimitExceededInfoJsonObj =
            cJSON_AddObjectToObject(decAbnormalOccupationInfoJsonObj, "HDecLimitExceededInfo");
        for (const auto &[appIndex, appInfoVec] : hdecLimitExceededInfo_) {
            auto appInfoArrayJsonObj = cJSON_AddArrayToObject(
                hdecLimitExceededInfoJsonObj, std::to_string(appIndex).c_str());
            for (const auto &[holderAppIndex, count] : appInfoVec) {
                std::string holderAppIdxStr = std::to_string(holderAppIndex);
                cJSON_AddNumberToObject(cJSON_AddObjectToObject(appInfoArrayJsonObj, holderAppIdxStr.c_str()),
                    holderAppIdxStr.c_str(), count);
            }
        }

        auto longTimeInBgInfoJsonObj = cJSON_AddObjectToObject(decAbnormalOccupationInfoJsonObj, "LongTimeInBgInfo");
        for (const auto &[appIndex, elapsedTime] : longTimeInBgInfo_) {
            cJSON_AddNumberToObject(longTimeInBgInfoJsonObj, std::to_string(appIndex).c_str(), elapsedTime);
        }
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
    void RegisterOccupationHDecLimitExceededEventHooker(AppNameIndex callerNameIndex)
    {
        StatisticsEventInfo::GetInstance().RegisterEventHooker(
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
    constexpr static int32_t appInBackgroundElapsedTimeThreadshold = 600; // seconds
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
    }

    void OnSummateEventInfo(std::shared_ptr<cJSON> jsonObj) override
    {
    }

    void ResetEventInfo() override
    {
    }
};

/*----------------------------------- CodecAbnormalInfo -----------------------------------*/

class CodecErrorInfo;
class CodecAbnormalInfo : public StatisticsMainEventInfoBase {
public:
    CodecAbnormalInfo()
    {
        eventInfoMap_.emplace(StatisticsEventType::CODEC_ERROR_INFO, std::make_shared<CodecErrorInfo>());
        mainEventName_ = "CODEC_ABNORMAL_INFO";
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

    void OnSummateEventInfo(std::shared_ptr<cJSON> jsonObj) override
    {
        auto codecErrorInfoJsonObj = cJSON_AddObjectToObject(jsonObj.get(), "CodecErrorInfo");
        for (const auto &[codecErrorKey, count] : errorCodeInfo_) {
            const auto &[vcodecType, mimeType, errorCode] = codecErrorKey;
            if (vcodecType <= VideoCodecType::UNKNOWN || vcodecType >= VideoCodecType::END || mimeType.empty()) {
                continue;
            }
            std::string key = videoCodecTypeToString[vcodecType] + "_" + mimeType +
                "_Err_" + std::to_string(errorCode);
            cJSON_AddNumberToObject(codecErrorInfoJsonObj, key.c_str(), count);
        }
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto iter = eventInfoMap_.find(eventType & StatisticsEventType::MAIN_EVENT_TYPE_MASK);
    if (iter != eventInfoMap_.end()) {
        iter->second->OnAddEventInfo(eventType, eventMeta);
    }
    auto hookerIter = eventHookers_.find(eventType);
    if (hookerIter != eventHookers_.end()) {
        auto needErase = hookerIter->second(eventMeta);
        if (needErase) {
            eventHookers_.erase(hookerIter);
        }
    }
}

void StatisticsEventInfo::OnSubmitEventInfo()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    for (auto &eventInfo : eventInfoMap_) {
        eventInfo.second->OnSubmitEventInfo();
    }
    ResetEventInfo();
}

void StatisticsEventInfo::ResetEventInfo()
{
    eventHookers_.clear();
    AppNameIndexInfo::GetInstance().Reset();
    for (auto &eventInfo : eventInfoMap_) {
        eventInfo.second->ResetEventInfo();
    }
}

void StatisticsEventInfo::RegisterEventHooker(StatisticsEventType eventType, EventHooker hooker)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    eventHookers_[eventType] = hooker;
}
} // namespace MediaAVCodec
} // namespace OHOS