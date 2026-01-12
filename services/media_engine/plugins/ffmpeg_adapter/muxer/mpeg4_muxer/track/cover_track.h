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

#ifndef MPEG4_MUXER_COVER_TRACK_H
#define MPEG4_MUXER_COVER_TRACK_H

#include "basic_box.h"
#include "basic_track.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class CoverTrack : public BasicTrack {
public:
    CoverTrack(std::string mimeType, std::shared_ptr<BasicBox> moov, bool &hasAigc);
    ~CoverTrack() override;
    Status Init(const std::shared_ptr<Meta> &trackDesc) override;
    Status WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample) override;
    Status WriteTailer() override;

private:
    int32_t width_ = 0;
    int32_t height_ = 0;
    std::shared_ptr<DataBox> data_ = nullptr;
    bool &hasAigc_;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif