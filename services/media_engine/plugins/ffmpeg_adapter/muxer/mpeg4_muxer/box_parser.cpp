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

#include "box_parser.h"
#include "avcodec_mime_type.h"
#include <sstream>
#include <iomanip>
#ifndef _WIN32
#include "securec.h"
#endif // !_WIN32
#include "audio_track.h"
#include "video_track.h"
#include "timed_meta_track.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "BoxParser" };

constexpr int32_t BOX_TYPE_LEN = 4;
constexpr int32_t BOX_FLAG_LEN = 3;
constexpr float LATITUDE_MIN = -90.0f;
constexpr float LATITUDE_MAX = 90.0f;
constexpr float LONGITUDE_MIN = -180.0f;
constexpr float LONGITUDE_MAX = 180.0f;
uint8_t g_flag[BOX_FLAG_LEN] = {0, 0, 0};
}

using namespace OHOS::MediaAVCodec;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
BoxParser::BoxParser(uint32_t fileType) : fileType_(fileType)
{
}

BoxParser::~BoxParser()
{
}

std::shared_ptr<BasicBox> BoxParser::MoovBoxGenerate()
{
    if (moov_ != nullptr) {
        MEDIA_LOG_D("moov already exists");
        return moov_;
    }

    moov_ = std::make_shared<BasicBox>(0, "moov");
    moov_->AddChild(MvhdBoxGenerate());
    return moov_;
}

void BoxParser::AddTrakBox(std::shared_ptr<BasicTrack> track)
{
    FALSE_RETURN_MSG(track != nullptr, "track is empty");
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    auto mvhdBox = BasicBox::GetBoxPtr<MvhdBox>(moov_, "mvhd");
    FALSE_RETURN_MSG(mvhdBox != nullptr && mvhdBox->nextTrackId_ < INT16_MAX, "track id is out of range"); // max int16
    track->SetTrackId(static_cast<int32_t>(mvhdBox->nextTrackId_));
    moov_->AddChild(TrakBoxGenerate(track));
    mvhdBox->nextTrackId_++;
}

void BoxParser::AddMoovUdtaLociBox(float latitude, float longitude, float altitude, bool hasAltitude)
{
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    FALSE_RETURN_MSG_I(latitude >= LATITUDE_MIN && latitude <= LATITUDE_MAX && longitude >= LONGITUDE_MIN &&
        longitude <= LONGITUDE_MAX, "latitude or longitude not in range or not set");
    auto udtaBox = moov_->GetChild("udta");
    FALSE_RETURN_MSG(udtaBox != nullptr, "udta box is empty");
    std::shared_ptr<LociBox> lociBox = std::make_shared<LociBox>(0, "loci");
    lociBox->latitude_ = static_cast<int32_t>((1 << 16) * static_cast<double>(latitude));  // 16
    lociBox->longitude_ = static_cast<int32_t>((1 << 16) * static_cast<double>(longitude));  // 16
    if (hasAltitude) {
        lociBox->altitude_ = static_cast<int32_t>((1 << 16) * static_cast<double>(altitude));  // 16
    }
    udtaBox->AddChild(lociBox);
}

std::shared_ptr<BasicBox> BoxParser::MvhdBoxGenerate()
{
    std::shared_ptr<MvhdBox> mvhdBox = std::make_shared<MvhdBox>(108, "mvhd");  // boxSize: 108
    mvhdBox->creationTime_ = 0;
    mvhdBox->modificationTime_ = 0;
    mvhdBox->duration_ = 0;
    mvhdBox->nextTrackId_ = 1;
    return mvhdBox;
}

void BoxParser::AddMoovUdtaBox()
{
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    std::shared_ptr<BasicBox> utdaBox = std::make_shared<BasicBox>(0, "udta");
    moov_->AddChild(utdaBox);
}

void BoxParser::AddMoovUdtaMetaBox()
{
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    auto udtaBox = moov_->GetChild("udta");
    FALSE_RETURN_MSG(udtaBox != nullptr, "udta box is empty");
    std::shared_ptr<FullBox> metaBox = std::make_shared<FullBox>(0, "meta");
    std::shared_ptr<HdlrBox> hdlrBox = std::make_shared<HdlrBox>(0, "hdlr");
    hdlrBox->handlerType_ = 'mdir';
    hdlrBox->reserved_[0] = 'ohav';
    metaBox->AddChild(hdlrBox);
    metaBox->AddChild(IlstBoxGenerate());
    udtaBox->AddChild(metaBox);
}

std::shared_ptr<BasicBox> BoxParser::IlstBoxGenerate()
{
    std::shared_ptr<BasicBox> ilstBox = std::make_shared<BasicBox>(0, "ilst");
    std::shared_ptr<BasicBox> tooBox = std::make_shared<BasicBox>(0, "\xA9too");
    std::shared_ptr<DataBox> dataBox = std::make_shared<DataBox>(0, "data");
    dataBox->dataType_ = 1;
    dataBox->default_ = 0;
    std::string tool = "Openharmony6.1";
    dataBox->data_.reserve(tool.length());
    dataBox->data_.assign(tool.data(), tool.data() + tool.length());
    tooBox->AddChild(dataBox);
    ilstBox->AddChild(tooBox);
    return ilstBox;
}

