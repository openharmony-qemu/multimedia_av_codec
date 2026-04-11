/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MV_ANALYZER_H
#define MV_ANALYZER_H

#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class IMvAnalyzer {
public:
    virtual ~IMvAnalyzer() = default;
    virtual void SetStreamAuxInfo(const StreamAuxInfo& info) = 0;
    virtual void SetVideoInfo(uint32_t width, uint32_t height) = 0;
    virtual MVStats parse(const uint8_t *buffer, size_t bufferSize) = 0;
    virtual SFDCodecType GetCodecType() const = 0;
    virtual const MVStats &GetStats() const = 0;
    virtual void Reset() = 0;
    virtual size_t GetMvDataSize() const = 0;
};

class MvAnalyzerFactory {
public:
    static std::unique_ptr<IMvAnalyzer> CreateMvAnalyzer(SFDCodecType codecType);
    static bool IsCodecSupported(SFDCodecType codecType);
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif // MV_ANALYZER_INTERFACE_H