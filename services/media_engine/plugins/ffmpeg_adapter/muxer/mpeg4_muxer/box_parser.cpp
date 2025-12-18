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
#ifndef _WIN32
#include "securec.h"
#endif // !_WIN32
#include "audio_track.h"
#include "video_track.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "BoxParser" };
}
#endif // !_WIN32

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
    FALSE_RETURN_MSG(mvhdBox->nextTrackId < INT16_MAX, "track id is out of range"); // max int16
    track->SetTrackId(static_cast<int32_t>(mvhdBox->nextTrackId));
    moov_->AddChild(TrakBoxGenerate(track));
    mvhdBox->nextTrackId++;
}

void BoxParser::AddUdtaBox()
{
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    moov_->AddChild(UdtaBoxGenerate());
}

std::shared_ptr<BasicBox> BoxParser::MvhdBoxGenerate()
{
    std::shared_ptr<MvhdBox> mvhdBox = std::make_shared<MvhdBox>(108, "mvhd");  // boxSize: 108
    mvhdBox->creationTime_ = 0;
    mvhdBox->modificationTime_ = 0;
    mvhdBox->duration_ = 0;
    mvhdBox->nextTrackId = 1;
    return mvhdBox;
}

std::shared_ptr<BasicBox> BoxParser::UdtaBoxGenerate()
{
    std::shared_ptr<BasicBox> utdaBox = std::make_shared<BasicBox>(0, "udta");
    utdaBox->AddChild(MetaBoxGenerate());
    return utdaBox;
}

std::shared_ptr<BasicBox> BoxParser::MetaBoxGenerate()
{
    std::shared_ptr<FullBox> metaBox = std::make_shared<FullBox>(0, "meta");
    std::shared_ptr<HdlrBox> hdlrBox = std::make_shared<HdlrBox>(0, "hdlr");
    hdlrBox->handlerType_ = 'mdir';
    hdlrBox->reserved_[0] = 'ohav';
    metaBox->AddChild(hdlrBox);
    metaBox->AddChild(IlstBoxGenerate());
    return metaBox;
}

std::shared_ptr<BasicBox> BoxParser::IlstBoxGenerate()
{
    std::shared_ptr<BasicBox> ilstBox = std::make_shared<BasicBox>(0, "ilst");
    std::shared_ptr<BasicBox> tooBox = std::make_shared<BasicBox>(0, "\xA9too");
    std::shared_ptr<DataBox> dataBox = std::make_shared<DataBox>(0, "data");
    dataBox->dataType_ = 1;
    dataBox->default_ = 0;
    std::string tool = "Openharmony4.1";
    dataBox->data_.reserve(tool.length());
    dataBox->data_.assign(tool.data(), tool.data() + tool.length());
    tooBox->AddChild(dataBox);
    ilstBox->AddChild(tooBox);
    return ilstBox;
}

std::shared_ptr<BasicBox> BoxParser::TrakBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::string type = "trak";
    type += std::to_string(track->GetTrackId());
    std::shared_ptr<BasicBox> trakBox = std::make_shared<BasicBox>(0, type);
    trakBox->AddChild(TkhdBoxGenerate(track));
    trakBox->AddChild(EdtsBoxGenerate());
    trakBox->AddChild(MdiaBoxGenerate(track));
    track->SetTrackPath(type);
    return trakBox;
}

std::shared_ptr<BasicBox> BoxParser::TkhdBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<TkhdBox> tkhdBox = std::make_shared<TkhdBox>(0, "tkhd");
    tkhdBox->creationTime_ = 0;
    tkhdBox->modificationTime_ = 0;
    tkhdBox->trackIndex_ = track->GetTrackId();
    tkhdBox->duration_ = 0;
    int32_t mediaType = track->GetMediaType();
    if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_AUD) {
        tkhdBox->volume_ = 0x0100;
    } else if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_VID) { // 视频的旋转角度，后边刷新
        auto videoTrack = BasicTrack::GetTrackPtr<VideoTrack>(track);
        if (videoTrack != nullptr) {
            tkhdBox->width_ = videoTrack->GetWidth() << 16;  // 16
            tkhdBox->height_ = videoTrack->GetHeight() << 16;  // 16
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

std::shared_ptr<BasicBox> BoxParser::MdhdBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<MdhdBox> mdhdBox = std::make_shared<MdhdBox>(0, "mdhd");
    mdhdBox->creationTime_ = 0;
    mdhdBox->modificationTime_ = 0;
    mdhdBox->timeScale_ = track->GetTimeScale();
    mdhdBox->duration_ = 0;
    return mdhdBox;
}

std::shared_ptr<BasicBox> BoxParser::HdlrBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<HdlrBox> hdlrBox = std::make_shared<HdlrBox>(0, "hdlr");
    hdlrBox->predefined_ = 0;
    int32_t mediaType = track->GetMediaType();
    if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_AUD) {
        hdlrBox->handlerType_ = 'soun';
        hdlrBox->name_ = "SoundHandler";
    } else if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_VID) {
        hdlrBox->handlerType_ = 'vide';
        hdlrBox->name_ = "VideoHandler";
    }
    return hdlrBox;
}

std::shared_ptr<BasicBox> BoxParser::MinfBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<BasicBox> minfBox = std::make_shared<BasicBox>(0, "minf");
    int32_t mediaType = track->GetMediaType();
    if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_AUD) {
        minfBox->AddChild(std::make_shared<SmhdBox>(0, "smhd"));
    } else if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_VID) {
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
    if (track->GetMediaType() == OHOS::MediaAVCodec::MEDIA_TYPE_VID) {
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
    int32_t mediaType = track->GetMediaType();
    if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_AUD) {
        stsdBox->AddChild(AudioBoxGenerate(track));
    } else if (mediaType == OHOS::MediaAVCodec::MEDIA_TYPE_VID) {
        stsdBox->AddChild(VideoBoxGenerate(track));
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
            (void)memcpy_s(videoBox->compressor_, sizeof(videoBox->compressor_), name.data(), name.length());
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

std::shared_ptr<BasicBox> BoxParser::EsdsBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<EsdsBox> esdsBox = std::make_shared<EsdsBox>(0, "esds");
    esdsBox->esId_ = static_cast<uint16_t>(track->GetTrackId());
    std::string mimeType = track->GetMimeType();
    if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG) == 0) {
        esdsBox->objectType_ = 0x68;
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
    if (esdsBox->codecConfig_.size() > 0) { // codec config
        esdsBox->esDescr_ = CalculateEsDescr(32 + esdsBox->codecConfig_.size());  // 32 = 3 + 5 + 13 + 5 + 5 + 1
        esdsBox->dcDescr_ = CalculateEsDescr(18 + esdsBox->codecConfig_.size());  // 18 = 13 + 5
        esdsBox->dsInfo_ = CalculateEsDescr(esdsBox->codecConfig_.size());
        esdsBox->slcDescr_ = CalculateEsDescr(1);
    }

    return esdsBox;
}

std::shared_ptr<BasicBox> BoxParser::AvccBoxGenerate(std::shared_ptr<BasicTrack> track)
{
    std::shared_ptr<AvccBox> avccBox = std::make_shared<AvccBox>(0, "avcC");
    auto codecConfig = track->GetCodecConfig();
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
    auto codecConfig = track->GetCodecConfig();
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
} // Mpeg4
} // Plugins
} // Media
} // OHOS