void BoxParser::AddIlstMetaData(const std::shared_ptr<Meta> &param)
{
    FALSE_RETURN_MSG_I(!param->Empty(), "param is empty");
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    constexpr uint32_t boxElemSize = 16;
    static const std::map<TagType, std::string> table = {
        {Tag::MEDIA_TITLE, "\251nam"},
        {Tag::MEDIA_ARTIST, "\251ART"},
        {Tag::MEDIA_ALBUM, "\251alb"},
        {Tag::MEDIA_ALBUM_ARTIST, "aART"},
        {Tag::MEDIA_DATE, "\251day"},
        {Tag::MEDIA_COMMENT, "\251cmt"},
        {Tag::MEDIA_GENRE, "\251gen"},
        {Tag::MEDIA_COPYRIGHT, "cprt"},
        {Tag::MEDIA_DESCRIPTION, "desc"},
        {Tag::MEDIA_LYRICS, "\251lyr"},
        {Tag::MEDIA_COMPOSER, "\251wrt"},
    };
    auto ilstBox = moov_->GetChild("udta.meta.ilst");
    FALSE_RETURN_MSG(ilstBox != nullptr, "ilst box is null!");
    std::string value;
    for (auto& key: table) {
        if (param->GetData(key.first, value)) {
            uint32_t boxSize = boxElemSize + static_cast<uint32_t>(value.length());
            std::shared_ptr<AnyBox> tagBox = std::make_shared<AnyBox>(0, key.second);
                tagBox->SetFunc([value, boxSize] (std::shared_ptr<AVIOStream> io) {
                        io->Write(boxSize);
                        io->Write("data", BOX_TYPE_LEN);
                        io->Write(static_cast<uint32_t>(1));
                        io->Write(static_cast<uint32_t>(0));
                        io->Write(value.data(), static_cast<int32_t>(value.length()));
                    }, [boxSize] () {
                        return boxSize;
                    });
            ilstBox->AddChild(tagBox);
        }
    }
}

void BoxParser::AddGnreBox(const std::shared_ptr<Meta> &param, bool needGenerate, std::string path)
{
    FALSE_RETURN_MSG_I(needGenerate, "user meta and aigc not set, not generate gnre box");
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    std::shared_ptr<BasicBox> parentBox = moov_;
    if (path.length() > 0) {
        parentBox = moov_->GetChild(path);
        FALSE_RETURN_MSG(parentBox != nullptr, "%{public}s box is empty", path.data());
    }
    constexpr uint32_t gnreDataLen = 14;
    std::string dataStr;
    if (param->GetData(Tag::MEDIA_GENRE, dataStr)) {
        std::shared_ptr<AnyBox> gnreBox = std::make_shared<AnyBox>(0, "gnre");
        gnreBox->SetFunc([dataStr] (std::shared_ptr<AVIOStream> io) {
                io->Write(static_cast<uint32_t>(0));
                io->Write(static_cast<uint16_t>(0));
                io->Write(dataStr);
                io->Write(static_cast<uint32_t>(0));
                io->Write(static_cast<uint32_t>(0));
            }, [size = static_cast<uint32_t>(dataStr.size() + 1) + gnreDataLen] () {
                return size;
            });
        parentBox->AddChild(gnreBox);
    }
}

std::string BoxParser::Uint32ToString(uint32_t num)
{
    std::string out {"\0\0\0\0\0", 5};  // 5
    out[3] = static_cast<char>(num & 0xff);  // 3
    out[2] = static_cast<char>((num >> 8) & 0xff);   // 2 8
    out[1] = static_cast<char>((num >> 16) & 0xff);  // 16
    out[0] = static_cast<char>((num >> 24) & 0xff);  // 24
    return out;
}

static std::shared_ptr<BasicBox> MetaKeyGenerate(std::string &type, std::string &key)
{
    std::shared_ptr<AnyBox> keyBox = std::make_shared<AnyBox>(0, type);
    keyBox->SetFunc([key] (std::shared_ptr<AVIOStream> io) {
            io->Write(key.data(), static_cast<uint32_t>(key.length()));
        }, [keyLen = static_cast<uint32_t>(key.length())] () {
            return keyLen;
        });
    return keyBox;
}

static std::shared_ptr<BasicBox> Uint32DataTagGenerate(std::string type, uint32_t dataType, uint32_t data)
{
    constexpr uint32_t boxLen = 20;
    std::shared_ptr<AnyBox> dataBox = std::make_shared<AnyBox>(0, type);
    dataBox->SetFunc([dataType, data, boxLen] (std::shared_ptr<AVIOStream> io) {
            io->Write(boxLen);
            io->Write("data", BOX_TYPE_LEN);
            io->Write(dataType);
            io->Write(static_cast<uint32_t>(0));
            io->Write(data);
        }, [boxLen] () {
            return boxLen;
        });
    return dataBox;
}

static std::shared_ptr<BasicBox> floatDataTagGenerate(std::string type, uint32_t dataType, float data)
{
    union {
        uint32_t u32;
        float f;
    } num;
    num.f = data;
    return Uint32DataTagGenerate(type, dataType, num.u32);
}

static std::shared_ptr<BasicBox> StringDataTagGenerate(std::string type, uint32_t dataType, std::string &data)
{
    constexpr uint32_t dataLen = 16;
    uint32_t boxLen = dataLen + static_cast<uint32_t>(data.length());
    std::shared_ptr<AnyBox> dataBox = std::make_shared<AnyBox>(0, type);
    dataBox->SetFunc([dataType, data, boxLen] (std::shared_ptr<AVIOStream> io) {
            io->Write(boxLen);
            io->Write("data", BOX_TYPE_LEN);
            io->Write(dataType);
            io->Write(static_cast<uint32_t>(0));
            io->Write(data.data(), static_cast<uint32_t>(data.length()));
        }, [boxLen] () {
            return boxLen;
        });
    return dataBox;
}

