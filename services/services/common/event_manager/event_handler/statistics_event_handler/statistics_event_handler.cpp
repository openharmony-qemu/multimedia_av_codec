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
#include <algorithm>
#include <cmath>
#include <mutex>
#include <ctime>
#include <type_traits>
#include "cJSON.h"
#include "hisysevent.h"
#include "avcodec_log.h"
#include "avcodec_server_manager.h"
#include "event_info_extented_key.h"
#include "avcodec_info.h"
#include "avcodec_xcollie.h"
#include "syspara/parameters.h"
#include "avcodec_errors.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace std::string_literals;
using AVCodecSpecifiedType = std::pair<bool, std::string>;
using VCodecSpecifiedType = std::tuple<VideoCodecType, std::string, bool>;

namespace {
// EVENT_STR_L0
static constexpr const char EVENT_DOMAIN[] = "AV_CODEC";
// EVENT_STR_L1
static constexpr const char EVENT_STATISTICS_INFO[] = "STATISTICS_INFO";
// EVENT_STR_L2
static constexpr const char QUERY_CAP_TIMES[] = "QUERY_CAP_TIMES";
static constexpr const char CREATE_CODEC_TIMES[] = "CREATE_CODEC_TIMES";
static constexpr const char CODEC_SPECIFIED_INFO[] = "CODEC_SPECIFIED_INFO";
static constexpr const char APP_NAME_DICT[] = "APP_NAME_DICT";
static constexpr const char CAP_UNSUPPORTED_INFO[] = "CAP_UNSUPPORTED_INFO";
static constexpr const char DEC_ABNORMAL_OCCUPATION_INFO[] = "DEC_ABNORMAL_OCCUPATION_INFO";
static constexpr const char SPEED_DECODING_INFO[] = "SPEED_DECODING_INFO";
static constexpr const char CODEC_ERROR_INFO[] = "CODEC_ERROR_INFO";
static constexpr const char CODEC_INSTANCE_USAGE_INFO[] = "CODEC_INSTANCE_USAGE_INFO";
static constexpr const char RESOLUTION_SPEC_INFO[] = "RESOLUTION_SPEC_INFO";
static constexpr const char CONTAINER_FORMAT_INFO[] = "CONTAINER_FORMAT_INFO";
static constexpr const char CODEC_MODE_INFO[] = "CODEC_MODE_INFO";
static constexpr const char ADVANCED_FEATURE_INFO[] = "ADVANCED_FEATURE_INFO";
static constexpr const char SECURE_DEC_ALLOC_BUF_FAILED_INFO[] = "SECURE_DEC_ALLOC_BUF_FAILED_INFO";
// EVENT_STR_L3
static constexpr const char QUERY_CAP_UNSUPPORTED_INFO[] = "QUERY_CAP_UNSUPPORTED_INFO";
static constexpr const char CREATE_CODEC_UNSUPPORTED_INFO[] = "CREATE_CODEC_UNSUPPORTED_INFO";

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "StatisticsEventHandler"};

const std::unordered_map<VideoCodecType, std::string> VIDEO_CODEC_TYPE_TO_STRING = {
    { VideoCodecType::DECODER_HARDWARE, "HDec" },
    { VideoCodecType::DECODER_SOFTWARE, "SDec" },
    { VideoCodecType::ENCODER_HARDWARE, "HEnc" },
    { VideoCodecType::ENCODER_SOFTWARE, "SEnc" },
};

const std::unordered_map<VideoEncodeBitrateMode, std::string> BITRATE_MODE_TO_STRING = {
    { VideoEncodeBitrateMode::CBR, "CBR" },
    { VideoEncodeBitrateMode::VBR, "VBR" },
    { VideoEncodeBitrateMode::CQ, "CQ" },
    { VideoEncodeBitrateMode::SQR, "SQR" },
    { VideoEncodeBitrateMode::CBR_HIGH_QUALITY, "CBR_HIGH_QUALITY" },
    { VideoEncodeBitrateMode::CBR_VIDEOCALL, "CBR_VIDEOCALL" },
    { VideoEncodeBitrateMode::CRF, "CRF" },
};

std::string ToString(VideoEncodeBitrateMode mode)
{
    auto it = BITRATE_MODE_TO_STRING.find(mode);
    return (it != BITRATE_MODE_TO_STRING.end()) ? it->second : "UNKNOWN";
}

