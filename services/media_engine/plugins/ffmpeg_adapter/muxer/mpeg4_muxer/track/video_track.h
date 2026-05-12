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

#ifndef MPEG4_MUXER_VIDEO_TRACK_H
#define MPEG4_MUXER_VIDEO_TRACK_H

#include <queue>
#include <list>
#include "basic_track.h"
#include "video_parser.h"
#include "meta/video_types.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class VideoTrack : public BasicTrack {
public:
    VideoTrack(std::string mimeType, std::shared_ptr<BasicBox> moov, std::vector<std::shared_ptr<BasicTrack>> &tracks);
    ~VideoTrack() override;
    Status Init(const std::shared_ptr<Meta> &trackDesc) override;
    void SetRotation(uint32_t rotation) override;
    Status WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample) override;
    Status WriteTailer() override;
    Any GetPointer() override {return Any(this);}
    void UpdateUserMeta(const std::shared_ptr<Meta> &userMeta) override;
    int32_t GetWidth() const;
    int32_t GetHeight() const;
    bool IsColor() const;
    int32_t GetColorPrimaries() const;
    int32_t GetColorTransfer() const;
    int32_t GetColorMatrixCoeff() const;
    bool GetColorRange() const;
    bool IsCuvaHDR() const;
    void SetCttsBox(std::shared_ptr<SttsBox> ctts);
    void SetStssBox(std::shared_ptr<StssBox> stss);
    bool GetSrcTrackIds(const std::shared_ptr<Meta> &trackDesc);
    
private:
    int32_t CreateParser();
    void ParserSetConfig();
    bool InitColor(const std::shared_ptr<Meta> &trackDesc);
    void InitCuva(const std::shared_ptr<Meta> &trackDesc);
    void DisposeCtts(int64_t pts, int64_t dts);
    void DisposeSttsAndCtts(int64_t ptsUs, int64_t mpeg4Pts);
    void UpdateCtts();
    void DisposeDuration();
    void DisposeBitrate();
    void DisposeColor();
    void DisposeCuva();
    void DisposeSdtp(uint32_t flag);
    void AddSdtpBox();
    bool CheckPts(int64_t pts);
    Status SetVideoDelay(const std::shared_ptr<Meta> &trackDesc);
    int64_t CalculateDtsByPts(int64_t mpeg4Pts);
    int32_t width_ = 0;
    int32_t height_ = 0;
    int64_t delay_ = 0;
    double frameRate_ = 0;
    bool isColor_ = false;
    int32_t colorPrimaries_ = -1;
    int32_t colorTransfer_ = -1;
    int32_t colorMatrixCoeff_ = -1;
    bool colorRange_ = false;
    bool isCuvaHDR_ = false;
    std::shared_ptr<VideoParser> videoParser_ = nullptr;
    std::shared_ptr<SttsBox> ctts_ = nullptr;
    std::shared_ptr<SttsBox> tempCtts_ = nullptr;
    std::shared_ptr<StssBox> stss_ = nullptr;
    std::shared_ptr<SdtpBox> sdtp_ = nullptr;
    int64_t startDts_ = 0;
    int64_t preDts_ = 0;
    int64_t prePtsUs_ = 0;
    int64_t preDuration_ = 0;
    bool hasSetParserConfig_ = false;
    std::vector<std::shared_ptr<BasicTrack>> &tracks_;
    int32_t videoDelay_ = 0;
    std::list<int64_t> dtsList_;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif