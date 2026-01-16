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

#ifndef MPEG4_MUXER_BASIC_TRACK_H
#define MPEG4_MUXER_BASIC_TRACK_H

#include <string>
#include <vector>
#include <cstdint>
#include "media_description.h"
#include "avcodec_common.h"
#include "basic_box.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class BasicTrack {
public:
    BasicTrack(std::string mimeType, std::shared_ptr<BasicBox> moov);
    virtual ~BasicTrack() = default;
    virtual Status Init(const std::shared_ptr<Meta> &trackDesc);
    virtual void SetRotation(uint32_t rotation);
    virtual Status WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample);
    virtual Status WriteTailer();
    virtual Any GetPointer() {return Any();}
    void SetTrackId(int32_t trackId);
    void SetTrackPath(std::string trackPath);
    void SetCodingType(std::string codingType);
    int32_t GetTrackId() const;
    std::string GetMimeType() const;
    MediaType GetMediaType() const;
    std::vector<uint8_t> &GetCodecConfig();
    int32_t GetTimeScale() const;
    void SetSttsBox(std::shared_ptr<SttsBox> stts);
    void SetStszBox(std::shared_ptr<StszBox> stsz);
    void SetStscBox(std::shared_ptr<StscBox> stsc);
    void SetStcoBox(std::shared_ptr<StcoBox> stco);
    void SetCreationTime(uint64_t creationTime, uint64_t modifyTime);
    static bool Compare(std::shared_ptr<BasicTrack> trackOne, std::shared_ptr<BasicTrack> trackTwo);
    uint32_t MoovIncrements(uint32_t moovSize);
    void SetOffset(uint32_t moovSize);
    int64_t GetDurationUs() const;
    int64_t GetStartTimeUs() const;
    bool IsAuxiliary() const;
    uint32_t GetHdlrType() const;
    const std::string &GetHdlrName() const;
    uint32_t GetTrefTag() const;
    std::shared_ptr<std::vector<uint32_t>> &GetSrcTrackIds();

    template <class T>
    static T* GetTrackPtr(const std::shared_ptr<BasicTrack> &basicTrack)
    {
        static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "BasicTrack" };
        FALSE_RETURN_V_MSG_W(basicTrack != nullptr, nullptr, "GetTrackPtr track is nullptr!");
        auto anyPtr = basicTrack->GetPointer();
        FALSE_RETURN_V_MSG_W(Any::IsSameTypeWith<T*>(anyPtr), nullptr,
            "GetTrackPtr type err!trackId:%{public}d, type:%{public}s",
            basicTrack->GetTrackId(), anyPtr.TypeName().data());
        return AnyCast<T*>(anyPtr);
    }

protected:
    virtual void DisposeStts(int64_t duration, int64_t pts);
    virtual void DisposeStco(int64_t pos);
    virtual void DisposeStsz(int32_t size);
    Status SetAuxiliaryTrackParam(const std::shared_ptr<Meta> &trackDesc);

    std::string trackPath_ = "";
    int32_t trackId_ = -1;
    MediaType mediaType_ = MediaType::UNKNOWN;
    std::string mimeType_ = "";
    std::string codingType_ = "";
    int64_t bitRate_ = 0;
    std::vector<uint8_t> codecConfig_;
    int32_t timeScale_ = 0;
    int64_t startTimestampUs_ = 0;
    int64_t durationUs_ = 0;
    int64_t lastTimestampUs_ = INT64_MIN;
    int64_t lastDuration_ = 0;
    int64_t pos_ = 0;
    uint64_t allSampleSize_ = 0;
    bool isSameSize_ = true;
    bool isAuxiliary_ = false;
    uint32_t hdlrType_ = 0;
    std::string hdlrName_ = "";
    std::shared_ptr<BasicBox> moov_ = nullptr;
    std::shared_ptr<SttsBox> stts_ = nullptr;
    std::shared_ptr<StszBox> stsz_ = nullptr;
    std::shared_ptr<StscBox> stsc_ = nullptr;
    std::shared_ptr<StcoBox> stco_ = nullptr;
    uint32_t trefTag_ = 0;
    std::shared_ptr<std::vector<uint32_t>> srcTrackIds_;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif