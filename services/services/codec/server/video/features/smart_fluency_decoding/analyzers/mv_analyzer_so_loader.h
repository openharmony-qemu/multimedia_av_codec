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

#ifndef MV_ANALYZER_SO_LOADER_H
#define MV_ANALYZER_SO_LOADER_H

#include <mutex>
#include "mv_analyzer.h"
#include "mv_analyzer_c_api.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
class MvAnalyzerSoLoader : public IMvAnalyzer {
public:
    explicit MvAnalyzerSoLoader(SFDCodecType codecType);
    ~MvAnalyzerSoLoader() override;

    void SetStreamAuxInfo(const StreamAuxInfo& info) override;
    void SetVideoInfo(uint32_t width, uint32_t height) override;
    MVStats Analyze(const uint8_t *buffer) override;
    SFDCodecType GetCodecType() const override;
    bool IsValid() const;

private:
    bool LoadLibrary();
    void UnloadLibrary();

    SFDCodecType codecType_;
    void* handle_{nullptr};
    MvAnalyzerHandle soAnalyzer_{nullptr};
    mutable std::mutex apiMutex_;

    using CreateFunc = MvAnalyzerHandle (*)(int32_t);
    using DestroyFunc = void (*)(MvAnalyzerHandle);
    using SetAuxInfoFunc = void (*)(MvAnalyzerHandle, const StreamAuxInfo*);
    using SetVideoInfoFunc = void (*)(MvAnalyzerHandle, uint32_t, uint32_t);
    using ParseFunc = MVStats (*)(MvAnalyzerHandle, const uint8_t*);

    CreateFunc createFunc_{nullptr};
    DestroyFunc destroyFunc_{nullptr};
    SetAuxInfoFunc setAuxInfoFunc_{nullptr};
    SetVideoInfoFunc setVideoInfoFunc_{nullptr};
    ParseFunc parseFunc_{nullptr};
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MV_ANALYZER_SO_LOADER_H