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

#ifndef AVCODEC_VIDEO_DECODER_DEMO_H
#define AVCODEC_VIDEO_DECODER_DEMO_H

#include <atomic>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include <string>

#include "surface/window.h"
#include "nocopyable.h"
#include "native_avcodec_videodecoder.h"
#include "avcodec_video_decoder_inner_demo.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace MediaAVCodec {
namespace VideoDemo {
class VDecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
};

class VDecDemo : public NoCopyable {
public:
    VDecDemo();
    virtual ~VDecDemo();
    void RunCase(std::string &mode);
    void RunDrmCase();

private:
    int32_t CreateDec();
    int32_t Configure(OH_AVFormat *format);
    int32_t SetSurface(OHNativeWindow *window);
    sptr<Surface> GetSurface(std::string &mode);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t ExtractPacket();
    void InputFunc();
    void OutputFunc();

    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> inputFile_ = nullptr;
    std::unique_ptr<std::ofstream> outFile_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_ = nullptr;
    std::unique_ptr<std::thread> outputLoop_ = nullptr;
    OH_AVCodec *videoDec_ = nullptr;
    VDecSignal *signal_ = nullptr;
    struct OH_AVCodecAsyncCallback cb_;
    bool isFirstFrame_ = true;
    int64_t timeStamp_ = 0;

    // Extract packet
    static constexpr int32_t VIDEO_INBUF_SIZE = 10240;
    static constexpr int32_t VIDEO_REFILL_THRESH = 4096;
    const AVCodec *codec_ = nullptr;
    AVCodecParserContext *parser_ = nullptr;
    AVCodecContext *codec_ctx_ = nullptr;
    AVPacket *pkt_ = nullptr;
    size_t data_size_ = 0;
    uint8_t *data_ = nullptr;
    uint8_t inbuf_[VIDEO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE] = {0};
    bool file_end_ = false;
    std::string mode_ = "0";
};
} // namespace VideoDemo
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_VIDEO_DECODER_DEMO_H
