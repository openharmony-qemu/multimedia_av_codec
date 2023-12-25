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

#ifndef HCODEC_TESTER_CAPI_H
#define HCODEC_TESTER_CAPI_H

#include "tester_common.h"
#include "native_avmagic.h"

namespace OHOS::MediaAVCodec {
struct TesterCapi : TesterCommon {
    explicit TesterCapi(const CommandOpt& opt) : TesterCommon(opt) {}

protected:
    bool Create() override;
    bool SetCallback() override;
    bool GetInputFormat() override;
    bool GetOutputFormat() override;
    bool Start() override;
    bool Stop() override;
    bool Release() override;
    bool Flush() override;
    void ClearAllBuffer() override;
    std::optional<uint32_t> GetInputIndexForAsharedMem(Span& span) override;
    std::optional<uint32_t> GetInputIndexForAvBuffer(std::shared_ptr<AVBuffer>& avBuffer) override;
    bool QueueInputForAsharedMem(uint32_t idx, OH_AVCodecBufferAttr attr) override;
    bool QueueInputForAvBuffer(uint32_t idx) override;
    std::optional<uint32_t> GetOutputIndex(Span& span, int64_t& pts) override;
    std::optional<uint32_t> GetOutputIndexForASharedMem(Span& span, int64_t& pts);
    std::optional<uint32_t> GetOutputIndexForAvBuffer(Span& span, int64_t& pts);
    bool ReturnOutputForASharedMem(uint32_t idx);
    bool ReturnOutputForAvBuffer(uint32_t idx);
    bool ReturnOutput(uint32_t idx) override;

    bool ConfigureEncoder() override;
    sptr<Surface> CreateInputSurface() override;
    bool NotifyEos() override;
    bool RequestIDR() override;
    std::optional<uint32_t> GetInputStride() override;

    bool SetOutputSurface(sptr<Surface>& surface) override;
    bool ConfigureDecoder() override;

    // callback for both sharedmem and avbuffer
    static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);

    // callback for sharedmem
    static void OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
    static void OnNewOutputData(
        OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData);

    // callback for avbuffer
    static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNeedOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

    OH_AVCodec *codec_ = nullptr;
    std::list<std::pair<uint32_t, OH_AVMemory*>> asharedMemInputList_;
    std::list<std::pair<uint32_t, std::shared_ptr<OHOS::Media::AVBuffer>>> avBufferInputList_;
    std::list<std::tuple<uint32_t, OH_AVMemory*, OH_AVCodecBufferAttr>> asharedMemOutputList_;
    std::list<std::pair<uint32_t, std::shared_ptr<OHOS::Media::AVBuffer>>> avBufferOutputList_;

    std::shared_ptr<OH_AVFormat> inputFmt_;
};
}
#endif