std::string GetMetaString(const Media::Meta &meta, const std::string_view &key, const std::string &defaultValue = "")
{
    std::string value;
    if (!meta.GetData(key.data(), value) || value.empty()) {
        return defaultValue;
    }
    return value;
}

template<typename T>
std::string GetMetaValueString(const Media::Meta &meta, const std::string_view &key, const std::string &defaultValue)
{
    T value {};
    if (!meta.GetData(key.data(), value)) {
        return defaultValue;
    }
    return std::to_string(value);
}

std::string JoinKey(const std::vector<std::string> &parts)
{
    std::string key;
    for (const auto &part : parts) {
        if (part.empty()) {
            continue;
        }
        if (!key.empty()) {
            key += "_";
        }
        key += part.empty() ? "UNKNOWN" : part;
    }
    return key;
}

time_t GetCurrentTime()
{
    time_t now = time(nullptr);
    return now == static_cast<time_t>(-1) ? 0 : now;
}

void AddMapToJsonObject(cJSON *root, const std::unordered_map<std::string, uint32_t> &records)
{
    if (root == nullptr) {
        return;
    }
    for (const auto &[key, count] : records) {
        cJSON_AddNumberToObject(root, key.c_str(), count);
    }
}

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

    const char *CStr() const
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
        return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
    }
};
struct HashTuple {
    template <class T1, class T2, class T3>
    std::size_t operator()(const std::tuple<T1, T2, T3>& t) const
    {
        const auto &[a, b, c] = t;
        return std::hash<T1>{}(a) ^ std::hash<T2>{}(b) ^ std::hash<T3>{}(c);
    }
};

constexpr size_t MAX_STATISTICS_KEY_STRING_LENGTH = 127;
constexpr size_t MAX_STATISTICS_MAP_ENTRY_COUNT = 127;

// Only validate string keys here; non-string keys (pair/tuple) are validated
// separately at their reporting sites before calling this function.
template<typename T1, typename T2>
inline void CreateOrIncrementMapCount(T1 &map, const T2 &key)
{
    CHECK_AND_RETURN_LOGW(map.size() < MAX_STATISTICS_MAP_ENTRY_COUNT, "Statistics map entry count limit");
    if constexpr (std::is_same_v<std::decay_t<T2>, std::string>) {
        CHECK_AND_RETURN_LOGW(key.size() <= MAX_STATISTICS_KEY_STRING_LENGTH, "Statistics key string too long");
    }
    auto iter = map.find(key);
    if (iter != map.end()) {
        iter->second++;
    } else {
        map.emplace(key, 1);
    }
}

struct StatisticsSubmitInfo {
    uint32_t queryCapTimes {0};
    uint32_t createCodecTimes {0};
    std::unordered_map<std::string_view, std::shared_ptr<cJSON>> jsonObjects;
};

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
        std::lock_guard<std::shared_mutex> lock(mutex_);
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

    AppNameIndex GetIndexByMeta(const Media::Meta &meta)
    {
        std::string callerName;
        pid_t pid = INVALID_PID;
        if (!(meta.GetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, callerName) ||
              meta.GetData(Media::Tag::AV_CODEC_CALLER_PROCESS_NAME, callerName) ||
              meta.GetData(Media::Tag::AV_CODEC_CALLER_PID, pid))) {
            return INVALID_APP_NAME_INDEX;
        }
        if (callerName.empty() && pid != INVALID_PID) {
            callerName = AVCodecServerManager::GetInstance().GetCallerNameByPid(pid);
        }
        return callerName.empty() ? INVALID_APP_NAME_INDEX : GetIndexByAppName(callerName);
    }

    std::string GetAppNameByIndex(AppNameIndex appIndex)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
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
        std::shared_lock<std::shared_mutex> lock(mutex_);
        for (const auto &[name, index] : appNameDict_) {
            cJSON_AddNumberToObject(cJSON_AddObjectToObject(jsonObj.get(), name.c_str()), name.c_str(), index);
        }
        submitInfo.jsonObjects[APP_NAME_DICT] = std::move(jsonObj);
    };

    void Reset()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        appNameDict_.clear();
    }

private:
    std::shared_mutex mutex_;
    std::unordered_map<std::string, AppNameIndex> appNameDict_; // appName, AppNameIndex
};

