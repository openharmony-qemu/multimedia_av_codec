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

#ifndef NALU_ANALYZER_H
#define NALU_ANALYZER_H

#include "buffer/avbuffer.h"
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
using AVBuffer = OHOS::Media::AVBuffer;
class INaluAnalyzer {
public:
    virtual ~INaluAnalyzer() = default;

    virtual void SetAuxInfoCallback(OnStreamAuxInfoParsed callback) = 0;
    virtual bool IsNonReferenceFrame(const std::shared_ptr<AVBuffer>& inputBuffer) = 0;
};

class NaluAnalyzerFactory {
public:
    static std::unique_ptr<INaluAnalyzer> CreateNaluAnalyzer(SFDCodecType codecType);
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // NALU_ANALYZER_H
