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

#ifndef HENCODER_TEST_H
#define HENCODER_TEST_H

#include <string>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <list>
#include <optional>
#include "hcodec_api.h"
#include "start_code_detector.h"
#include "command_parser.h"

namespace OHOS::MediaAVCodec {
class HEncoderTest {
public:
    explicit HEncoderTest(const CommandOpt& opt) : opt_(opt) {}
    void Run();

private:
    struct CallBack : public AVCodecCallback {
        explicit CallBack(HEncoderTest *test) : mTest(test) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index) override;
        void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    private:
        HEncoderTest *mTest;
    };
    void DealWithInputByteBufferLoop();
    void DealWithInputSurfaceLoop();
    void DealWithOutputLoop();

    uint32_t ReadOneFrame(char* dst);
    uint32_t ReadOneFrameYUV420P(char* dst);
    uint32_t ReadOneFrameYUV420SP(char* dst);
    uint32_t ReadOneFrameRGBA(char* dst);

    CommandOpt opt_;
    std::ifstream ifs_;
    CodeType mType = H264;
    GraphicPixelFormat displayFmt_;
    uint32_t stride_ = 0;

    std::shared_ptr<CodecBase> encoder_;
    sptr<Surface> surface_;

    std::mutex mInputMtx;
    std::condition_variable mInputCond;
    std::list<uint32_t> mInputList;

    std::mutex mOutputMtx;
    std::condition_variable mOutputCond;
    std::list<std::tuple<uint32_t, AVCodecBufferInfo, AVCodecBufferFlag>> mOutputList;

    uint32_t curFrameNum = 0;

    static constexpr uint32_t SAMPLE_RATIO = 2;
    static constexpr uint32_t BYTES_PER_PIXEL_RBGA = 4;
};
}
#endif // HENCODER_TEST_H