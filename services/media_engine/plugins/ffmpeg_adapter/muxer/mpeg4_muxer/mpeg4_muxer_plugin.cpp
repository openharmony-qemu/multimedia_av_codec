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

#include "mpeg4_muxer_plugin.h"
#include <functional>
#include <sys/types.h>
#include <algorithm>
#include <fcntl.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <regex>
#ifndef _WIN32
#include <unistd.h>
#include "securec.h"
#endif // !_WIN32
#include "avcodec_common.h"
#include "avcodec_mime_type.h"
#include "mpeg4_utils.h"
#include "audio_track.h"
#include "video_track.h"
#include "cover_track.h"
#include "timed_meta_track.h"
#include "meta/mime_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_MUXER, "Mpeg4MuxerPlugin"};
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace Plugins;
using namespace Mpeg4;

constexpr float LATITUDE_MIN = -90.0f;
constexpr float LATITUDE_MAX = 90.0f;
constexpr float LONGITUDE_MIN = -180.0f;
constexpr float LONGITUDE_MAX = 180.0f;
constexpr float ALTITUDE_MIN = -32768.0f;
constexpr float ALTITUDE_MAX = 32767.0f;
constexpr int32_t MAX_USERMETA_STRING_LENGTH = 256;

const std::vector<TagType> g_gltfTags = {
    Tag::MEDIA_GLTF_VERSION,
    Tag::MEDIA_GLTF_DATA,
    Tag::MEDIA_GLTF_ITEM_NAME,
    Tag::MEDIA_GLTF_CONTENT_TYPE,
    Tag::MEDIA_GLTF_CONTENT_ENCODING,
    Tag::MEDIA_GLTF_ITEM_TYPE
};

std::map<std::string, uint32_t> g_supportedMuxer = {
    {"Mpeg4Mux_mp4", OHOS::MediaAVCodec::OUTPUT_FORMAT_MPEG_4},
    {"Mpeg4Mux_m4a", OHOS::MediaAVCodec::OUTPUT_FORMAT_M4A}
};

Status RegisterMuxerPlugins(const std::shared_ptr<Register>& reg)
{
    for (auto &muxer : g_supportedMuxer) {
        MuxerPluginDef def;
        def.name = muxer.first;
        def.description = "mpeg4 muxer";
        def.rank = 100;  // 100
        def.SetCreator([](const std::string& name) -> std::shared_ptr<MuxerPlugin> {
            return std::make_shared<Mpeg4MuxerPlugin>(name);
        });
        if (reg->AddPlugin(def) != Status::NO_ERROR) {
            MEDIA_LOG_W("fail to add plugin " PUBLIC_LOG_S, muxer.first.c_str());
            continue;
        }
    }
    return Status::NO_ERROR;
}

PLUGIN_DEFINITION(Mpeg4Muxer, LicenseType::APACHE_V2, RegisterMuxerPlugins, [] {})


std::string GetCurrentDate() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    // 转换为time_t
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    // 转换为tm结构（本地时间）
    std::tm* localTime = std::localtime(&timeNow);
    
    // 使用stringstream格式化日期
    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d ");
    
    return oss.str();
}

