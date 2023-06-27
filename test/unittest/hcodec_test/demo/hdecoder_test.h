/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HDECODER_TEST_H
#define HDECODER_TEST_H

#include <string>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <list>
#include <vector>
#include <optional>
#include "hcodec_api.h"
#include "window.h"  // foundation/window/window_manager/interfaces/innerkits/wm/
#include "start_code_detector.h"
#include "command_parser.h"

namespace OHOS::MediaAVCodec {
class HDecoderTest {
public:
    explicit HDecoderTest(const CommandOpt& opt) : opt_(opt) { }
    ~HDecoderTest();
    void Run();

private:
    struct CallBack : public AVCodecCallback {
        explicit CallBack(HDecoderTest *test) : mTest(test) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index) override;
        void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    private:
        HDecoderTest *mTest;
    };

    class Listener : public IBufferConsumerListener {
    public:
        explicit Listener(HDecoderTest *test) : mTest(test) {}
        void OnBufferAvailable() override;
    private:
        HDecoderTest *mTest;
    };

    sptr<Surface> CreateSurfaceFromWindow();
    sptr<Surface> CreateSurfaceNormal();

    std::optional<std::pair<AVCodecBufferInfo, AVCodecBufferFlag>> GetNextSample(
        const std::shared_ptr<AVSharedMemoryBase>& mem);
    void DealWithInputLoop();
    void DealWithOutputLoop();
    void DealWithUserFlush();

    HCodecDemuxer mDemuxer;
    std::list<PositionPair> mUserSeekPos;
    std::list<size_t> mNaluSeekPos;
    std::mutex mFlushMtx;
    std::condition_variable mFlushCond;

    CommandOpt opt_;
    std::ifstream mIfs;
    CodeType mType = H264;
    sptr<OHOS::Rosen::Window> mWindow;
    sptr<Surface> mConsumer;
    std::shared_ptr<CodecBase> mDecoder;

    std::mutex mInputMtx;
    std::condition_variable mInputCond;
    std::list<uint32_t> mInputList;

    std::mutex mOutputMtx;
    std::condition_variable mOutputCond;
    std::list<std::tuple<uint32_t, AVCodecBufferInfo, AVCodecBufferFlag>> mOutputList;
};
} // namespace OHOS::MediaAVCodec
#endif // HDECODER_TEST_H