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
#include "basic_track.h"
#include "video_parser.h"
#include "meta/video_types.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class VideoTrack : public BasicTrack {
public:
    VideoTrack(std::string mimeType, std::shared_ptr<BasicBox> moov);
    ~VideoTrack() override;
    Status Init(const std::shared_ptr<Meta> &trackDesc) override;
    void SetRotation(uint32_t rotation) override;
    Status WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample) override;
    Status WriteTailer() override;
    Any GetPointer() override {return Any(this);}
    int32_t GetWidth();
    int32_t GetHeight();
    bool IsColor();
    ColorPrimary GetColorPrimaries();
    TransferCharacteristic GetColorTransfer();
    MatrixCoefficient GetColorMatrixCoeff();
    bool GetColorRange();
    bool IsCuvaHDR();
    void SetCttsBox(std::shared_ptr<SttsBox> ctts);
    void SetStssBox(std::shared_ptr<StssBox> stss);

private:
    int32_t CreateParser();
    void ParserSetConfig();
    void InitColor(const std::shared_ptr<Meta> &trackDesc);
    void InitCuva(const std::shared_ptr<Meta> &trackDesc);
    void DisposeCtts(int64_t pts);
    void DisposeCtts();
    void DisposeCttsByFrameRate(int64_t pts);
    void DisposeStts();
    void DisposeStts(int64_t duration, int64_t pts);
    void DisposeSttsNoPts();
    void DisposeStco(int64_t pos);
    void DisposeStsz(uint32_t size);
    void DisposeDuration();
    void DisposeBitrate();
    void DisposeColor();
    void DisposeCuva();
    int32_t width_ = 0;
    int32_t height_ = 0;
    int64_t delay_ = 0;
    double frameRate_ = 0;
    bool isColor_ = false;
    ColorPrimary colorPrimaries_ = ColorPrimary::UNSPECIFIED;
    TransferCharacteristic colorTransfer_ = TransferCharacteristic::UNSPECIFIED;
    MatrixCoefficient colorMatrixCoeff_ = MatrixCoefficient::UNSPECIFIED;
    bool colorRange_ = false;
    bool isCuvaHDR_ = false;
    std::shared_ptr<VideoParser> videoParser_ = nullptr;
    std::shared_ptr<SttsBox> ctts_ = nullptr;
    std::shared_ptr<StssBox> stss_ = nullptr;
    std::queue<int64_t> allPts_;
    bool isHaveBFrame_ = false;
    int64_t dts_ = 0;
    int64_t startDts_ = 0;
    bool hasSetParserConfig_ = false;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif