/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef IDEMUXER_ENGINE_H
#define IDEMUXER_ENGINE_H

#include <cstdint>
#include "meta/format.h"
#include "avcodec_common.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
class IDemuxerEngine {
public:
    virtual ~IDemuxerEngine() = default;
    virtual int32_t SelectTrackByID(uint32_t trackIndex) = 0;
    virtual int32_t UnselectTrackByID(uint32_t trackIndex) = 0;
    virtual int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample) = 0;
    virtual int32_t SeekToTime(int64_t millisecond, AVSeekMode mode) = 0;
    virtual int32_t GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos)
    {
        (void)infos;
        return 0;
    }
    virtual void SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback) = 0;
};

class __attribute__((visibility("default"))) IDemuxerEngineFactory {
public:
    static std::shared_ptr<IDemuxerEngine> CreateDemuxerEngine(uintptr_t sourceAddr);
private:
    IDemuxerEngineFactory() = default;
    ~IDemuxerEngineFactory() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // IDEMUXER_ENGINE_H