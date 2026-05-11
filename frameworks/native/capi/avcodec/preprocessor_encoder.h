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

#ifndef PREPROCESSOR_ENCODER_H
#define PREPROCESSOR_ENCODER_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include "native_avbuffer.h"
#include "native_avcodec_base.h"
#include "native_avmagic.h"
#include "native_avcodec_videoencoder.h"
#include "preprocessor_manager.h"
#include "video_encoder_object.h"

namespace OHOS {
namespace MediaAVCodec {
class PreprocessorEncoder : public OH_AVCodec {
public:
    explicit PreprocessorEncoder(AVMagic magic);
    ~PreprocessorEncoder();

    static OH_AVErrCode CreatePrimary(const char* mime, OH_AVCodec** codec);
    static OH_AVErrCode CreateSecondary(OH_AVCodec* primary, OH_AVCodec** secondary);
    static void Destroy(OH_AVCodec* codec);

    static bool IsPrimaryEncoderMagic(AVMagic magic);
    static bool IsSecondaryEncoderMagic(AVMagic magic);
    static bool IsPreprocEncoderMagic(AVMagic magic);
    static bool IsValidPreprocEncoderMime(const char* mime);

    OH_AVErrCode RegisterCallback(struct OH_AVCodecCallback callback, void* userData);
    OH_AVErrCode Configure(const Media::Format& format);
    OH_AVErrCode SetParameter(const Media::Format& format);
    sptr<Surface> GetSurface();
    OH_AVErrCode Start();
    OH_AVErrCode Stop();
    OH_AVErrCode Flush();
    OH_AVErrCode NotifyEos();
    OH_AVErrCode Reset();
    OH_AVErrCode ReleaseOutputBuffer(uint32_t index);
    OH_AVErrCode Release();

    std::optional<OH_AVBuffer*> GetTransData(const uint32_t& index, std::shared_ptr<AVBuffer>& buffer, bool isOutput);
    std::shared_ptr<Preprocessor> GetPreprocessor();
    OH_AVFormat* GetOutputDescription();
    OH_AVFormat* GetInputDescription();
    PreprocessorEncoder* DetachSecondaryEncoder();
    void DetachFromPrimaryEncoder();
private:
    void OnStreamChanged(const Media::Format& format);
    void OnPreprocessError(int32_t errorCode);
    static std::string GenerateEncoderId(const std::string& prefix);
    OH_AVErrCode InitAsPrimary(const char* mime);
    OH_AVErrCode InitAsSecondary(PreprocessorEncoder* primary);

    int32_t ConfigureVideoEncoder(const Media::Format& format);
    int32_t ConfigurePreprocessor(const Media::Format& format);
    int32_t CreateEncoderSurface();

    void CreateSharedSurface();
    sptr<Surface> GetSharedSurface();
    void RegisterEncoderToManager();
    void UnregisterEncoderFromManager();
    void DetachPeerEncoder();

    std::shared_ptr<VideoEncoderObject> videoEncoderObject_{nullptr};

    bool isPrimary_{false};
    std::string mime_;
    std::string encoderId_;
    bool configured_{false};

    std::mutex instanceMutex_;
    PreprocessorEncoder* primary_{nullptr};
    PreprocessorEncoder* secondary_{nullptr};

    std::shared_ptr<PreprocessorManager> preprocMgr_{nullptr};
    std::unordered_map<std::string_view, std::shared_ptr<Preprocessor>> preprocessors_{};

    OHOS::sptr<OHOS::Surface> encoderSurface_{nullptr};
    std::queue<sptr<SurfaceBuffer>> writableBufferQueue_{}; // queue of surface buffers for writing preprocessing result

    OH_AVCodecAsyncCallback asyncCallback_{};
    void* userData_{nullptr};
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // PREPROCESSOR_ENCODER_H