static std::shared_ptr<BasicBox> Uint8ArrDataTagGenerate(
    std::string type, uint32_t dataType, std::vector<uint8_t> &data)
{
    constexpr uint32_t dataLen = 16;
    uint32_t boxLen = dataLen + static_cast<uint32_t>(data.size());
    std::shared_ptr<AnyBox> dataBox = std::make_shared<AnyBox>(0, type);
    dataBox->SetFunc([dataType, data, boxLen] (std::shared_ptr<AVIOStream> io) {
            io->Write(boxLen);
            io->Write("data", BOX_TYPE_LEN);
            io->Write(dataType);
            io->Write(static_cast<uint32_t>(0));
            io->Write(data.data(), static_cast<uint32_t>(data.size()));
        }, [boxLen] () {
            return boxLen;
        });
    return dataBox;
}

static void SetKeysIlstBox(std::shared_ptr<Meta> userMeta,
    std::shared_ptr<BasicBox> keysBox, std::shared_ptr<BasicBox> ilstBox)
{
    std::vector<std::string> keys;
    std::string dataStr;
    std::string keyType;
    userMeta->GetKeys(keys);
    FALSE_RETURN_MSG(keys.size() > 0, "user meta is empty!");
    uint32_t i = 1;
    for (auto& k: keys) {
        if (k.compare(0, 16, "com.openharmony.") != 0) { // 16 "com.openharmony." length
            MEDIA_LOG_W("the meta key %{public}s must com.openharmony.xxx!", k.c_str());
            continue;
        }

        std::string value = "";
        int32_t dataInt = 0;
        float dataFloat = 0.0f;
        keyType = "mdta" + std::to_string(i);
        std::vector<uint8_t> dataBinary;
        if (userMeta->GetData(k, dataInt)) {
            keysBox->AddChild(MetaKeyGenerate(keyType, k));
            ilstBox->AddChild(Uint32DataTagGenerate(
                BoxParser::Uint32ToString(i), 67, static_cast<uint32_t>(dataInt)));  // 67: type uint32
        } else if (userMeta->GetData(k, dataFloat)) {
            keysBox->AddChild(MetaKeyGenerate(keyType, k));
            ilstBox->AddChild(floatDataTagGenerate(BoxParser::Uint32ToString(i), 23, dataFloat));  // 23: type float
        } else if (userMeta->GetData(k, dataStr)) {
            keysBox->AddChild(MetaKeyGenerate(keyType, k));
            ilstBox->AddChild(StringDataTagGenerate(BoxParser::Uint32ToString(i), 1, dataStr));  // 1: type string
        } else if (userMeta->GetData(k, dataBinary)) {
            keysBox->AddChild(MetaKeyGenerate(keyType, k));
            ilstBox->AddChild(Uint8ArrDataTagGenerate(BoxParser::Uint32ToString(i), 0, dataBinary));  // 0: type binary
        } else {
            MEDIA_LOG_E("the value type of meta key %{public}s is not supported!", k.c_str());
            continue;
        }
        i++;
    }

    // set aigc key and value
    if (userMeta->GetData(Tag::MEDIA_AIGC, dataStr) && dataStr.length() > 0) {
        keyType = "mdta" + std::to_string(i);
        std::string keyName = "AIGC";
        keysBox->AddChild(MetaKeyGenerate(keyType, keyName));
        ilstBox->AddChild(StringDataTagGenerate(BoxParser::Uint32ToString(i), 1, dataStr));  // 1: type string
        i++;
    }
}

void BoxParser::AddUserMetaBox(const std::shared_ptr<Meta> &userMeta, std::string path)
{
    FALSE_RETURN_MSG_I(!userMeta->Empty(), "user meta and aigc not set, not generate user meta box");
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    std::shared_ptr<BasicBox> parentBox = moov_;
    if (path.length() > 0) {
        parentBox = moov_->GetChild(path);
        FALSE_RETURN_MSG(parentBox != nullptr, "%{public}s box is empty", path.data());
    }
    // generate meta box
    std::shared_ptr<FullBox> metaBox = std::make_shared<FullBox>(0, "meta");
    std::shared_ptr<HdlrBox> hdlrBox = std::make_shared<HdlrBox>(0, "hdlr");
    hdlrBox->handlerType_ = 'mdta';

    std::shared_ptr<AnyBox> keysBox = std::make_shared<AnyBox>(0, "keys");
    keysBox->SetFunc([keysBox] (std::shared_ptr<AVIOStream> io) {
            io->Write(static_cast<uint32_t>(0));
            io->Write(static_cast<uint32_t>(keysBox->GetChildCount()));  // entry count
        }, [] () {
            constexpr uint32_t keysBoxSize = 8;
            return keysBoxSize;
        });

    std::shared_ptr<BasicBox> ilstBox = std::make_shared<BasicBox>(0, "ilst");
    SetKeysIlstBox(userMeta, keysBox, ilstBox);

    metaBox->AddChild(hdlrBox);
    metaBox->AddChild(keysBox);
    metaBox->AddChild(ilstBox);
    parentBox->AddChild(metaBox);
}

