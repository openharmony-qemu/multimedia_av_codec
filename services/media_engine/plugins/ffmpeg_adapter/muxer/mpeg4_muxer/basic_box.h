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

#ifndef BASIC_BOX_H
#define BASIC_BOX_H

#include <string>
#include <deque>
#include <unordered_map>
#include <vector>
#include <memory>
#include "meta/any.h"
#include "common/log.h"
#include "avio_stream.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class BasicBox {
public:
    BasicBox(uint32_t size, std::string type);
    virtual ~BasicBox() = default;
    virtual uint32_t GetSize();
    void SetType(std::string type);
    std::string GetType();
    bool AddChild(std::shared_ptr<BasicBox> box);
    std::shared_ptr<BasicBox> GetChild(std::string typePath);
    size_t GetChildCount();
    virtual int64_t Write(std::shared_ptr<AVIOStream> io);
    virtual void SetVersion(uint8_t version) {return;}
    virtual Any GetCurBoxPtr() {return Any();}
public:
    template <class T>
    static T* GetBoxPtr(std::shared_ptr<BasicBox> &box)
    {
        static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "BasicBox" };
        FALSE_RETURN_V_MSG_W(box != nullptr, nullptr,
            "GetBoxPtr parent box is nullptr! child:%{public}s", box->GetType().c_str());
        auto anyPtr = box->GetCurBoxPtr();
        FALSE_RETURN_V_MSG_W(Any::IsSameTypeWith<T*>(anyPtr), nullptr,
            "GetBoxPtr type err! box:%{public}s, type:%{public}s", box->GetType().c_str(), anyPtr.TypeName().data());
        return AnyCast<T*>(anyPtr);
    }

    template <class T>
    static T* GetBoxPtr(std::shared_ptr<BasicBox> &parentBox, std::string typePath)
    {
        static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "BasicBox" };
        FALSE_RETURN_V_MSG_W(parentBox != nullptr, nullptr,
            "GetBoxPtr parent box is nullptr! child:%{public}s", typePath.c_str());
        auto box = parentBox->GetChild(typePath);
        FALSE_RETURN_V_MSG_W(box != nullptr, nullptr,
            "GetBoxPtr get child failed! parent box:%{public}s, child:%{public}s",
            parentBox->GetType().c_str(), typePath.c_str());
        auto anyPtr = box->GetCurBoxPtr();
        FALSE_RETURN_V_MSG_W(Any::IsSameTypeWith<T*>(anyPtr), nullptr,
            "GetBoxPtr type err! parent box:%{public}s, child:%{public}s, type:%{public}s",
            parentBox->GetType().c_str(), typePath.c_str(), anyPtr.TypeName().data());
        return AnyCast<T*>(anyPtr);
    }

protected:
    inline void WriteChild(std::shared_ptr<AVIOStream> io);

protected:
    uint32_t size_;
    std::string type_;
    std::unordered_map<std::string, std::shared_ptr<BasicBox>> childBoxes_;
    std::vector<std::string> childTypes_;
};

class FullBox : public BasicBox {
public:
    FullBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    void SetVersion(uint8_t version) override;

protected:
    uint8_t version_;
    uint8_t flags_[3];
};

class MvhdBox : public FullBox {
public:
    MvhdBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint64_t creationTime_ = 0;
    uint64_t modificationTime_ = 0;
    uint32_t timeScale_ = 1000;
    uint64_t duration_ = 0;
    uint32_t rate_ = 0x00010000;
    uint16_t volume_ = 0x0100;
    uint8_t reserved1_[10] = {0}; // reserved
    uint32_t matrix_[3][3] = {{0x00010000, 0, 0}, {0, 0x00010000, 0}, {0, 0, 0x40000000}};
    uint8_t reserved2_[24] = {0}; // reserved
    uint32_t nextTrackId = 1;
};

class TkhdBox : public FullBox {
public:
    TkhdBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 7);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint64_t creationTime_ = 0;
    uint64_t modificationTime_ = 0;
    uint32_t trackIndex_ = 0;
    uint32_t reserved1_ = 0; // reserved
    uint64_t duration_ = 0;
    uint8_t reserved2_[8] = {0}; // reserved
    uint16_t layer_ = 0;
    uint16_t alternateGroup_ = 0;
    uint16_t volume_ = 0;
    uint16_t reserved3_ = 0; // reserved
    uint32_t matrix_[3][3] = {{0x00010000, 0, 0}, {0, 0x00010000, 0}, {0, 0, 0x40000000}};
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

class ElstBox : public FullBox {
public:
    ElstBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    class Data {
    public:
        uint64_t segmentDuration_ = 0;
        uint64_t mediaTime_ = 0;
        uint16_t mediaRateInteger_ = 1;
        uint16_t mediaRateFraction_ = 0;
    };

    uint32_t entryCount_ = 0;
    std::deque<Data> data_;
};