uint64_t String2Uint64Timestamp(std::string &creationTime)
{
    std::tm tm = {};
    // 去除00:00:00.000毫秒部分 -> 00:00:00
    std::string tmpCreationTime = std::regex_replace(creationTime, std::regex(R"(\.\d+)"), "");
    // 如果只输入时分秒，日期设置为今天
    if (std::regex_match(creationTime, std::regex(R"((\d{1,2})(:\d{1,2}){0,2})"))) {
        tmpCreationTime = GetCurrentDate() + tmpCreationTime;
    }
    MEDIA_LOG_I("mpeg4 muxer input time:%{public}s, %{public}s", creationTime.c_str(), tmpCreationTime.c_str());
    constexpr uint64_t timeSec = 0x7C25B080;  // 1904-01-01 00:00:00 UTC

    static const std::vector<std::string> timeFormatList = {
        R"(%Y-%m-%d T %H:%M:%S Z)", // ISO 8601 UTC
        R"(%Y-%m-%d T %H:%M:%S)",   // ISO 8601 无时区
        R"(%Y-%m-%d %H:%M:%S)",
        R"(%Y/%m/%d %H:%M:%S)",
    };
    bool success = false;
    for (auto &format : timeFormatList) {
        std::istringstream iss(tmpCreationTime);
        iss >> std::get_time(&tm, format.c_str());
        if (!iss.fail()) {
            success = true;
            break;
        }
    }
    // 时间小于1904-01-01 00:00:00 UTC 用time now
    if (success) {
        // 带 z 则用UTC时间戳
        uint64_t timestamp = std::regex_search(creationTime, std::regex("[zZ]")) ?
            static_cast<uint64_t>(timegm(&tm)) : static_cast<uint64_t>(std::mktime(&tm));
        if (static_cast<int64_t>(timeSec) + static_cast<int64_t>(timestamp) >= 0) {
            tm.tm_isdst = -1;
            return timeSec + timestamp;
        }
        MEDIA_LOG_W("mpeg4 muxer input time:%{public}s is earlier than 1904-01-01 00:00:00 UTC", creationTime.c_str());
    }

    MEDIA_LOG_I("mpeg4 muxer input time:%{public}s format not support, use time now", creationTime.c_str());
    // 获取当前系统时间（自1970-01-01以来的微秒/纳秒数）
    auto now = std::chrono::system_clock::now();
    // 转换为自1970-01-01以来的秒数
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    // 转自1904年开始
    return timeSec + static_cast<uint64_t>(seconds.count());
}

void AppendMeta(const std::shared_ptr<Meta> &src, std::shared_ptr<Meta> &dest)
{
    std::vector<TagType> keys;
    int32_t dataInt = 0;
    float dataFloat = 0.0f;
    std::string dataStr = "";
    std::vector<uint8_t> dataBinary;

    src->GetKeys(keys);
    for (auto& k: keys) {
        if (src->GetData(k, dataInt)) {
            dest->SetData(k, dataInt);
        } else if (src->GetData(k, dataFloat)) {
            dest->SetData(k, dataFloat);
        } else if (src->GetData(k, dataStr)) {
            dest->SetData(k, dataStr);
        } else if (src->GetData(k, dataBinary)) {
            dest->SetData(k, dataBinary);
        } else {
            MEDIA_LOG_D("the value type of key: %{public}s is not supported!", k.c_str());
            continue;
        }
    }
}
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
Mpeg4MuxerPlugin::Mpeg4MuxerPlugin(std::string name)
    : MuxerPlugin(std::move(name))
{
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    outputFormat_ = g_supportedMuxer[pluginName_];
    boxParser_ = std::make_shared<BoxParser>(outputFormat_);
    moov_ = boxParser_->MoovBoxGenerate();
    param_ = std::make_shared<Meta>();
    userMeta_ = std::make_shared<Meta>();
}

Mpeg4MuxerPlugin::~Mpeg4MuxerPlugin()
{
    MEDIA_LOG_D("Destory");
    io_ = nullptr;
    moov_ = nullptr;
    boxParser_ = nullptr;
    tracks_.clear();
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

Status Mpeg4MuxerPlugin::SetDataSink(const std::shared_ptr<DataSink> &dataSink)
{
    FALSE_RETURN_V_MSG_E(dataSink != nullptr, Status::ERROR_INVALID_PARAMETER, "data sink is null");
    io_ = std::make_shared<AVIOStream>(dataSink);
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::SetParameter(const std::shared_ptr<Meta> &param)
{
    int32_t dataInt = 0;
    std::string dataStr;
    if (param->GetData(Tag::MEDIA_EDITLIST, dataInt) && dataInt == 0) {
        boxParser_->SetEdtsBoxFlag(false);
        MEDIA_LOG_I("close edit list (delete edts box)");
    }
    if (param->GetData(Tag::MEDIA_AIGC, dataStr) && dataStr.size() > 0) {
        userMeta_->SetData(Tag::MEDIA_AIGC, dataStr);
        hasAigc_ = true;
        MEDIA_LOG_I("include aigc");
    }
    if (param->GetData("use_timed_meta_track", dataInt) && dataInt == 1) {
        useTimedMeta_ = true;
        MEDIA_LOG_I("use timed metadata track");
    }
    FALSE_RETURN_V_MSG_E(param != nullptr, Status::ERROR_INVALID_PARAMETER, "param is null");
    Status ret = Status::NO_ERROR;
    ret = SetRotation(param);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "SetRotation failed");
    ret = SetLocation(param);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "SetLocation failed");
    AppendMeta(param, param_);
    return ret;
}