void BoxParser::AddMoovUdtaGeoTag(float latitude, float longitude, bool needGenerate, float altitude, bool hasAltitude)
{
    FALSE_RETURN_MSG_I(needGenerate, "user meta and aigc not set, not generate geo tag");
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    auto udtaBox = moov_->GetChild("udta");
    FALSE_RETURN_MSG(udtaBox != nullptr, "udta box is empty");
    constexpr int32_t cof = 10000;
    constexpr int32_t maxLat = 900000;
    constexpr int32_t maxLon = 1800000;
    constexpr int32_t latWholeNum = 2;
    constexpr int32_t lonWholeNum = 3;
    constexpr int32_t fractionNum = 4;
    int32_t latitude10000 = static_cast<int32_t>(static_cast<float>(cof) * latitude);
    int32_t longitude10000 = static_cast<int32_t>(static_cast<float>(cof) * longitude);
    FALSE_RETURN_MSG_I(latitude10000 >= (-maxLat) && latitude10000 <= maxLat && longitude10000 >= (-maxLon) &&
        longitude10000 <= maxLon, "location not set or not supported!");
    std::stringstream ss;
    // 0.1 -> +00.1000
    char sign = latitude10000 < 0 ? '-' : '+';
    latitude10000 = latitude10000 < 0 ? (-latitude10000) : latitude10000;
    ss << sign << std::setfill('0') << std::setw(latWholeNum) << (latitude10000 / cof) << "."
        << std::setfill('0') << std::setw(fractionNum) << (latitude10000 % cof);

    // 0.1 -> +000.1000
    sign = longitude10000 < 0 ? '-' : '+';
    longitude10000 = longitude10000 < 0 ? (-longitude10000) : longitude10000;
    ss << sign << std::setfill('0') << std::setw(lonWholeNum) << (longitude10000 / cof) << "."
        << std::setfill('0') << std::setw(fractionNum) << (longitude10000 % cof);

    if (hasAltitude) {
        if (altitude < 0) {
            ss << altitude;
        } else {
            ss << '+' << altitude;
        }
    }

    std::shared_ptr<AnyBox> geoTag = std::make_shared<AnyBox>(0, "\251xyz");
    geoTag->SetFunc([locString = ss.str()] (std::shared_ptr<AVIOStream> io) {
            io->Write(static_cast<uint32_t>(0x001215c7));
            io->Write(locString.c_str(), static_cast<int32_t>(locString.length()));
            io->Write(static_cast<uint8_t>(0x2f));
        }, [strLen = static_cast<uint32_t>(ss.str().length())] () {
            return static_cast<uint32_t>(strLen + sizeof(uint32_t) + 1);
        });
    udtaBox->AddChild(geoTag);
}

std::shared_ptr<BasicBox> BoxParser::TrakBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::string type = "trak";
    type += std::to_string(track->GetTrackId());
    std::shared_ptr<BasicBox> trakBox = std::make_shared<BasicBox>(0, type);
    trakBox->AddChild(TkhdBoxGenerate(track));
    if (enableEdtsBox_) {
        trakBox->AddChild(EdtsBoxGenerate());
    }
    if (track->IsAuxiliary()) {
        trakBox->AddChild(TrefBoxGenerate(track));
    }
    trakBox->AddChild(MdiaBoxGenerate(track));
    track->SetTrackPath(type);
    return trakBox;
}

std::shared_ptr<BasicBox> BoxParser::TkhdBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<TkhdBox> tkhdBox = std::make_shared<TkhdBox>(0, "tkhd");
    tkhdBox->creationTime_ = 0;
    tkhdBox->modificationTime_ = 0;
    tkhdBox->trackIndex_ = static_cast<uint32_t>(track->GetTrackId());
    tkhdBox->duration_ = 0;
    auto mediaType = track->GetMediaType();
    if (mediaType == MediaType::AUDIO) {
        tkhdBox->volume_ = 0x0100;
    } else if (mediaType == MediaType::VIDEO) { // 视频的旋转角度，后边刷新
        auto videoTrack = BasicTrack::GetTrackPtr<VideoTrack>(track);
        if (videoTrack != nullptr) {
            tkhdBox->width_ = static_cast<uint32_t>(videoTrack->GetWidth()) << 16;  // 16
            tkhdBox->height_ = static_cast<uint32_t>(videoTrack->GetHeight()) << 16;  // 16
        }
    }
    return tkhdBox;
}

std::shared_ptr<BasicBox> BoxParser::EdtsBoxGenerate()
{
    std::shared_ptr<BasicBox> edtsBox = std::make_shared<BasicBox>(0, "edts");
    std::shared_ptr<ElstBox> elstBox = std::make_shared<ElstBox>(0, "elst");
    edtsBox->AddChild(elstBox);
    return edtsBox;
}

std::shared_ptr<BasicBox> BoxParser::MdiaBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<BasicBox> mdiaBox = std::make_shared<BasicBox>(0, "mdia");
    mdiaBox->AddChild(MdhdBoxGenerate(track));
    mdiaBox->AddChild(HdlrBoxGenerate(track));
    mdiaBox->AddChild(MinfBoxGenerate(track));
    return mdiaBox;
}

std::shared_ptr<BasicBox>  BoxParser::TrefBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    auto trefIds = track->GetSrcTrackIds();
    FALSE_RETURN_V_MSG_E(trefIds != nullptr, nullptr, "tref box ids is nullptr!");
    std::shared_ptr<TrefBox> trefBox = std::make_shared<TrefBox>(0, "tref");
    trefBox->trefTag_ = track->GetTrefTag();
    trefBox->srcTrackIds_.assign(trefIds->begin(), trefIds->end());
    MEDIA_LOG_I("trefTag:0x%{public}x size: %{public}zu", trefBox->trefTag_, trefIds->size());
    return trefBox;
}

std::shared_ptr<BasicBox> BoxParser::MdhdBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<MdhdBox> mdhdBox = std::make_shared<MdhdBox>(0, "mdhd");
    mdhdBox->creationTime_ = 0;
    mdhdBox->modificationTime_ = 0;
    mdhdBox->timeScale_ = static_cast<uint32_t>(track->GetTimeScale());
    mdhdBox->duration_ = 0;
    return mdhdBox;
}

std::shared_ptr<BasicBox> BoxParser::HdlrBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<HdlrBox> hdlrBox = std::make_shared<HdlrBox>(0, "hdlr");
    hdlrBox->predefined_ = 0;
    hdlrBox->handlerType_ = track->GetHdlrType();
    hdlrBox->name_ = track->GetHdlrName();
    return hdlrBox;
}