class StatisticsEventInfoBase {
public:
    StatisticsEventInfoBase() = default;
    virtual ~StatisticsEventInfoBase() = default;

    virtual void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) = 0;
    virtual void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) = 0;
    virtual void ResetEventInfo() = 0;
    virtual bool IsEventValid() = 0;

protected:
    std::mutex mutex_;
    std::unordered_map<StatisticsEventType, std::shared_ptr<StatisticsEventInfoBase>> eventInfoMap_;
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

    bool IsEventValid() override { return false; }
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
            case StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO:
                OnBasicCreateCodecSpecInfo(eventMeta);
                break;
            case StatisticsEventType::CODEC_INSTANCE_USAGE_INFO:
                OnCodecInstanceUsageInfo(eventMeta);
                break;
            case StatisticsEventType::RESOLUTION_SPEC_INFO:
                OnResolutionSpecInfo(eventMeta);
                break;
            case StatisticsEventType::CONTAINER_FORMAT_INFO:
                OnContainerFormatInfo(eventMeta);
                break;
            case StatisticsEventType::CODEC_MODE_INFO:
                OnCodecModeInfo(eventMeta);
                break;
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
            const auto &[vcodecType, mimeType, isSecure] = codecSpecType;
            if (vcodecType <= VideoCodecType::UNKNOWN || vcodecType >= VideoCodecType::END || mimeType.empty()) {
                continue;
            }
            auto typeIter = VIDEO_CODEC_TYPE_TO_STRING.find(vcodecType);
            if (typeIter == VIDEO_CODEC_TYPE_TO_STRING.end()) {
                continue;
            }
            std::string key = typeIter->second + "_" + mimeType;
            if (isSecure) {
                key += "_secure";
            }
            cJSON_AddNumberToObject(cJSON_AddObjectToObject(jsonObj.get(), key.c_str()), key.c_str(), count);
        }
        for (const auto &[key, count] : audioCodecSpecInfo_) {
            cJSON_AddNumberToObject(cJSON_AddObjectToObject(jsonObj.get(), key.c_str()), key.c_str(), count);
        }
        submitInfo.jsonObjects[CODEC_SPECIFIED_INFO] = std::move(jsonObj);
        SettleInstanceUsage(GetCurrentTime());
        submitInfo.jsonObjects[CODEC_INSTANCE_USAGE_INFO] = BuildInstanceUsageJson();
        submitInfo.jsonObjects[RESOLUTION_SPEC_INFO] = BuildCountJson(resolutionSpecInfo_);
        submitInfo.jsonObjects[CONTAINER_FORMAT_INFO] = BuildContainerFormatJson();
        submitInfo.jsonObjects[CODEC_MODE_INFO] = BuildCountJson(codecModeInfo_);
        AppNameIndexInfo::GetInstance().OnSummateEventInfo(submitInfo);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queryCapTimes_ = 0;
        createCodecTimes_ = 0;
        codecSpecInfo_.clear();
        audioCodecSpecInfo_.clear();
        instanceUsageInfo_.clear();
        resolutionSpecInfo_.clear();
        demuxerFormatInfo_.clear();
        muxerFormatInfo_.clear();
        codecModeInfo_.clear();
    }

    bool IsEventValid() override
    {
        return queryCapTimes_ > 0 || createCodecTimes_ > 0 || !codecSpecInfo_.empty() ||
               !audioCodecSpecInfo_.empty();
    }

