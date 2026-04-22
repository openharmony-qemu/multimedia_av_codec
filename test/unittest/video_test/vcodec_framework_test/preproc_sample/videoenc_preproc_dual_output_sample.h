/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef VIDEOENC_PREPROC_DUAL_OUTPUT_SAMPLE_H
#define VIDEOENC_PREPROC_DUAL_OUTPUT_SAMPLE_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avformat.h"
#include "native_window.h"
#include "surface.h"

#include "videoenc_preproc_dual_output_scenarios.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoEncPreprocDualOutputSample {
public:
    VideoEncPreprocDualOutputSample() = default;
    ~VideoEncPreprocDualOutputSample();

    VideoEncPreprocDualOutputSample(const VideoEncPreprocDualOutputSample &) = delete;
    VideoEncPreprocDualOutputSample &operator=(const VideoEncPreprocDualOutputSample &) = delete;

    bool SetOutputFilePath(const std::string &path);
    bool CreatePrimary(const std::string &mime);
    int32_t ConfigurePrimary(const PreprocDualOutput::ScenarioCase &scenario);
    int32_t GetPrimarySurface();
    int32_t StartPrimary();
    int32_t SendPrimaryFrames(uint32_t frameCount);
    bool WaitForDualEos(uint32_t timeoutSeconds = 20);

    int32_t CreateSecondary();
    int32_t ConfigureSecondary(const PreprocDualOutput::ScenarioCase &scenario);
    int32_t StartSecondary();
    int32_t DuplicateSecondary();
    int32_t DestroyPrimary();
    int32_t DestroySecondary();
    void ForgetSecondaryHandle();

    bool PrimaryHasData() const;
    bool SecondaryHasData() const;
    bool PrimaryGotEos() const;
    bool SecondaryGotEos() const;
    int32_t PrimaryError() const;
    int32_t SecondaryError() const;

private:
    struct EncoderState {
        OH_AVCodec *codec = nullptr;
        std::atomic<int32_t> dataCount {0};
        std::atomic<bool> gotEos {false};
        std::atomic<int32_t> errorCode {0};
        std::mutex *eosMutex = nullptr;
        std::condition_variable *eosCv = nullptr;
        std::mutex *outputFileMutex = nullptr;
        std::ofstream *outputFile = nullptr;
    };

    static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedOutputData(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void DumpOutputBuffer(OH_AVBuffer *buffer, const OH_AVCodecBufferAttr &attr, EncoderState &state);

    int32_t RegisterCallback(OH_AVCodec *codec, EncoderState &state);
    int32_t BuildAndConfigure(OH_AVCodec *codec, const PreprocDualOutput::PreprocConfig &config);
    static bool ApplyPreprocConfig(OH_AVFormat *format, const PreprocDualOutput::PreprocConfig &config);
    bool RequestAndFlushFrame(int32_t width, int32_t height, uint8_t pattern, int64_t timestampUs);

    void ReleaseWindow();
    void ResetStates();

    static constexpr int32_t kWidth_ = 1280;
    static constexpr int32_t kHeight_ = 720;

    OH_AVCodec *primary_ = nullptr;
    OH_AVCodec *secondary_ = nullptr;
    OHNativeWindow *window_ = nullptr;
    EncoderState primaryState_ {};
    EncoderState secondaryState_ {};
    std::mutex eosMutex_;
    std::condition_variable eosCv_;
    std::mutex outputFileMutex_;
    std::ofstream outputFile_;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // VIDEOENC_PREPROC_DUAL_OUTPUT_SAMPLE_H
