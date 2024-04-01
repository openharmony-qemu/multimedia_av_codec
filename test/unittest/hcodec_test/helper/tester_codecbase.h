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

#ifndef HCODEC_TESTER_CODECBASE_H
#define HCODEC_TESTER_CODECBASE_H

#include "tester_common.h"
#include "codecbase.h"

namespace OHOS::MediaAVCodec {
struct TesterCodecBase : TesterCommon {
    explicit TesterCodecBase(const CommandOpt& opt) : TesterCommon(opt) {}

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
    bool WaitForInput(BufInfo& buf) override;
    bool WaitForOutput(BufInfo& buf) override;
    bool ReturnInput(const BufInfo& buf) override;
    bool ReturnOutput(uint32_t idx) override;
    void EnableHighPerf(Format& fmt) const;

    bool ConfigureEncoder() override;
    bool SetEncoderParameter(const SetParameterParams& param) override;
    bool SetEncoderPerFrameParam(BufInfo& buf, const PerFrameParams& param) override;
    sptr<Surface> CreateInputSurface() override;
    bool NotifyEos() override;
    bool RequestIDR() override;
    std::optional<uint32_t> GetInputStride() override;

    bool SetOutputSurface(sptr<Surface>& surface) override;
    bool ConfigureDecoder() override;

    struct CallBack : public MediaCodecCallback {
        explicit CallBack(TesterCodecBase* tester) : tester_(tester) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    private:
        TesterCodecBase* tester_;
    };

    std::shared_ptr<CodecBase> codec_;
    std::list<std::pair<uint32_t, std::shared_ptr<AVBuffer>>> inputList_;
    std::list<std::pair<uint32_t, std::shared_ptr<AVBuffer>>> outputList_;

    Format inputFmt_;
};
}
#endif