private:
    struct InstanceUsage {
        uint32_t currentCount = 0;
        uint32_t periodMax = 0;
        uint64_t weightedSum = 0;
        time_t periodStartTime = GetCurrentTime();
        time_t lastSettleTime = GetCurrentTime();
    };

    static constexpr const char instanceActionCreate[] = "create";
    static constexpr const char instanceActionRelease[] = "release";
    static constexpr const char instanceActionEmpty[] = "empty";
    static constexpr int32_t resolution360P = 360;
    static constexpr int32_t resolution480P = 480;
    static constexpr int32_t resolution576P = 576;
    static constexpr int32_t resolution720P = 720;
    static constexpr int32_t resolution1080P = 1080;
    static constexpr int32_t resolution1440P = 1440;
    static constexpr int32_t resolution2160P = 2160;
    static constexpr int32_t resolution4320P = 4320;

    void SettleInstanceUsage(time_t now)
    {
        for (auto &[_, usage] : instanceUsageInfo_) {
            if (now > usage.lastSettleTime) {
                usage.weightedSum += static_cast<uint64_t>(usage.currentCount) *
                    static_cast<uint64_t>(now - usage.lastSettleTime);
                usage.lastSettleTime = now;
            }
        }
    }

    void OnBasicCreateCodecSpecInfo(const Media::Meta &meta)
    {
        std::string codecType;
        if (meta.GetData(EventInfoExtentedKey::CODEC_TYPE.data(), codecType) &&
            (codecType == std::to_string(AVCODEC_TYPE_AUDIO_ENCODER) ||
            codecType == std::to_string(AVCODEC_TYPE_AUDIO_DECODER))) {
            bool isEncoder = (codecType == std::to_string(AVCODEC_TYPE_AUDIO_ENCODER));
            OnAudioCodecSpecInfo(meta, isEncoder);
            return;
        }

        VideoCodecType vcodecType = VideoCodecType::UNKNOWN;
        std::string mimeType;
        if (meta.GetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), vcodecType) &&
            meta.GetData(Media::Tag::MIME_TYPE, mimeType) &&
            (vcodecType > VideoCodecType::UNKNOWN && vcodecType < VideoCodecType::END) &&
            !mimeType.empty() && mimeType.size() <= MAX_STATISTICS_KEY_STRING_LENGTH) {
            const bool isSecure = GetMetaString(meta, Media::Tag::MEDIA_CODEC_NAME).find("secure") !=
                std::string::npos;
            auto type = std::make_tuple(vcodecType, mimeType, isSecure);
            CreateOrIncrementMapCount(codecSpecInfo_, type);
        }
    }

    void OnAudioCodecSpecInfo(const Media::Meta &meta, bool isEncoder)
    {
        std::string mimeType;
        if (!meta.GetData(Media::Tag::MIME_TYPE, mimeType) || mimeType.empty()) {
            return;
        }
        std::string kind = isEncoder ? "SEnc" : "SDec";
        std::string key = kind + "_" + mimeType;
        CreateOrIncrementMapCount(audioCodecSpecInfo_, key);
    }

    void OnCodecInstanceUsageInfo(const Media::Meta &meta)
    {
        time_t now = GetCurrentTime();
        SettleInstanceUsage(now);
        std::string key = IsEncoder(meta) ? "Enc" : "Dec";
        auto &usage = instanceUsageInfo_[key];
        usage.lastSettleTime = now;
        std::string action = GetMetaString(meta, EventInfoExtentedKey::INSTANCE_ACTION, instanceActionEmpty);
        if (action == instanceActionEmpty) {
            return;
        }
        if (action == instanceActionRelease) {
            if (usage.currentCount > 0) {
                usage.currentCount--;
            }
        } else if (action == instanceActionCreate) {
            usage.currentCount++;
            usage.periodMax = std::max(usage.periodMax, usage.currentCount);
        }
    }

    void OnResolutionSpecInfo(const Media::Meta &meta)
    {
        std::string level = GetMetaString(meta, EventInfoExtentedKey::RESOLUTION_LEVEL);
        if (level.empty()) {
            level = ResolveResolutionLevel(meta);
        }
        std::string key = JoinKey({
            IsEncoder(meta) ? "Enc" : "Dec",
            GetMetaString(meta, Media::Tag::MIME_TYPE, "UNKNOWN"),
            level
        });
        CreateOrIncrementMapCount(resolutionSpecInfo_, key);
    }

    std::string ResolveResolutionLevel(const Media::Meta &meta) const
    {
        int32_t width = 0;
        int32_t height = 0;
        if (!meta.GetData(EventInfoExtentedKey::WIDTH.data(), width) ||
            !meta.GetData(EventInfoExtentedKey::HEIGHT.data(), height) || width <= 0 || height <= 0) {
            return "OTHER";
        }
        
        constexpr double aspectRatio = 16.0 / 9.0;
        int64_t area = static_cast<int64_t>(width) * static_cast<int64_t>(height);
        int32_t normalized = static_cast<int32_t>(std::sqrt(static_cast<double>(area) / aspectRatio));
        if (normalized <= resolution360P) {
            return "360P";
        } else if (normalized <= resolution480P) {
            return "480P";
        } else if (normalized <= resolution576P) {
            return "576P";
        } else if (normalized <= resolution720P) {
            return "720P";
        } else if (normalized <= resolution1080P) {
            return "1080P";
        } else if (normalized <= resolution1440P) {
            return "1440P";
        } else if (normalized <= resolution2160P) {
            return "2160P";
        } else if (normalized <= resolution4320P) {
            return "4320P";
        }
        return "OTHER";
    }

    void OnContainerFormatInfo(const Media::Meta &meta)
    {
        std::string muxerFormat = GetMetaString(meta, EventInfoExtentedKey::MUXER_FORMAT);
        if (!muxerFormat.empty()) {
            CreateOrIncrementMapCount(muxerFormatInfo_, muxerFormat);
            return;
        }
        std::string format = GetMetaString(meta, EventInfoExtentedKey::DEMUXER_FORMAT, "fail:unknown");
        CreateOrIncrementMapCount(demuxerFormatInfo_, format);
    }

    void OnCodecModeInfo(const Media::Meta &meta)
    {
        std::string key = JoinKey({
            IsEncoder(meta) ? "Enc" : "Dec",
            GetMetaString(meta, Media::Tag::MIME_TYPE, "UNKNOWN"),
            GetMetaString(meta, EventInfoExtentedKey::CODEC_MODE, "UNKNOWN")
        });
        CreateOrIncrementMapCount(codecModeInfo_, key);
    }

    bool IsEncoder(const Media::Meta &meta) const
    {
        bool isEncoder = false;
        if (meta.GetData(EventInfoExtentedKey::IS_ENCODER.data(), isEncoder)) {
            return isEncoder;
        }
        AVCodecType codecType = AVCODEC_TYPE_NONE;
        if (meta.GetData(EventInfoExtentedKey::CODEC_TYPE.data(), codecType)) {
            return codecType == AVCODEC_TYPE_VIDEO_ENCODER || codecType == AVCODEC_TYPE_AUDIO_ENCODER;
        }
        return false;
    }

    std::shared_ptr<cJSON> BuildCountJson(const std::unordered_map<std::string, uint32_t> &records) const
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        AddMapToJsonObject(jsonObj.get(), records);
        return jsonObj;
    }

    std::shared_ptr<cJSON> BuildContainerFormatJson() const
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return jsonObj;
        }
        auto demuxerObj = cJSON_AddObjectToObject(jsonObj.get(), "DEMUXER");
        AddMapToJsonObject(demuxerObj, demuxerFormatInfo_);
        auto muxerObj = cJSON_AddObjectToObject(jsonObj.get(), "MUXER");
        AddMapToJsonObject(muxerObj, muxerFormatInfo_);
        return jsonObj;
    }

    std::shared_ptr<cJSON> BuildInstanceUsageJson() const
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return jsonObj;
        }
        for (const auto &[key, usage] : instanceUsageInfo_) {
            auto infoObj = cJSON_AddObjectToObject(jsonObj.get(), key.c_str());
            cJSON_AddNumberToObject(infoObj, "Current", usage.currentCount);
            cJSON_AddNumberToObject(infoObj, "Max", usage.periodMax);
            double duration = static_cast<double>(usage.lastSettleTime - usage.periodStartTime);
            double average = duration > 0 ? static_cast<double>(usage.weightedSum) / duration : usage.currentCount;
            cJSON_AddNumberToObject(infoObj, "WeightedAverage", average);
        }
        return jsonObj;
    }

    uint32_t queryCapTimes_ { 0 };
    uint32_t createCodecTimes_ { 0 };
    // <VideoCodecType, mimeType, isSecure>, count
    std::unordered_map<VCodecSpecifiedType, uint32_t, HashTuple> codecSpecInfo_;
    std::unordered_map<std::string, uint32_t> audioCodecSpecInfo_;
    std::unordered_map<std::string, InstanceUsage> instanceUsageInfo_;
    std::unordered_map<std::string, uint32_t> resolutionSpecInfo_;
    std::unordered_map<std::string, uint32_t> demuxerFormatInfo_;
    std::unordered_map<std::string, uint32_t> muxerFormatInfo_;
    std::unordered_map<std::string, uint32_t> codecModeInfo_;
};

