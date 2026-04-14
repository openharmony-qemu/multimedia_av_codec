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

#ifndef NALU_ANALYZER_SO_LOADER_H
#define NALU_ANALYZER_SO_LOADER_H

#include <mutex>
#include "nalu_analyzer.h"
#include "nalu_analyzer_c_api.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
class NaluAnalyzerSoLoader : public INaluAnalyzer {
public:
    explicit NaluAnalyzerSoLoader(SFDCodecType codecType);
    ~NaluAnalyzerSoLoader() override;

    void SetAuxInfoCallback(OnStreamAuxInfoParsed callback) override;
    bool IsNonReferenceFrame(const std::shared_ptr<AVBuffer>& inputBuffer) override;

private:
    static void StaticAuxInfoCallback(const StreamAuxInfo* info, void* userData);
    bool LoadLibrary();
    void UnloadLibrary();

    SFDCodecType codecType_;
    void* handle_{nullptr};
    OnStreamAuxInfoParsed callback_{nullptr};
    std::mutex apiMutex_;

    using CreateNaluAnalyzerFunc = NaluAnalyzerHandle (*)(int32_t codecType);
    using DestroyAnalyzerFunc = void (*)(NaluAnalyzerHandle analyzer);
    using SetCallbackFunc = void (*)(NaluAnalyzerHandle analyzer, NaluStreamAuxInfoCallback cb, void* userData);
    using IsNonReferenceFunc = bool (*)(NaluAnalyzerHandle analyzer, const uint8_t* bufferAddr, uint32_t bufferSize);

    NaluAnalyzerHandle soAnalyzer_{nullptr};
    CreateNaluAnalyzerFunc createFunc_{nullptr};
    DestroyAnalyzerFunc destroyFunc_{nullptr};
    SetCallbackFunc setCallbackFunc_{nullptr};
    IsNonReferenceFunc isNonReferenceFunc_{nullptr};
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // NALU_ANALYZER_SO_LOADER_H