std::shared_ptr<BasicBox> BoxParser::MinfBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<BasicBox> minfBox = std::make_shared<BasicBox>(0, "minf");
    auto mediaType = track->GetMediaType();
    if (mediaType == MediaType::AUDIO && !track->IsAuxiliary()) {
        minfBox->AddChild(std::make_shared<SmhdBox>(0, "smhd"));
    } else if (mediaType == MediaType::VIDEO && !track->IsAuxiliary()) {
        minfBox->AddChild(std::make_shared<VmhdBox>(0, "vmhd"));
    } else {
        minfBox->AddChild(std::make_shared<FullBox>(0, "nmhd"));
    }
    std::shared_ptr<BasicBox> dinfBox = std::make_shared<BasicBox>(0, "dinf");
    std::shared_ptr<DrefBox> drefBox = std::make_shared<DrefBox>(0, "dref");
    std::shared_ptr<FullBox> urlBox = std::make_shared<FullBox>(0, "url ", 0, 1);
    drefBox->AddChild(urlBox);
    dinfBox->AddChild(drefBox);
    minfBox->AddChild(dinfBox);
    minfBox->AddChild(StblBoxGenerate(track));
    return minfBox;
}

std::shared_ptr<BasicBox> BoxParser::StblBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<BasicBox> stblBox = std::make_shared<BasicBox>(0, "stbl");
    stblBox->AddChild(StsdBoxGenerate(track));
    auto stts = std::make_shared<SttsBox>(0, "stts");
    auto stss = std::make_shared<StssBox>(0, "stss");
    auto ctts = std::make_shared<SttsBox>(0, "ctts");
    auto stsc = std::make_shared<StscBox>(0, "stsc");
    auto stsz = std::make_shared<StszBox>(0, "stsz");
    auto stco = std::make_shared<StcoBox>(0, "stco");
    track->SetSttsBox(stts);
    stblBox->AddChild(stts);
    if (track->GetMediaType() == MediaType::VIDEO) {
        auto videoTrack = BasicTrack::GetTrackPtr<VideoTrack>(track);
        if (videoTrack != nullptr) {
            videoTrack->SetStssBox(stss);
            videoTrack->SetCttsBox(ctts);
        }
        stblBox->AddChild(stss);
        stblBox->AddChild(ctts);
    }
    track->SetStscBox(stsc);
    track->SetStszBox(stsz);
    track->SetStcoBox(stco);
    stblBox->AddChild(stsc);
    stblBox->AddChild(stsz);
    stblBox->AddChild(stco);
    if (track->GetMimeType().compare(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC) == 0) {
        stblBox->AddChild(std::make_shared<SgpdBox>(0, "sgpd"));
        stblBox->AddChild(std::make_shared<SbgpBox>(0, "sbgp"));
    }
    return stblBox;
}

std::shared_ptr<BasicBox> BoxParser::StsdBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<StsdBox> stsdBox = std::make_shared<StsdBox>(0, "stsd");
    auto mediaType = track->GetMediaType();
    if (mediaType == MediaType::AUDIO) {
        stsdBox->AddChild(AudioBoxGenerate(track));
    } else if (mediaType == MediaType::VIDEO) {
        stsdBox->AddChild(VideoBoxGenerate(track));
    } else if (mediaType == MediaType::TIMEDMETA) {
        stsdBox->AddChild(TimedMetaMebxBoxGenerate(track));
    } else {
        MEDIA_LOG_E("mediaType %{public}d is unsupported", mediaType);
    }
    return stsdBox;
}

std::shared_ptr<BasicBox> BoxParser::AudioBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<AudioBox> audioBox = std::make_shared<AudioBox>(0, "mp4a"); // 当前暂只支持mp4a
    auto audioTrack = BasicTrack::GetTrackPtr<AudioTrack>(track);
    FALSE_RETURN_V_MSG_E(audioTrack != nullptr, nullptr, "AudioBoxGenerate track is null");
    audioBox->channels_ = audioTrack->GetChannels();
    audioBox->sampleRate_ = audioTrack->GetSampleRate();
    audioBox->AddChild(EsdsBoxGenerate(track));
    if (fileType_ == OUTPUT_FORMAT_MPEG_4 || fileType_ == OUTPUT_FORMAT_DEFAULT) {
        audioBox->AddChild(std::make_shared<BtrtBox>(0, "btrt"));
    }
    track->SetCodingType(audioBox->GetType());
    return audioBox;
}

std::shared_ptr<BasicBox> BoxParser::VideoBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    auto videoTrack = BasicTrack::GetTrackPtr<VideoTrack>(track);
    FALSE_RETURN_V_MSG_E(videoTrack != nullptr, nullptr, "VideoBoxGenerate track is null");
    std::string mimeType = videoTrack->GetMimeType();
    std::shared_ptr<VideoBox> videoBox = nullptr;
    if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_MPEG4) == 0) {
        videoBox = std::make_shared<VideoBox>(0, "mp4v");
        videoBox->AddChild(EsdsBoxGenerate(track));
    } else if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC) == 0) {
        videoBox = std::make_shared<VideoBox>(0, "avc1");
        videoBox->AddChild(AvccBoxGenerate(track));
    } else if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) == 0) {
        videoBox = std::make_shared<VideoBox>(0, "hvc1");
        videoBox->AddChild(HvccBoxGenerate(track));
    } else {
        MEDIA_LOG_E("mimeType %{public}s is unsupported", mimeType.c_str());
    }
    if (videoBox != nullptr) {
        videoBox->width_ = static_cast<uint16_t>(videoTrack->GetWidth());
        videoBox->height_ = static_cast<uint16_t>(videoTrack->GetHeight());
        if (videoTrack->IsCuvaHDR() && mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) == 0) {
            std::string name = "CUVA HDR Video";
            videoBox->compressorLen_ = static_cast<uint8_t>(name.length());
            if (memcpy_s(videoBox->compressor_, sizeof(videoBox->compressor_), name.data(), name.length()) != EOK) {
                MEDIA_LOG_W("copy video box compressor failed!");
            }
        }
        videoBox->AddChild(std::make_shared<PaspBox>(0, "pasp"));
        videoBox->AddChild(ColrBoxGenerate(track));
        videoBox->AddChild(CuvvBoxGenerate(track));
        if (fileType_ == OUTPUT_FORMAT_MPEG_4 || fileType_ == OUTPUT_FORMAT_DEFAULT) {
            videoBox->AddChild(std::make_shared<BtrtBox>(0, "btrt"));
        }
        track->SetCodingType(videoBox->GetType());
    }
    return videoBox;
}