/*----------------------------------- AppSpecificationsInfo -----------------------------------*/

class CapUnsupportedInfo;
class AdvancedFeatureInfo;
class AppSpecificationsInfo : public StatisticsMainEventInfoBase {
public:
    AppSpecificationsInfo()
    {
        eventInfoMap_.emplace(StatisticsEventType::CAP_UNSUPPORTED_INFO, std::make_shared<CapUnsupportedInfo>());
        eventInfoMap_.emplace(StatisticsEventType::ADVANCED_FEATURE_INFO, std::make_shared<AdvancedFeatureInfo>());
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

        std::lock_guard<std::mutex> lock(mutex_);
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        addInfoToJsonObj(jsonObj, QUERY_CAP_UNSUPPORTED_INFO, queryCapUnsupportedInfo_);
        addInfoToJsonObj(jsonObj, CREATE_CODEC_UNSUPPORTED_INFO, createCodecUnsupportedInfo_);
        submitInfo.jsonObjects[CAP_UNSUPPORTED_INFO] = std::move(jsonObj);
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

class AdvancedFeatureInfo : public StatisticsMainEventInfoBase {
public:
    AdvancedFeatureInfo() = default;

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string feature = GetMetaString(eventMeta, EventInfoExtentedKey::ADVANCED_FEATURE);
        if (feature.empty()) {
            return;
        }
        std::string featurePending;
        if (int32_t rcMode = 0;
            feature == "RC_MODE"s && eventMeta.GetData(EventInfoExtentedKey::RC_MODE.data(), rcMode)) {
            featurePending = ToString(static_cast<VideoEncodeBitrateMode>(rcMode));
        }
        AppNameIndex appIndex = AppNameIndexInfo::GetInstance().GetIndexByMeta(eventMeta);
        if (appIndex == INVALID_APP_NAME_INDEX) {
            return;
        }
        CreateOrIncrementMapCount(appAdvancedFeatureInfo_,
            JoinKey({std::to_string(appIndex), feature, featurePending}));
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return;
        }
        AddMapToJsonObject(jsonObj.get(), appAdvancedFeatureInfo_);
        submitInfo.jsonObjects[ADVANCED_FEATURE_INFO] = std::move(jsonObj);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        appAdvancedFeatureInfo_.clear();
    }

private:
    std::unordered_map<std::string, uint32_t> appAdvancedFeatureInfo_;
};

