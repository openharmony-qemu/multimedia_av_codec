/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_MUXER_TIMED_META_TRACK_H
#define MPEG4_MUXER_TIMED_META_TRACK_H

#include "basic_track.h"
#include "box_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class TimedMetaTrack : public BasicTrack {
public:
    TimedMetaTrack(std::string mimeType, std::shared_ptr<BasicBox> moov,
        std::vector<std::shared_ptr<BasicTrack>> &tracks, bool &useTimedMeta);
    ~TimedMetaTrack() override;
    Status Init(const std::shared_ptr<Meta> &trackDesc) override;
    Status WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample) override;
    Status WriteTailer() override;
    Any GetPointer() override {return Any(this);}
    std::shared_ptr<Meta> &GetTrackDesc() {return trackDesc_;}

private:
    void DisposeDuration();
    void SetIsAuxiliary();
    bool GetSrcTrackDurationUs(int64_t &durationUs, int64_t &startTimeUs);
    int32_t frameSize_ = 0;
    std::shared_ptr<Meta> trackDesc_;
    std::vector<std::shared_ptr<BasicTrack>> &tracks_;
    bool &useTimedMeta_;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif