/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef VIDEOENC_API11_SAMPLE_H
#define VIDEOENC_API11_SAMPLE_H

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include "securec.h"
#include "native_avcodec_videoencoder.h"
#include "nocopyable.h"
#include "native_avbuffer.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "surface/window.h"
#include "media_description.h"
#include "av_common.h"
#include "external_window.h"
#include "native_buffer_inner.h"
namespace OHOS
{
    namespace Media
    {
        class VEncSignal
        {
        public:
            std::mutex inMutex_;
            std::condition_variable inCond_;
            std::queue<uint32_t> inIdxQueue_;
            std::queue<OH_AVBuffer *> inBufferQueue_;
            int count = 0;
        };

        class VEncAPI11FuzzSample : public NoCopyable
        {
        public:
            VEncAPI11FuzzSample() = default;
            ~VEncAPI11FuzzSample();
            uint32_t defaultWidth = 1280;
            uint32_t defaultHeight = 720;
            uint32_t defaultBitRate = 5000000;
            uint32_t defaultQuality = 30;
            double defaultFrameRate = 30.0;
            uint32_t maxFrameInput = 20;
            int32_t defaultQP = 20;
            uint32_t defaultBitRateMode = CBR;
            OH_AVPixelFormat defaultPixFmt = AV_PIXEL_FORMAT_NV12;
            uint32_t defaultKeyFrameInterval = 1000;
            const uint8_t *fuzzData;
            size_t fuzzSize;
            int32_t CreateVideoEncoder(const char *codecName);
            int32_t ConfigureVideoEncoder();
            int32_t SetVideoEncoderCallback();
            int32_t StartVideoEncoder();
            void GetStride();
            void WaitForEOS();
            int64_t GetSystemTimeUs();
            int32_t Start();
            int32_t Flush();
            int32_t Reset();
            int32_t Stop();
            int32_t Release();
            void SetEOS(uint32_t index, OH_AVBuffer *buffer);
            void InputFunc();
            void ReleaseInFile();
            void ReleaseSignal();
            void StopInloop();
            VEncSignal *signal_;
            uint32_t frameCount = 0;
            bool sleepOnFPS = false;
            bool surfInput = false;
            std::atomic<bool> isRunning_{false};

        private:
            std::unique_ptr<std::thread> inputLoop_;
            OH_AVCodec *venc_;
            OH_AVCodecCallback cb_;
            int stride_;
        };
    } // namespace Media
} // namespace OHOS

#endif // VIDEOENC_API11_SAMPLE_H