/*----------------------------------- AppBehaviorsInfo -----------------------------------*/

class DecAbnormalOccupationInfo;
class SpeedDecodingInfo;
class SecureDecoderAllocBufFailedInfo;
class AppBehaviorsInfo : public StatisticsMainEventInfoBase {
public:
    AppBehaviorsInfo()
    {
        eventInfoMap_.emplace(StatisticsEventType::DEC_ABNORMAL_OCCUPATION_INFO,
                              std::make_shared<DecAbnormalOccupationInfo>());
        eventInfoMap_.emplace(StatisticsEventType::SPEED_DECODING_INFO, std::make_shared<SpeedDecodingInfo>());
        eventInfoMap_.emplace(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO,
                              std::make_shared<SecureDecoderAllocBufFailedInfo>());
    }
};

class DecAbnormalOccupationInfo : public StatisticsMainEventInfoBase {
public:
    DecAbnormalOccupationInfo() = default;

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        AppNameIndex callerNameIndex = AppNameIndexInfo::GetInstance().GetIndexByMeta(eventMeta);
        if (callerNameIndex == INVALID_APP_NAME_INDEX) {
            return;
        }

        switch (eventType) {
            case StatisticsEventType::DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO: {
                if (hdecLimitExceededInfo_.size() < maxOccupationHDecRecordCount &&
                    !isInOccupationHDecEvent_.load()) {
                    auto usageStatistics = AVCodecServerManager::GetInstance().GetHDecUsageStatistics();
                    for (const auto& [holder, count] : usageStatistics) {
                        CHECK_AND_BREAK_LOG(lastOccupationHDecAppInfoVec_.size() < maxOccupationHDecRecordCount,
                            "lastOccupationHDecAppInfoVec_ reached limit %{public}zu", maxOccupationHDecRecordCount);
                        AppNameIndex appIndex = AppNameIndexInfo::GetInstance().GetIndexByAppName(holder);
                        lastOccupationHDecAppInfoVec_.emplace_back(appIndex, count);
                    }

                    lastCallerNameIndex_ = callerNameIndex;
                    isInOccupationHDecEvent_.store(true);
                    RegisterOccupationHDecLimitExceededEventHook();
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

        std::lock_guard<std::mutex> lock(mutex_);
        if (isInOccupationHDecEvent_.load()) {
            hdecLimitExceededInfo_.emplace(lastCallerNameIndex_, lastOccupationHDecAppInfoVec_);
            lastOccupationHDecAppInfoVec_.clear();
            isInOccupationHDecEvent_.store(false);
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

        submitInfo.jsonObjects[DEC_ABNORMAL_OCCUPATION_INFO] = std::move(jsonObj);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isInOccupationHDecEvent_.store(false);
        lastCallerNameIndex_ = INVALID_APP_NAME_INDEX;
        lastOccupationHDecAppInfoVec_.clear();
        hdecLimitExceededInfo_.clear();
        longTimeInBgInfo_.clear();
    }

private:
    void RegisterOccupationHDecLimitExceededEventHook()
    {
        StatisticsEventInfo::GetInstance().RegisterEventHook(
            StatisticsEventType::APP_BEHAVIORS_RELEASE_HDEC_INFO,
            [this] (const Media::Meta &eventMeta) -> bool {
                if (!isInOccupationHDecEvent_.load()) {
                    return true;
                }

                std::lock_guard<std::mutex> lock(mutex_);
                hdecLimitExceededInfo_.emplace(lastCallerNameIndex_, lastOccupationHDecAppInfoVec_);
                lastOccupationHDecAppInfoVec_.clear();
                isInOccupationHDecEvent_.store(false);
                return true;
            });
    }

    using HDecLimitExceededAppInfoVector = std::vector<std::pair<AppNameIndex, uint32_t>>; // AppNameIndex, count
    constexpr static size_t maxOccupationHDecRecordCount = 200;
    std::atomic<bool> isInOccupationHDecEvent_{ false };
    AppNameIndex lastCallerNameIndex_ = INVALID_APP_NAME_INDEX;
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
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_TOTAL, decDurationTotal_, decCntTotal_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_0_75X, decDuration075x_, decCnt075x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_1_00X, decDuration100x_, decCnt100x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_1_25X, decDuration125x_, decCnt125x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_1_50X, decDuration150x_, decCnt150x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_2_00X, decDuration200x_, decCnt200x_);
        ParseAndAccumulateSpeedData(
            eventMeta, EventInfoExtentedKey::SPEED_DECODING_INFO_3_00X, decDuration300x_, decCnt300x_);
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        if (jsonObj == nullptr) {
            return;
        }
        auto speedRootObj = cJSON_AddObjectToObject(jsonObj.get(), SPEED_DECODING_INFO);
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

        submitInfo.jsonObjects[SPEED_DECODING_INFO] = std::move(jsonObj);
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

class SecureDecoderAllocBufFailedInfo : public StatisticsMainEventInfoBase {
public:
    SecureDecoderAllocBufFailedInfo() = default;

    void OnAddEventInfo(StatisticsEventType eventType, const Media::Meta &eventMeta) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        AppNameIndex appIndex = AppNameIndexInfo::GetInstance().GetIndexByMeta(eventMeta);
        if (appIndex == INVALID_APP_NAME_INDEX) {
            return;
        }
        std::string mimeType = GetMetaString(eventMeta, Media::Tag::MIME_TYPE, "UNKNOWN");
        CreateOrIncrementMapCount(secureDecoderAllocBufFailedInfo_[appIndex], mimeType);
    }

    void OnSummateEventInfo(StatisticsSubmitInfo &submitInfo) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto jsonObj = std::shared_ptr<cJSON>(cJSON_CreateObject(), cJSON_Delete);
        for (const auto &[appIndex, mimeInfo] : secureDecoderAllocBufFailedInfo_) {
            auto appObj = cJSON_AddObjectToObject(jsonObj.get(), std::to_string(appIndex).c_str());
            AddMapToJsonObject(appObj, mimeInfo);
        }
        submitInfo.jsonObjects[SECURE_DEC_ALLOC_BUF_FAILED_INFO] = std::move(jsonObj);
    }

    void ResetEventInfo() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        secureDecoderAllocBufFailedInfo_.clear();
    }

private:
    std::unordered_map<AppNameIndex, std::unordered_map<std::string, uint32_t>> secureDecoderAllocBufFailedInfo_;
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
            mimeType.size() <= MAX_STATISTICS_KEY_STRING_LENGTH &&
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
        std::lock_guard<std::mutex> lock(mutex_);
        auto codecErrorRootObj = cJSON_AddObjectToObject(jsonObj.get(), CODEC_ERROR_INFO);
        for (const auto &[codecErrorKey, count] : errorCodeInfo_) {
            const auto &[vcodecType, mimeType, errorCode] = codecErrorKey;
            if (vcodecType <= VideoCodecType::UNKNOWN || vcodecType >= VideoCodecType::END || mimeType.empty()) {
                continue;
            }
            auto typeIter = VIDEO_CODEC_TYPE_TO_STRING.find(vcodecType);
            if (typeIter == VIDEO_CODEC_TYPE_TO_STRING.end()) {
                continue;
            }
            std::string key = typeIter->second + "_" + mimeType + "_Err_" +
                AVCSErrorToEnumName(static_cast<AVCodecServiceErrCode>(errorCode));
            cJSON_AddNumberToObject(codecErrorRootObj, key.c_str(), count);
        }

