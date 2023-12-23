/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef PLUGIN_CORE_DEMUXER_H
#define PLUGIN_CORE_DEMUXER_H

#include "meta/format.h"
#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "demuxer_plugin.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
using namespace Media;
class Demuxer {
public:
    Demuxer(const Demuxer &) = delete;
    Demuxer operator=(const Demuxer &) = delete;
    ~Demuxer() = default;
    int32_t SelectTrackByID(uint32_t trackIndex);
    int32_t UnselectTrackByID(uint32_t trackIndex);
    int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample);
    int32_t SeekToTime(int64_t millisecond, AVSeekMode mode);

private:
    friend class DemuxerFactory;
    Demuxer(uint32_t pkgVer, uint32_t apiVer, std::shared_ptr<DemuxerPlugin> plugin);
    const uint32_t pkgVersion_;
    const uint32_t apiVersion_;
    std::shared_ptr<DemuxerPlugin> demuxer_;
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // PLUGIN_CORE_DEMUXER_H