std::shared_ptr<BasicBox> BoxParser::TimedMetaMebxBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    auto timedMetaTrack = BasicTrack::GetTrackPtr<TimedMetaTrack>(track);
    FALSE_RETURN_V_MSG_E(timedMetaTrack != nullptr, nullptr, "get timed meta track failed!");
    std::shared_ptr<AnyBox> mebxBox = std::make_shared<AnyBox>(0, "mebx");
    mebxBox->SetFunc([] (std::shared_ptr<AVIOStream> io) {
            io->Write(static_cast<uint32_t>(0));  /* Reserved */
            io->Write(static_cast<uint16_t>(0));  /* Reserved */
            io->Write(static_cast<uint16_t>(0));  /* Data-reference index */
        }, [] () {
            return static_cast<uint32_t>(sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t));
        });
    
    std::shared_ptr<BasicBox> keysBox = std::make_shared<BasicBox>(0, "keys");
    auto &trackDesc = timedMetaTrack->GetTrackDesc();
    std::string value;
    if (trackDesc->GetData(Tag::TIMED_METADATA_KEY, value) &&
        value.compare(0, 30, "com.openharmony.timed_metadata") == 0) {  // string length 30
        // keysbox里最多只有1个元素
        uint32_t keyBoxSize = static_cast<uint32_t>(value.length() + 12);  // 12
        std::shared_ptr<AnyBox> timedMetaKeyBox = std::make_shared<AnyBox>(0, Uint32ToString(1));
        timedMetaKeyBox->SetFunc([value, keyBoxSize] (std::shared_ptr<AVIOStream> io) {
                io->Write(keyBoxSize);  /* size */
                io->Write("keyd", BOX_TYPE_LEN);
                io->Write("mdta", BOX_TYPE_LEN);
                io->Write(value.c_str(), static_cast<uint32_t>(value.length()));
            }, [keyBoxSize] () {
                return keyBoxSize;
            });
        keysBox->AddChild(timedMetaKeyBox);
    }
    MEDIA_LOG_I("timed_metadata value:[%{public}s]", value.c_str());
    mebxBox->AddChild(keysBox);
    return mebxBox;
}

std::shared_ptr<BasicBox> BoxParser::EsdsBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<EsdsBox> esdsBox = std::make_shared<EsdsBox>(0, "esds");
    esdsBox->esId_ = static_cast<uint16_t>(track->GetTrackId());
    std::string mimeType = track->GetMimeType();
    if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG) == 0) {
        esdsBox->objectType_ = 0x69;
        esdsBox->streamType_ = 0x15;
    } else if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC) == 0) {
        esdsBox->objectType_ = 0x40;
        esdsBox->streamType_ = 0x15;
    } else if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_MPEG4) == 0) {
        esdsBox->objectType_ = 0x20;
        esdsBox->streamType_ = 0x11;
    } else {
        MEDIA_LOG_D("mimeType %{public}s is unsupported", mimeType.c_str());
    }
    esdsBox->codecConfig_ = track->GetCodecConfig();
    uint32_t codeConfigAndSpInfoLen = esdsBox->codecConfig_.size() > 0 ? esdsBox->codecConfig_.size() + 5 : 0;  // 5
    esdsBox->esDescr_ = CalculateEsDescr(27 + codeConfigAndSpInfoLen);  // 27 = 3 + 5 + 13  + 5 + 1
    esdsBox->dcDescr_ = CalculateEsDescr(13 + codeConfigAndSpInfoLen);  // 13
    esdsBox->dsInfo_ = CalculateEsDescr(esdsBox->codecConfig_.size());
    esdsBox->slcDescr_ = CalculateEsDescr(1);

    return esdsBox;
}