        submitInfo.jsonObjects[CODEC_ERROR_INFO] = std::move(jsonObj);
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
        std::lock_guard<std::shared_mutex> lock(eventHookMutex_);
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

    if (IsEventValid()) {
        HiSysEventWrite(
            EVENT_DOMAIN, EVENT_STATISTICS_INFO,
            OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
            QUERY_CAP_TIMES,                    submitInfo.queryCapTimes,
            CREATE_CODEC_TIMES,                 submitInfo.createCodecTimes,
            CODEC_SPECIFIED_INFO,               getJsonField(CODEC_SPECIFIED_INFO).CStr(),
            APP_NAME_DICT,                      getJsonField(APP_NAME_DICT).CStr(),
            CAP_UNSUPPORTED_INFO,               getJsonField(CAP_UNSUPPORTED_INFO).CStr(),
            DEC_ABNORMAL_OCCUPATION_INFO,       getJsonField(DEC_ABNORMAL_OCCUPATION_INFO).CStr(),
            SPEED_DECODING_INFO,                getJsonField(SPEED_DECODING_INFO).CStr(),
            CODEC_ERROR_INFO,                   getJsonField(CODEC_ERROR_INFO).CStr(),
            CODEC_INSTANCE_USAGE_INFO,          getJsonField(CODEC_INSTANCE_USAGE_INFO).CStr(),
            RESOLUTION_SPEC_INFO,               getJsonField(RESOLUTION_SPEC_INFO).CStr(),
            CONTAINER_FORMAT_INFO,               getJsonField(CONTAINER_FORMAT_INFO).CStr(),
            CODEC_MODE_INFO,                    getJsonField(CODEC_MODE_INFO).CStr(),
            ADVANCED_FEATURE_INFO,              getJsonField(ADVANCED_FEATURE_INFO).CStr(),
            SECURE_DEC_ALLOC_BUF_FAILED_INFO, getJsonField(SECURE_DEC_ALLOC_BUF_FAILED_INFO).CStr()
        );
    }

    {
        std::lock_guard<std::shared_mutex> eventHookLock(eventHookMutex_);
        eventHooks_.clear();
    }
    AppNameIndexInfo::GetInstance().Reset();
    for (auto &eventInfo : eventInfoMap_) {
        eventInfo.second->ResetEventInfo();
    }
    RegisterSubmitEventTimer();
}

void StatisticsEventInfo::RegisterEventHook(StatisticsEventType eventType, EventHook hook)
{
    std::lock_guard<std::shared_mutex> lock(eventHookMutex_);
    eventHooks_[eventType] = std::move(hook);
}

void StatisticsEventInfo::RegisterSubmitEventTimer()
{
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

bool StatisticsEventInfo::IsEventValid()
{
    for (auto &[eventType, eventInfo] : eventInfoMap_) {
        if (eventInfo != nullptr && eventInfo->IsEventValid()) {
            return true;
        }
    }
    return false;
}
} // namespace MediaAVCodec
} // namespace OHOS
