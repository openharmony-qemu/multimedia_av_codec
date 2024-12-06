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

#ifndef VCODEC_SIGNAL_H
#define VCODEC_SIGNAL_H
#include <atomic>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>
#include "avcc_reader.h"
#include "codec_buffer_queue.h"
#include "avcodec_log.h"
#include "native_avbuffer.h"
#include "native_avformat.h"
#include "native_avmemory.h"

namespace OHOS {
namespace MediaAVCodec {

class VCodecSampleBase {
public:
    virtual int32_t HandleInputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo) = 0;
    virtual int32_t HandleOutputFrame(std::shared_ptr<CodecBufferInfo> bufferInfo) = 0;
    virtual int32_t Operate() = 0;
};

class VCodecSignal {
public:
    VCodecSignal(std::shared_ptr<VCodecSampleBase> codec) : codecSample_(codec){};
    ~VCodecSignal()
    {
        if (outFile_ != nullptr && outFile_->is_open()) {
            outFile_->close();
        }
    }

    void FlushQueue()
    {
        inQueue_.Flush();
        outQueue_.Flush();
    }

    std::mutex eosMutex_;
    std::condition_variable eosCond_;
    std::atomic<bool> isInEos_ = false;
    std::atomic<bool> isOutEos_ = false;

    CodecBufferQueue inQueue_;
    CodecBufferQueue outQueue_;

    std::vector<int32_t> errors_;
    std::atomic<int32_t> controlNum_ = 0;
    std::atomic<bool> isRunning_ = false;

    std::shared_ptr<DataProducerBase> reader_;
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::weak_ptr<VCodecSampleBase> codecSample_;

    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t widthStride_ = 0;
    int32_t heightStride_ = 0;
    int32_t cropTop_ = 0;
    int32_t cropBottom_ = 0;
    int32_t cropLeft_ = 0;
    int32_t cropRight_ = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VCODEC_SIGNAL_H