class MdhdBox : public FullBox {
public:
    MdhdBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint64_t creationTime_ = 0;
    uint64_t modificationTime_ = 0;
    uint32_t timeScale_ = 1;
    uint64_t duration_ = 0;
    uint16_t language_ = 0x55C4;
    uint16_t quality_ = 0; // reserved
};

class HdlrBox : public FullBox {
public:
    HdlrBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t predefined_ = 0;
    uint32_t handlerType_ = 0;
    uint32_t reserved_[3] = {0}; // reserved
    std::string name_ = ""; // \x00 需要写入
};

class VmhdBox : public FullBox {
public:
    VmhdBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 1);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint16_t graphicsMode_ = 0;
    uint16_t opColor_[3] = {0, 0, 0};
};

class SmhdBox : public FullBox {
public:
    SmhdBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint16_t balance_ = 0;
    uint16_t reserved_ = 0;
};

class DrefBox : public FullBox {
public:
    DrefBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t entryCount_ = 1;
};

class StsdBox : public FullBox {
public:
    StsdBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t entryCount_ = 1;
};

class AudioBox : public BasicBox {
public:
    AudioBox(uint32_t size, std::string type); // mp4a
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t reserved1_ = 0; // reserved
    uint16_t reserved2_ = 0; // reserved
    uint16_t dataRefIndex_ = 1;
    uint16_t version_ = 0;
    uint16_t revisionLevel_ = 0;
    uint32_t reserved3_ = 0; // reserved
    uint16_t channels_ = 0x02;
    uint16_t sampleSize_ = 0x10;
    uint16_t predefined_ = 0;
    uint16_t packetSize_ = 0; // reserved
    uint16_t sampleRate_ = 0;
    uint16_t reserved4_ = 0; // reserved
};

class EsdsBox : public FullBox {
public:
    EsdsBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint8_t esDescrTag_ = 0x03; // ES descriptor tag
    uint32_t esDescr_ = 0; //ES descriptor
    uint16_t esId_ = 0; // trackId
    uint8_t esFlags_ = 0;
    uint8_t dcDescrTag_ = 0x04; // decoder config descriptor tag
    uint32_t dcDescr_ = 0; // decoder config descriptor
    uint8_t objectType_ = 0; // AAC 0x40, MP3 0x6B, MPEG4 0x20
    uint8_t streamType_ = 0x15; // Audio stream 0x15, Visual/Video stream 0x11
    uint8_t bufferSize_[3] = {0x00, 0x03, 0x00}; // 0x000300
    uint32_t maxBitrate_ = 0;
    uint32_t avgBitrate_ = 0;
    uint8_t dsInfoTag_ = 0x05; // decoder specific info tag
    uint32_t dsInfo_ = 0; // decoder specific info
    std::vector<uint8_t> codecConfig_; // codec config
    uint8_t slcDescrTag_ = 0x06; // SL descriptor tag
    uint32_t slcDescr_ = 0x80808001; // SL descriptor
    uint8_t slcData_ = 0x02;
};

class BtrtBox : public BasicBox {
public:
    BtrtBox(uint32_t size, std::string type);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint32_t bufferSize_ = 0;
    uint32_t maxBitrate_ = 0;
    uint32_t avgBitrate_ = 0;
};

class VideoBox : public BasicBox {
public:
    VideoBox(uint32_t size, std::string type); // mp4v, avc1, hvc1
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint32_t reserved1_ = 0; // reserved
    uint16_t reserved2_ = 0; // reserved
    uint16_t dataRefIndex_ = 1;
    uint16_t version_ = 0;
    uint16_t revision_ = 0;
    uint32_t reserved3_[3] = {0}; // reserved
    uint16_t width_ = 0;
    uint16_t height_ = 0;
    uint32_t horiz_ = 0x00480000;
    uint32_t vert_ = 0x00480000;
    uint32_t dataSize_ = 0;
    uint16_t frameCount_ = 1;
    uint8_t compressorLen_ = 0;
    uint8_t compressor_[31] = {0};
    uint16_t depth_ = 0x18;
    uint16_t predefined_ = 0xFFFF;
};

class AvccBox : public BasicBox {
public:
    AvccBox(uint32_t size, std::string type);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint8_t version_ = 0x01;
    uint8_t profileIdc_ = 0x64; // sps[3]
    uint8_t profileCompat_ = 0; // sps[4]
    uint8_t levelIdc_ = 0x1F; // sps[5]
    uint8_t naluSize_ = 0xFF; // 0xFC | 3 (2 bits nal size length - 1)
    uint8_t spsCount_ = 0xE0; // 0xE0 | 1 (5-bit sps count)
    std::vector<std::vector<uint8_t>> sps_; // 2 bytes size + sps content (0x67&0x1F)
    uint8_t ppsCount_ = 0;
    std::vector<std::vector<uint8_t>> pps_; // 2 bytes size + pps content (0x68&0x1F)
    // only sps[3] != 0x42 && sps[3] != 0x4D && sps[3] != 0x58
    uint8_t chromaFormat_ = 0xFC; // 0xFC | 2 bits idc
    uint8_t bitDepthLuma_ = 0xF8; // 0xF8 | 3 bits luma
    uint8_t bitDepthChroma_ = 0xF8; // 0xF8 | 3 bits chroma
    uint8_t spsExtCount_ = 0;
    std::vector<std::vector<uint8_t>> spsExt_; // 2 bytes size + sps ext content (0x6D&0x1F)
};

