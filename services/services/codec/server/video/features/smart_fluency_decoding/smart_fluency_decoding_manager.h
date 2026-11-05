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

#ifndef SMART_FLUENCY_DECODING_MANAGER_H
#define SMART_FLUENCY_DECODING_MANAGER_H

#include "avcodec_dfx_component.h"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <unordered_map>
#include <functional>
#include "buffer/avbuffer.h"
#include "surface_buffer.h"
#include "meta/format.h"
#include "avcodec_info.h"
#include "smart_fluency_decoding.h"
#include "smart_fluency_decoding_builder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
class SmartFluencyDecodingManager : public AVCodecDfxComponent {
public:
    SmartFluencyDecodingManager(const std::string& codecMime, AVCodecType codecType);
    static bool IsCodecSupported(const std::string& codecMime, AVCodecType codecType, bool isSecure = false);
    void Configure(const Media::Format& format);
    void SetCallbacks(SmartFluencyDecoding::AsyncDropInputCallback inputCb,
                      SmartFluencyDecoding::AsyncDropOutputCallback outputCb);
    void CleanResources();
    void Flush();
    bool ShouldReportDfx() const;
    void EnableMVOutput(Media::Format& format);
    void OnInputBufferAvailable(uint32_t index, const std::shared_ptr<AVBuffer>& buffer);
    bool MakePreDecodeDecision(uint32_t index);
    bool MakePostDecodeDecision(uint32_t index, const std::shared_ptr<AVBuffer>& buffer, bool isSurfaceMode);
    void OnOutputBufferUnbinded();
    void OnDecInfoUpdate(double sourceFps, double speed);

private:
    // Requires exclusive ownership of sfdMutex_.
    void ConfigureLocked(const Media::Format& format);
    void CacheCsdData(uint32_t index);
    void ExtractCsdData(const std::shared_ptr<AVBuffer>& buffer);
#ifdef SFD_DEBUG
    void ApplyDebugConfig(Media::Format& format);
    void CheckAndTriggerDebugConfig();
#endif

    std::string codecMime_;
    AVCodecType codecType_;
    // Acquire before csdMutex_/surfaceBufMutex_; keep held through calls into sfd_.
    mutable std::shared_mutex sfdMutex_;
    std::unique_ptr<SmartFluencyDecodingBuilder> builder_{nullptr};
    std::unique_ptr<SmartFluencyDecoding> sfd_{nullptr};
    SmartFluencyDecoding::AsyncDropInputCallback inputCb_;
    SmartFluencyDecoding::AsyncDropOutputCallback outputCb_;
    std::vector<uint8_t> cachedCsdData_;
    bool isCsdCached_ = false;
    std::mutex csdMutex_;
    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> tempCsdBufferMap_;
    std::mutex surfaceBufMutex_;
    std::unordered_map<int64_t, sptr<SurfaceBuffer>> surfaceBufMap_;
#ifdef SFD_DEBUG
    int32_t configMode_ = -1;
    double configRatio_ = -1.0;
    double configDecSpeed_ = -1.0;
    int32_t triggerFrameCnt_ = -1;
    int32_t newMode_ = -1;
    double newRatio_ = -1.0;
    double newDecSpeed_ = -1.0;
    uint32_t inputFrameCount_ = 0;
    bool isDebugConfigInitialized_ = false;
#endif
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SMART_FLUENCY_DECODING_MANAGER_H