std::shared_ptr<BasicBox> BoxParser::AvccBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<AvccBox> avccBox = std::make_shared<AvccBox>(0, "avcC");
    auto &codecConfig = track->GetCodecConfig();
    size_t size = codecConfig.size();
    if (size > 6 && codecConfig[0] == 0x01) { // codec config, min: 6
        uint32_t pos = 0;
        avccBox->version_ = codecConfig[pos++];
        avccBox->profileIdc_ = codecConfig[pos++];
        avccBox->profileCompat_ = codecConfig[pos++];
        avccBox->levelIdc_ = codecConfig[pos++];
        avccBox->naluSize_ = codecConfig[pos++];
        avccBox->spsCount_ = codecConfig[pos++]; // pos 5
        for (uint8_t i = 0; i < (avccBox->spsCount_ & 0x1F) && pos + 1 < size; ++i) {
            uint32_t len = codecConfig[pos] * 0x100 + codecConfig[pos + 1] + 2;  // 2
            std::vector<uint8_t> sps(codecConfig.data() + pos, codecConfig.data() + pos + len);
            avccBox->sps_.emplace_back(sps);
            pos += len;
        }
        if (pos >= size) {
            return avccBox;
        }
        avccBox->ppsCount_ = codecConfig[pos++];
        for (uint8_t i = 0; i < avccBox->ppsCount_ && pos + 1 < size; ++i) {
            uint32_t len = codecConfig[pos] * 0x100 + codecConfig[pos + 1] + 2;  // 2
            std::vector<uint8_t> pps(codecConfig.data() + pos, codecConfig.data() + pos + len);
            avccBox->pps_.emplace_back(pps);
            pos += len;
        }
        if (pos + 3 >= size) {  // 3
            return avccBox;
        }
        avccBox->chromaFormat_ = codecConfig[pos++];
        avccBox->bitDepthLuma_ = codecConfig[pos++];
        avccBox->bitDepthChroma_ = codecConfig[pos++];
        avccBox->spsExtCount_ = codecConfig[pos++];
        for (uint8_t i = 0; i < avccBox->spsExtCount_ && pos + 1 < size; ++i) {
            uint32_t len = codecConfig[pos] * 0x100 + codecConfig[pos + 1] + 2;  // 2
            std::vector<uint8_t> spsExt(codecConfig.data() + pos, codecConfig.data() + pos + len);
            avccBox->spsExt_.emplace_back(spsExt);
            pos += len;
        }
    }
    return avccBox;
}

std::shared_ptr<BasicBox> BoxParser::HvccBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<HvccBox> hvccBox = std::make_shared<HvccBox>(0, "hvcC");
    auto &codecConfig = track->GetCodecConfig();
    size_t size = codecConfig.size();
    if (size > 23 && codecConfig[0] == 0x01) { // codec config, min: 23
        uint32_t pos = 0;
        hvccBox->version_ = codecConfig[pos++];
        hvccBox->profileSpace_ = (codecConfig[pos] & 0xC0) >> 6;  // 6
        hvccBox->tierFlag_ = (codecConfig[pos] & 0x20) >> 5;  // 5
        hvccBox->profileIdc_ = codecConfig[pos++] & 0x1F;
        hvccBox->profileCompatFlags_ = 0;
        for (int32_t i = 0; i < 4; ++i) {  // 4
            hvccBox->profileCompatFlags_ = codecConfig[pos++] + hvccBox->profileCompatFlags_ * 0x100;
        }
        hvccBox->constraintIndicatorFlags_ = 0;
        for (int32_t i = 0; i < 6; ++i) {  // 6
            hvccBox->constraintIndicatorFlags_ = codecConfig[pos++] + hvccBox->constraintIndicatorFlags_ * 0x100;
        }
        hvccBox->levelIdc_ = codecConfig[pos++];
        uint16_t tmpNum = codecConfig[pos++] * 0x100;
        hvccBox->segmentIdc_ = tmpNum + codecConfig[pos++];
        hvccBox->parallelismType_ = codecConfig[pos++];
        hvccBox->chromaFormat_ = codecConfig[pos++];
        hvccBox->bitDepthLuma_ = codecConfig[pos++];
        hvccBox->bitDepthChroma_ = codecConfig[pos++];
        tmpNum = codecConfig[pos++] * 0x100;
        hvccBox->avgFrameRate_ = tmpNum + codecConfig[pos++];
        hvccBox->constFrameRate_ = (codecConfig[pos] & 0xC0) >> 6;  // 6
        hvccBox->numTemporalLayers_ = (codecConfig[pos] & 0x38) >> 3;  // 3
        hvccBox->temporalIdNested_ = (codecConfig[pos] & 0x04) >> 2;  // 2
        hvccBox->lenSizeMinusOne_ = codecConfig[pos++] & 0x03;
        hvccBox->naluTypeCount_ = codecConfig[pos++]; // pos 23
        for (uint8_t i = 0; i < hvccBox->naluTypeCount_ && pos + 2 < size; ++i) {  // 2
            HvccBox::Data data;
            data.type_ = codecConfig[pos++];
            tmpNum = codecConfig[pos++] * 0x100;
            data.count_ = tmpNum + codecConfig[pos++];
            for (uint32_t j = 0; j < data.count_ && pos + 1 < size; ++j) {
                uint32_t len = codecConfig[pos] * 0x100 + codecConfig[pos + 1] + 2;  // 2
                std::vector<uint8_t> nalUnit(codecConfig.data() + pos, codecConfig.data() + pos + len);
                data.nalu_.emplace_back(nalUnit);
                pos += len;
            }
            hvccBox->naluType_.emplace_back(data);
        }
    }
    return hvccBox;
}

std::shared_ptr<BasicBox> BoxParser::ColrBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    auto videoTrack = BasicTrack::GetTrackPtr<VideoTrack>(track);
    if (videoTrack == nullptr || !videoTrack->IsColor()) {
        return nullptr;
    }
    std::shared_ptr<ColrBox> colrBox = std::make_shared<ColrBox>(0, "colr");
    colrBox->colorPrimaries_ = static_cast<uint16_t>(videoTrack->GetColorPrimaries());
    colrBox->colorTransfer_ = static_cast<uint16_t>(videoTrack->GetColorTransfer());
    colrBox->colorMatrixCoeff_ = static_cast<uint16_t>(videoTrack->GetColorMatrixCoeff());
    colrBox->colorRange_ = videoTrack->GetColorRange() ? 0x80 : 0x00;
    return colrBox;
}