class HvccBox : public BasicBox {
public:
    HvccBox(uint32_t size, std::string type);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    class Data {
    public:
        uint8_t type_ = 0;
        uint16_t count_ = 0;
        std::vector<std::vector<uint8_t>> nalu_; // 2 bytes size + content
    };

    uint8_t version_ = 0x01;
    uint8_t profileSpace_ = 0;
    uint8_t tierFlag_ = 0;
    uint8_t profileIdc_ = 0;
    uint32_t profileCompatFlags_ = 0xFFFFFFFF;
    uint64_t constraintIndicatorFlags_ = 0xFFFFFFFFFFFF;
    uint8_t levelIdc_ = 0;
    uint16_t segmentIdc_ = 0;
    uint8_t parallelismType_ = 0;
    uint8_t chromaFormat_ = 0;
    uint8_t bitDepthLuma_ = 0;
    uint8_t bitDepthChroma_ = 0;
    uint16_t avgFrameRate_ = 0;
    uint8_t constFrameRate_ = 0;
    uint8_t numTemporalLayers_ = 0;
    uint8_t temporalIdNested_ = 0;
    uint8_t lenSizeMinusOne_ = 0x03;
    uint8_t naluTypeCount_ = 0;
    std::vector<Data> naluType_;
    std::vector<uint8_t> data;
};

class PaspBox : public BasicBox {
public:
    PaspBox(uint32_t size, std::string type);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t hSpacing_ = 0x10000;
    uint32_t vSpacing_ = 0x10000;
};

class ColrBox : public BasicBox {
public:
    ColrBox(uint32_t size, std::string type);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t colorType_ = 'nclx';
    uint16_t colorPrimaries_ = 0;
    uint16_t colorTransfer_ = 0;
    uint16_t colorMatrixCoeff_ = 0;
    uint8_t colorRange_ = 0;
};

class CuvvBox : public BasicBox {
public:
    CuvvBox(uint32_t size, std::string type);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint16_t cuvaVersion_ = 0x0001;
    uint16_t terminalProvideCode_ = 0x0004;
    uint16_t terminalProvideOriCode_ = 0x0005;
    uint32_t reserved_[4] = {0};  // reserved
};

class SttsBox : public FullBox {
public:
    SttsBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0); // stts, ctts
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t entryCount_ = 0;
    std::deque<std::pair<uint32_t, uint32_t>> timeToSamples_;
};

class StscBox : public FullBox {
public:
    StscBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    class Data {
    public:
        uint32_t chunkNum_ = 1;
        uint32_t samplesPerChunk_ = 1;
        uint32_t descrIndex_ = 0x01;
    };

    uint32_t entryCount_ = 0;
    std::deque<Data> chunks_;
};

class StszBox : public FullBox {
public:
    StszBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t sampleSize_ = 0;
    uint32_t sampleCount_ = 0;
    std::deque<uint32_t> samples_;
};

class StcoBox : public FullBox {
public:
    StcoBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0); // stco, co64
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t chunkCount_ = 0;
    std::deque<uint32_t> chunksOffset32_;
    std::deque<uint64_t> chunksOffset64_;
    bool isCo64_ = false;
    uint64_t offset_ = 0;
};

class StssBox : public FullBox {
public:
    StssBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0); // stss, stps
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t syncCount_ = 0;
    std::deque<uint32_t> syncIndex_;
};

class SgpdBox : public FullBox {
public:
    SgpdBox(uint32_t size, std::string type, uint8_t version = 1, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t groupType_ = 'roll';
    uint32_t defaultLen_ = 2;
    uint32_t entryCount_ = 1;
    std::deque<uint16_t> distances_ = {0xFFFF};
};

class SbgpBox : public FullBox {
public:
    SbgpBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;
    Any GetCurBoxPtr() override {return Any(this);}

    uint32_t groupType_ = 'roll';
    uint32_t entryCount_ = 1;
    std::deque<std::pair<uint32_t, uint32_t>> descriptions_ = {{0, 1}};
};

class KeysBox : public FullBox {
public:
    KeysBox(uint32_t size, std::string type, uint8_t version = 0, uint32_t flags = 0);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t keyCount_ = 0;
};

class DataBox : public BasicBox {
public:
    DataBox(uint32_t size, std::string type);
    uint32_t GetSize() override;
    int64_t Write(std::shared_ptr<AVIOStream> io) override;

    uint32_t dataType_ = 0; // 0x01 UTF8, 0x17 float32, 0x43 int32, 0x0D jpg, 0x0E png, 0x1B bmp
    uint32_t default_ = 0;
    std::vector<uint8_t> data_;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif