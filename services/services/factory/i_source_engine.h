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

#ifndef ISOURCE_ENGINE_H
#define ISOURCE_ENGINE_H

#include <cstdint>
#include "meta/format.h"
#include "avcodec_common.h"
#include "buffer/avsharedmemory.h"
#include "i_source_engine.h"

namespace OHOS {
namespace MediaAVCodec {
class ISourceEngine {
public:
    virtual ~ISourceEngine() = default;
    virtual int32_t Init() = 0;
    virtual int32_t GetTrackCount(uint32_t &trackCount) = 0;
    virtual int32_t GetSourceFormat(Format &format) = 0;
    virtual int32_t GetTrackFormat(Format &format, uint32_t trackIndex) = 0;
    virtual uintptr_t GetSourceAddr() = 0;
};

class __attribute__((visibility("default"))) ISourceEngineFactory {
public:
    static std::shared_ptr<ISourceEngine> CreateSourceEngine(const std::string& uri);
private:
    ISourceEngineFactory() = default;
    ~ISourceEngineFactory() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // ISOURCE_ENGINE_H