Status Mpeg4MuxerPlugin::SetRotation(const std::shared_ptr<Meta> &param)
{
    if (param->Find(Tag::VIDEO_ROTATION) != param->end()) {
        param->Get<Tag::VIDEO_ROTATION>(rotation_); // rotation
        if (rotation_ != VIDEO_ROTATION_0 && rotation_ != VIDEO_ROTATION_90 &&
            rotation_ != VIDEO_ROTATION_180 && rotation_ != VIDEO_ROTATION_270) {
            MEDIA_LOG_W("Invalid rotation: %{public}d, keep default 0", rotation_);
            rotation_ = VIDEO_ROTATION_0;
            return Status::ERROR_INVALID_DATA;
        }
    }
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::SetLocation(const std::shared_ptr<Meta> &param)
{
    float latitude = latitude_;
    float longitude = longitude_;
    float altitude = altitude_;
    if (param->Find(Tag::MEDIA_LATITUDE) == param->end() &&
        param->Find(Tag::MEDIA_LONGITUDE) == param->end()) {
        return Status::NO_ERROR;
    } else if (param->Find(Tag::MEDIA_LATITUDE) != param->end() &&
        param->Find(Tag::MEDIA_LONGITUDE) != param->end()) {
        param->Get<Tag::MEDIA_LATITUDE>(latitude); // latitude
        param->Get<Tag::MEDIA_LONGITUDE>(longitude); // longitude
    } else {
        MEDIA_LOG_W("the latitude and longitude are all required");
        return Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_MSG_E(latitude >= LATITUDE_MIN && latitude <= LATITUDE_MAX,
        Status::ERROR_INVALID_DATA, "latitude must be in [-90, 90]!");
    FALSE_RETURN_V_MSG_E(longitude >= LONGITUDE_MIN && longitude <= LONGITUDE_MAX,
        Status::ERROR_INVALID_DATA, "longitude must be in [-180, 180]!");
    if (param->Find(Tag::MEDIA_ALTITUDE) != param->end()) {
        param->Get<Tag::MEDIA_ALTITUDE>(altitude); // altitude
        if (altitude >= ALTITUDE_MIN && altitude <= ALTITUDE_MAX) {
            MEDIA_LOG_I("has altitude.");
            hasAltitude_ = true;
            altitude_ = altitude;
        } else {
            MEDIA_LOG_W("The type of altitude is float, must be in [-32768.0, 32767.0]");
        }
    }
    latitude_ = latitude;
    longitude_ = longitude;
    MEDIA_LOG_I("set latitude and longitude successfully");
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::SetCreationTime(const std::shared_ptr<Meta> &param)
{
    auto mvhdBox = BasicBox::GetBoxPtr<MvhdBox>(moov_, "mvhd");
    FALSE_RETURN_V_MSG_E(mvhdBox != nullptr, Status::ERROR_INVALID_DATA, "mvhd box is empty");
    std::string creationTime;
    if (!param->GetData(Tag::MEDIA_CREATION_TIME, creationTime)) {
        creationTime = "now";
    }
    mvhdBox->creationTime_ = String2Uint64Timestamp(creationTime);
    mvhdBox->modificationTime_ = mvhdBox->creationTime_;
    for (auto& track : tracks_) {
        track->SetCreationTime(mvhdBox->creationTime_, mvhdBox->creationTime_);
    }
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::SetUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    std::vector<std::string> keys;
    userMeta->GetKeys(keys);
    FALSE_RETURN_V_MSG_E(keys.size() > 0, Status::ERROR_INVALID_DATA, "user meta is empty!");
    for (auto& k: keys) {
        if (k.compare(0, 16, "com.openharmony.") != 0) { // 16 "com.openharmony." length
            MEDIA_LOG_W("the meta key %{public}s must com.openharmony.xxx!", k.c_str());
            continue;
        }
        std::string value = "";
        int32_t dataInt = 0;
        float dataFloat = 0.0f;
        std::string dataStr = "";
        std::vector<uint8_t> dataBinary;
        if (userMeta->GetData(k, dataInt)) {
            userMeta_->SetData(k, dataInt);
        } else if (userMeta->GetData(k, dataFloat)) {
            userMeta_->SetData(k, dataFloat);
        } else if (userMeta->GetData(k, dataStr)) {
            if (dataStr.length() > MAX_USERMETA_STRING_LENGTH) {
                MEDIA_LOG_E("the usermeta key %{public}s string value length %{public}zu more than 256 characters.",
                    k.c_str(), dataStr.length());
                return Status::ERROR_INVALID_DATA;
            }
            userMeta_->SetData(k, dataStr);
        } else if (userMeta->GetData(k, dataBinary)) {
            userMeta_->SetData(k, dataBinary);
        } else {
            MEDIA_LOG_E("the value type of meta key %{public}s is not supported!", k.c_str());
            continue;
        }
    }
    return Status::NO_ERROR;
}

bool Mpeg4MuxerPlugin::CheckGltfParam(std::shared_ptr<Meta> &param)
{
    for (const auto& tag : g_gltfTags) {
        if (param->Find(tag) == param->end()) {
            return false;
        }
    }
    return true;
}

Status Mpeg4MuxerPlugin::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    FALSE_RETURN_V_MSG_E(boxParser_ != nullptr, Status::ERROR_INVALID_OPERATION, "box parser is not set");
    FALSE_RETURN_V_MSG_E(mdatPos_ == 0, Status::ERROR_WRONG_STATE, "AddTrack failed! muxer has start!");
    constexpr int32_t mimeTypeLen = 5;  // 5
    Status ret = Status::NO_ERROR;
    std::string mimeType = "";
    std::shared_ptr<BasicTrack> track;
    FALSE_RETURN_V_MSG_E(trackDesc->Get<Tag::MIME_TYPE>(mimeType), Status::ERROR_INVALID_PARAMETER,
        "get mimeType failed!"); // mime
    MEDIA_LOG_D("mimeType is %{public}s", mimeType.c_str());
    if (!mimeType.compare(0, mimeTypeLen, "audio")) {
        track = std::make_shared<AudioTrack>(mimeType, moov_, tracks_);
    } else if (!mimeType.compare(0, mimeTypeLen, "video")) {
        track = std::make_shared<VideoTrack>(mimeType, moov_, tracks_);
    } else if (!mimeType.compare(0, mimeTypeLen, "image")) {
        track = std::make_shared<CoverTrack>(mimeType, moov_, hasAigc_);
    } else if (!mimeType.compare(0, mimeTypeLen - 1, "meta")) {
        track = std::make_shared<TimedMetaTrack>(mimeType, moov_, tracks_, useTimedMeta_);
    } else {
        MEDIA_LOG_W("mimeType %{public}s is unsupported", mimeType.c_str());
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    ret = track->Init(trackDesc);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "add track failed!");
    if (track->GetMediaType() == MediaType::AUDIO ||
        track->GetMediaType() == MediaType::VIDEO ||
        track->GetMediaType() == MediaType::TIMEDMETA) {
        boxParser_->AddTrakBox(track);
        FALSE_RETURN_V_MSG_E(track->GetTrackId() > 0, Status::ERROR_INVALID_DATA, "track id must be > 0");
    }
    if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC) == 0) {
        isH264_ = true;
    }
    trackIndex = static_cast<int32_t>(tracks_.size());
    tracks_.emplace_back(track);
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::Start()
{
    FALSE_RETURN_V_MSG_E(mdatPos_ == 0, Status::ERROR_WRONG_STATE, "Start failed! muxer has start!");
    FALSE_RETURN_V_MSG_E(io_ != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    hasGltf_ = CheckGltfParam(param_);
    return WriteHeader();
}

Status Mpeg4MuxerPlugin::WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(mdatPos_ > 0, Status::ERROR_WRONG_STATE, "WriteSample failed! Did not write header!");
    FALSE_RETURN_V_MSG_E(io_ != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr && sample->memory_->GetAddr() != nullptr,
        Status::ERROR_NULL_POINTER, "sample is null!");
    FALSE_RETURN_V_MSG_E(trackIndex < tracks_.size(), Status::ERROR_INVALID_PARAMETER, "track index is invalid!");
    return tracks_[trackIndex]->WriteSample(io_, sample);
}

Status Mpeg4MuxerPlugin::Stop()
{
    FALSE_RETURN_V_MSG_E(mdatPos_ > 0, Status::ERROR_WRONG_STATE, "Stop failed! Did not write header!");
    FALSE_RETURN_V_MSG_E(io_ != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    FALSE_RETURN_V_MSG_E(moov_ != nullptr, Status::ERROR_INVALID_OPERATION, "moov is not set");
    for (auto &track : tracks_) {
        track->UpdateUserMeta(userMeta_);
    }
    WriteMdatSize();
    SetCreationTime(param_);
    boxParser_->AddMoovUdtaBox();
    if (hasAigc_) {
        boxParser_->AddGnreBox(param_, !userMeta_->Empty(), "udta");  // gnre box
        boxParser_->AddMoovUdtaGeoTag(latitude_, longitude_, !userMeta_->Empty(), altitude_, hasAltitude_);
        boxParser_->AddUserMetaBox(userMeta_, "udta");  // user meta box, moov.udta.meta
    } else {
        boxParser_->AddMoovUdtaGeoTag(latitude_, longitude_, !userMeta_->Empty(), altitude_, hasAltitude_);
        boxParser_->AddMoovUdtaMetaBox();  // moov.udta.meta
        boxParser_->AddIlstMetaData(param_);  // moov.udta.meta.ilst
        boxParser_->AddGnreBox(param_, !userMeta_->Empty());  // gnre box
        boxParser_->AddUserMetaBox(userMeta_);  // user meta box, moov.meta
    }
    boxParser_->AddMoovUdtaLociBox(latitude_, longitude_, altitude_, hasAltitude_);
    WriteFileLevelMetafBox();
    return WriteTailer();
}

Status Mpeg4MuxerPlugin::WriteHeader()
{
    FALSE_RETURN_V_MSG_E(boxParser_ != nullptr, Status::ERROR_INVALID_OPERATION, "box parser is not set");
    constexpr int32_t typeLen = 4;
    for (auto &track : tracks_) {
        track->SetRotation(rotation_);
    }
    io_->Seek(0);
    io_->Write(static_cast<uint32_t>(0));
    io_->Write("ftyp", typeLen);
    if (outputFormat_ == OUTPUT_FORMAT_M4A) {
        io_->Write("M4A ", typeLen);
    } else {
        io_->Write("mp42", typeLen);
    }
    io_->Write(static_cast<uint32_t>(0));
    if (outputFormat_ == OUTPUT_FORMAT_M4A) {
        io_->Write("M4A ", typeLen);
        io_->Write("isom", typeLen);
        io_->Write("iso2", typeLen);
    } else {
        io_->Write("iso2", typeLen);
        if (isH264_) {
            io_->Write("avc1", typeLen);
        }
        io_->Write("mp42", typeLen);
        if (hasGltf_) {
            io_->Write("glti", typeLen);
        }
    }
    freePos_ = io_->GetPos();
    io_->Seek(0);
    io_->Write(static_cast<uint32_t>(freePos_));
    io_->Seek(freePos_);
    io_->Write(static_cast<uint32_t>(8));  // 8
    io_->Write("free", typeLen);
    mdatPos_ = io_->GetPos();
    io_->Write(static_cast<uint32_t>(0));
    io_->Write("mdat", typeLen);
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::WriteTailer()
{
    for (auto &track : tracks_) {
        track->WriteTailer();
    }
    int32_t isMoovFront = 0;
    if (param_->GetData(Tag::MEDIA_ENABLE_MOOV_FRONT, isMoovFront) && isMoovFront == 1 && io_->CanRead()) {
        return MoveMoovBoxToFront(needFree_);
    }
    if (isMoovFront == 1 && !io_->CanRead()) {
        MEDIA_LOG_W("do not have read permission. cancel move moov to front!");
    }
    uint32_t moovSize = moov_->GetSize();
    io_->InitCache(moovSize);
    moov_->Write(io_);
    io_->InitCache(0);
    moov_ = nullptr;
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::MoveMoovBoxToFront(bool isNeedFree)
{
    uint32_t moovSize = moov_->GetSize();
    uint32_t freeSize = isNeedFree ? 8 : 0; // 需要补齐free box 8
    moovSize += freeSize;
    std::sort(tracks_.begin(), tracks_.end(), BasicTrack::Compare);
    for (auto &track : tracks_) {
        uint32_t  increments = track->MoovIncrements(moovSize);
        if (increments == 0) {
            break;
        }
        moovSize += increments;
    }
    uint32_t offset = moov_->GetSize() + freeSize;
    MEDIA_LOG_I("check moov size %{public}d - %{public}d", moovSize, offset);
    for (auto& track : tracks_) {
        track->SetOffset(offset);
    }
    auto io = std::make_shared<AVIOStream>(*io_.get());
    io->Seek(freePos_);
    io_->Seek(freePos_);

    std::vector<std::pair<uint8_t*, int32_t>> buffer = {{nullptr, 0}, {nullptr, 0}};
    buffer[0].first = new uint8_t[offset];
    buffer[0].second = io->Read(buffer[0].first, offset);
    io_->InitCache(moovSize);
    moov_->Write(io_);
    io_->InitCache(0);
    if (isNeedFree) {
        io_->Write(static_cast<uint32_t>(8));  // 8
        io_->Write("free", 4);  // 4
    }
    tracks_.clear();
    moov_ = nullptr;
    boxParser_ = nullptr;
    buffer[1].first = new uint8_t[offset];

    int32_t i = 0;
    while (buffer[i].second > 0) {
        buffer[(i + 1) % 2].second = io->Read(buffer[(i + 1) % 2].first, offset);  // 2
        io_->Write(buffer[i].first, buffer[i].second);
        i = (i + 1) % 2;  // 2
    }
    delete[] buffer[0].first;
    delete[] buffer[1].first;
    return Status::NO_ERROR;
}

void Mpeg4MuxerPlugin::WriteMdatSize()
{
    needFree_ = false;
    int64_t pos = io_->GetPos();
    uint64_t mdatSize = static_cast<uint64_t>(pos - mdatPos_);
    if (mdatSize <= UINT32_MAX) {
        io_->Seek(mdatPos_);
        io_->Write(static_cast<uint32_t>(mdatSize));
    } else {
        needFree_ = true;
        io_->Seek(freePos_);
        io_->Write(static_cast<uint32_t>(1));
        io_->Write("mdat", 4);  // 4
        io_->Write(mdatSize + 8);  // 8
    }
    io_->Seek(pos);
}

void Mpeg4MuxerPlugin::WriteFileLevelMetafBox()
{
    if (!hasGltf_) {
        return;
    }
    auto metaBox = boxParser_->FileLevelMetaBoxGenerate(param_);
    metaBox->GetSize();
    metaBox->Write(io_);
}
} // Mpeg4
} // namespace Plugins
} // Media
} // OHOS
