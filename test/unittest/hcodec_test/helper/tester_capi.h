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
#include "native_avcodec_base.h"

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
    std::optional<uint32_t> GetInputIndex(Span& span) override;
    bool QueueInput(uint32_t idx, OH_AVCodecBufferAttr attr) override;
    std::optional<uint32_t> GetOutputIndex() override;
    bool ReturnOutput(uint32_t idx) override;

    bool ConfigureEncoder() override;
    bool CreateInputSurface() override;
    bool NotifyEos() override;
    bool RequestIDR() override;
    std::optional<uint32_t> GetInputStride() override;

    bool SetOutputSurface(sptr<Surface>& surface) override;
    bool ConfigureDecoder() override;

    static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
    static void OnNewOutputData(
        OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData);

    OH_AVCodec *codec_ = nullptr;
    std::list<std::pair<uint32_t, OH_AVMemory*>> inputList_;
    std::list<std::tuple<uint32_t, OH_AVMemory*, OH_AVCodecBufferAttr>> outputList_;

    std::shared_ptr<OH_AVFormat> inputFmt_;
};
}
#endif