std::shared_ptr<BasicBox> BoxParser::CuvvBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    auto videoTrack = BasicTrack::GetTrackPtr<VideoTrack>(track);
    if (videoTrack == nullptr || !videoTrack->IsCuvaHDR() ||
        videoTrack->GetMimeType().compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) != 0) {
        return nullptr;
    }
    auto cuvvBox = std::make_shared<CuvvBox>(0, "cuvv");
    cuvvBox->cuvaVersion_ = 0x0001;
    cuvvBox->terminalProvideCode_ = 0x0004;
    cuvvBox->terminalProvideOriCode_ = 0x0005;
    return cuvvBox;
}

uint32_t BoxParser::CalculateEsDescr(uint32_t size)
{
    uint32_t data = ((size >> 21) | 0x80) << 24;  // 21 24
    data += ((size >> 14) | 0x80) << 16;  // 14 16
    data += ((size >> 7) | 0x80) << 8;  // 7 8
    data += size & 0x7F;
    return data;
}

std::shared_ptr<BasicBox> BoxParser::FileLevelMetaBoxGenerate(const std::shared_ptr<Meta> &param)
{
    std::shared_ptr<FullBox> metaBox = std::make_shared<FullBox>(0, "meta");
    // add hdlr
    std::shared_ptr<HdlrBox> hdlrBox = std::make_shared<HdlrBox>(0, "hdlr");
    hdlrBox->handlerType_ = 'gltf';
    hdlrBox->name_ = "gltf";
    metaBox->AddChild(hdlrBox);
    // add iloc
    std::vector<uint8_t> dataBinary = {};
    param->GetData(Tag::MEDIA_GLTF_DATA, dataBinary);
    metaBox->AddChild(GltfIlocBoxGenerate(static_cast<uint32_t>(dataBinary.size())));

    // add pitm
    std::shared_ptr<AnyBox> pitmBox = std::make_shared<AnyBox>(0, "pitm");
    pitmBox->SetFunc([] (std::shared_ptr<AVIOStream> io) {
            io->Write(static_cast<uint32_t>(0));  // full box version and flag
            io->Write(static_cast<uint16_t>(1));  // item id: 1
        }, [] () {
            return static_cast<uint32_t>(sizeof(uint32_t) + sizeof(uint16_t));
        });
    metaBox->AddChild(pitmBox);
    // add iinf
    metaBox->AddChild(GltfIinfBoxGenerate(param));
    // add idat
    std::shared_ptr<AnyBox> idatBox = std::make_shared<AnyBox>(0, "idat");
    idatBox->SetFunc([dataBinary] (std::shared_ptr<AVIOStream> io) {
            io->Write(dataBinary.data(), static_cast<int32_t>(dataBinary.size()));
        }, [dataLen = static_cast<uint32_t>(dataBinary.size())] () {
            return dataLen;
        });
    metaBox->AddChild(idatBox);
    MEDIA_LOG_I("infe box, binary data size:%{public}zu", dataBinary.size());
    return metaBox;
}

std::shared_ptr<BasicBox> BoxParser::GltfIlocBoxGenerate(uint32_t binaryDataLen)
{
    std::shared_ptr<AnyBox> ilocBox = std::make_shared<AnyBox>(0, "iloc");
    ilocBox->SetFunc([binaryDataLen, &flag = g_flag] (std::shared_ptr<AVIOStream> io) {
            io->Write(static_cast<uint8_t>(1));  // version
            io->Write(flag, BOX_FLAG_LEN);
            io->Write(static_cast<uint8_t>((4 << 4) | 4));  // offset: 4, length: 4
            io->Write(static_cast<uint8_t>((4 << 4) | 0));  // base offset: 4, index: 4

            io->Write(static_cast<uint16_t>(1));            // item count: 1
            // item[0]
            io->Write(static_cast<uint16_t>(1));            // item id 1
            io->Write(static_cast<uint16_t>(1));            // reserved: 0(12bit) + construction_method: 1(4bit)
            io->Write(static_cast<uint16_t>(0));            // data reference index: 0
            io->Write(static_cast<uint32_t>(0));            // base offset:0

            io->Write(static_cast<uint16_t>(1));            // extent count: 1
            // extent[0]
            io->Write(static_cast<uint32_t>(0));
            io->Write(static_cast<uint32_t>(binaryDataLen));
        }, [] () {
            return static_cast<uint32_t>(28);  // 28
        });
    return ilocBox;
}

std::shared_ptr<BasicBox> BoxParser::GltfIinfBoxGenerate(const std::shared_ptr<Meta> &param)
{
    std::shared_ptr<AnyBox> iinfBox = std::make_shared<AnyBox>(0, "iinf");
    iinfBox->SetFunc([] (std::shared_ptr<AVIOStream> io) {
            io->Write(static_cast<uint32_t>(0));  // full box version and flag
            io->Write(static_cast<uint16_t>(1));  // entry count: 1
        }, [] () {
            return static_cast<uint32_t>(sizeof(uint32_t) + sizeof(uint16_t));
        });

    int32_t version = 0;
    param->GetData(Tag::MEDIA_GLTF_VERSION, version);
    std::shared_ptr<InfeBox> infeBox = std::make_shared<InfeBox>(0, "infe", version, 0);
    param->GetData(Tag::MEDIA_GLTF_ITEM_TYPE, infeBox->itemType_);
    param->GetData(Tag::MEDIA_GLTF_ITEM_NAME, infeBox->itemName_);
    param->GetData(Tag::MEDIA_GLTF_CONTENT_TYPE, infeBox->contentType_);
    param->GetData(Tag::MEDIA_GLTF_CONTENT_ENCODING, infeBox->contentEncoding_);
    MEDIA_LOG_I("infe box: item type:%{public}s, name:%{public}s, content type:%{public}s",
        infeBox->itemType_.data(), infeBox->itemName_.data(), infeBox->contentType_.data());
    iinfBox->AddChild(infeBox);
    return iinfBox;
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS