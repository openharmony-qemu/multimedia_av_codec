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

#ifndef MPEG4_MUXER_AUDIO_TRACK_H
#define MPEG4_MUXER_AUDIO_TRACK_H

#include "basic_track.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class AudioTrack : public BasicTrack {
public:
    AudioTrack(std::string mimeType, std::shared_ptr<BasicBox> moov, std::vector<std::shared_ptr<BasicTrack>> &tracks);
    ~AudioTrack() override;
    Status Init(const std::shared_ptr<Meta> &trackDesc) override;
    Status WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample) override;
    Status WriteTailer() override;
    Any GetPointer() override {return Any(this);}
    int32_t GetSampleRate() const;
    int32_t GetChannels() const;
    bool GetSrcTrackIds(const std::shared_ptr<Meta> &trackDesc);
    int32_t GetAacAdtsSize(const uint8_t *data, int32_t len);

private:
    void DisposeDuration();
    void DisposeBitrate();
    void DisposeLastDuration();
    int32_t sampleRate_ = 44100;  // 44100
    int32_t channels_ = 2;  // 2
    int32_t frameSize_ = 0;
    std::vector<std::shared_ptr<BasicTrack>> &tracks_;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif