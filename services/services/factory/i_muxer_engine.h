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

#ifndef IMUXER_ENGINE_H
#define IMUXER_ENGINE_H

#include <cstdint>
#include "buffer/avsharedmemory.h"
#include "media_description.h"
#include "av_common.h"
#include "avcodec_common.h"

namespace OHOS {
namespace MediaAVCodec {
class IMuxerEngine {
public:
    virtual ~IMuxerEngine() = default;
    virtual int32_t SetRotation(int32_t rotation) = 0;
    virtual int32_t AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WriteSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
        AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t DumpInfo(int32_t fd) = 0;
};

class __attribute__((visibility("default"))) IMuxerEngineFactory {
public:
    static std::shared_ptr<IMuxerEngine> CreateMuxerEngine(
        int32_t appUid, int32_t appPid, int32_t fd, OutputFormat format);
private:
    IMuxerEngineFactory() = default;
    ~IMuxerEngineFactory() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // IMUXER_ENGINE_H