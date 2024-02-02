/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef AVDECODER_DEMO_COMMON_H
#define AVDECODER_DEMO_COMMON_H

#include <iostream>
#include <atomic>
#include <fstream>
#include <queue>
#include <string>
#include <vector>
#include <thread>
#include <sys/time.h>

#include "securec.h"
#include "media_description.h"
#include "avcodec_audio_decoder.h"
#include "nocopyable.h"
#include "native_avcodec_audiodecoder.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
    namespace MediaAVCodec {
        extern void OnError(OH_AVCodec* codec, int32_t errorCode, void* userData);
        extern void OnOutputFormatChanged(OH_AVCodec* codec, OH_AVFormat* format, void* userData);
        extern void OnInputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data, void* userData);
        extern void OnOutputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data,
            OH_AVCodecBufferAttr* attr, void* userData);

        constexpr double DEFAULT_TIME_NUM = 1000000.0;
        constexpr int32_t CODE_ERROR = -1;
        constexpr uint32_t ERROR_INDEX = 100;

        typedef enum MyTimer {
            TIMER_NONE = 0,
            TIMER_INPUT = 1,
            TIMER_FREEOUTPUT = 2,
            TIMER_GETOUTPUTDESCRIPTION = 3
        } MyTimer;

        class ADecSignal {
        public:
            std::mutex inMutex_;
            std::mutex outMutex_;
            std::condition_variable inCond_;
            std::condition_variable outCond_;
            std::queue<uint32_t> inQueue_;
            std::queue<uint32_t> outQueue_;
            std::queue<OH_AVMemory*> inBufferQueue_;
            std::queue<OH_AVMemory*> outBufferQueue_;
            std::queue<OH_AVCodecBufferAttr> attrQueue_;
            std::queue<AVCodecBufferInfo> infoQueue_;
            std::queue<AVCodecBufferFlag> flagQueue_;
            std::queue<std::shared_ptr<AVSharedMemory>> inInnerBufQueue_;
            std::queue<std::shared_ptr<AVSharedMemory>> outInnerBufQueue_;
        };
        class InnerADecDemoCallback : public AVCodecCallback, public NoCopyable {
        public:
            explicit InnerADecDemoCallback(std::shared_ptr<ADecSignal> signal);
            ~InnerADecDemoCallback() = default;

            void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
            void OnOutputFormatChanged(const Format& format) override;
            void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
            void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                std::shared_ptr<AVSharedMemory> buffer) override;
        private:
            std::shared_ptr<ADecSignal> innersignal_;
        };

        class AudioDecoderDemo : public NoCopyable {
        public:
            AudioDecoderDemo();
            ~AudioDecoderDemo();

            // native api
            OH_AVCodec* NativeCreateByMime(const char* mime);
            OH_AVCodec* NativeCreateByName(const char* name);
            OH_AVErrCode NativeDestroy(OH_AVCodec *codec);
            OH_AVErrCode NativeSetCallback(OH_AVCodec* codec, OH_AVCodecAsyncCallback callback);
            OH_AVErrCode NativeConfigure(OH_AVCodec* codec, OH_AVFormat* format);
            OH_AVErrCode NativePrepare(OH_AVCodec* codec);
            OH_AVErrCode NativeStart(OH_AVCodec* codec);
            OH_AVErrCode NativeStop(OH_AVCodec* codec);
            OH_AVErrCode NativeFlush(OH_AVCodec* codec);
            OH_AVErrCode NativeReset(OH_AVCodec* codec);
            OH_AVFormat* NativeGetOutputDescription(OH_AVCodec* codec);
            OH_AVErrCode NativeSetParameter(OH_AVCodec* codec, OH_AVFormat* format);
            OH_AVErrCode NativePushInputData(OH_AVCodec* codec, uint32_t index, OH_AVCodecBufferAttr attr);
            OH_AVErrCode NativeFreeOutputData(OH_AVCodec* codec, uint32_t index);
            OH_AVErrCode NativeIsValid(OH_AVCodec* codec, bool* isVaild);

            void stopThread();
            void updateInputData();
            void updateOutputData();

            void setTimerFlag(int32_t flag);

            uint32_t NativeGetInputIndex();
            uint8_t* NativeGetInputBuf();
            uint32_t NativeGetOutputIndex();

            void HandleEOS(const uint32_t& index);
            int32_t NativePushInput(uint32_t index, OH_AVMemory* buffer);
            void NativeGetDescription();
            void NativeWriteOutput(std::ofstream& pcmFile, uint32_t index,
                OH_AVCodecBufferAttr attr, OH_AVMemory* data);

            void NativeInputFunc();
            void NativeOutputFunc();
            void NativeGetVorbisConf(OH_AVFormat* format);
            void NativeCreateToStart(const char* name, OH_AVFormat* format);
            void NativeStopDec();
            void NativeFFmpegConf(const char* name, OH_AVFormat* format);
            void NativeCloseFFmpeg();
            void NativeStopAndClear();

            void NativeRunCase(std::string inputFile, std::string outputFile,
                const char* name, OH_AVFormat* format);
            void NativeRunCaseWithoutCreate(OH_AVCodec* handle, std::string inputFile, std::string outputFile,
                OH_AVFormat* format, const char* name, bool needConfig);
            void NativeRunCasePerformance(std::string inputFile, std::string outputFile,
                const char* name, OH_AVFormat* format);
            void NativeRunCaseFlush(std::string inputFile, std::string outputFileFirst,
                std::string outputFileSecond, const char* name, OH_AVFormat* format);
            void NativeRunCaseReset(std::string inputFile, std::string outputFileFirst,
                std::string outputFileSecond, const char* name, OH_AVFormat* format);
            OH_AVFormat* NativeRunCaseGetOutputDescription(std::string inputFile,
                std::string outputFile, const char* name, OH_AVFormat* format);

            // for test
            int32_t TestReadDatFile(uint32_t index, OH_AVMemory* buffer);
            void TestInputFunc();
            void TestRunCase(std::string inputFile, std::string outputFile, const char* name, OH_AVFormat* format);
            void TestFFmpeg(std::string inputFile);

            // Inner api
            int32_t InnerCreateByMime(const std::string& mime);
            int32_t InnerCreateByName(const std::string& name);
            int32_t InnerConfigure(const Format& format);
            int32_t InnerPrepare();
            int32_t InnerStart();
            int32_t InnerStop();
            int32_t InnerFlush();
            int32_t InnerReset();
            int32_t InnerRelease();
            int32_t InnerQueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);

            int32_t InnerGetOutputFormat(Format& format);
            int32_t InnerReleaseOutputBuffer(uint32_t index);
            int32_t InnerSetParameter(const Format& format);
            int32_t InnerSetCallback(const std::shared_ptr<AVCodecCallback>& callback);
            int32_t InnerDestroy();
            void InnerInputFunc();
            uint32_t InnerInputFuncRead(uint32_t index);
            void InnerOutputFunc();
            void InnerRunCase(std::string inputFile, std::string outputFile,
                const std::string& name, Format& format);

            void InnerRunCaseOHVorbis(const std::string& name, Format& format);
            int InnerRunCasePre();

            void InnerRunCaseFlush(std::string inputFile, std::string outputFileFirst,
                std::string outputFileSecond, const std::string& name, Format& format);

            void InnerRunCaseFlushPost();
            int InnerRunCaseFlushPre();
            void InnerRunCaseFlushOHVorbis(const std::string& name, Format& format);
            void InnerRunCaseFlushAlloc(Format& format);

            void InnerRunCaseReset(std::string inputFile, std::string outputFileFirst,
                std::string outputFileSecond, const std::string& name, Format& format);

            void InnerRunCaseResetPost();
            int InnerRunCaseResetPre();
            void InnerRunCaseResetOHVorbis(const std::string& name, Format& format);
            void InnerRunCaseResetAlloc(Format& format);
            void InnerRunCaseResetInPut();

            void InnerStopThread();
            void InnerUpdateInputData();
            void InnerUpdateOutputData();
            std::shared_ptr<ADecSignal> getSignal();

        private:
            std::atomic<bool> isRunning_ = false;
            std::unique_ptr<std::thread> inputLoop_;
            std::unique_ptr<std::thread> outputLoop_;
            OH_AVCodec* audioDec_;
            ADecSignal* signal_;
            struct OH_AVCodecAsyncCallback cb_;
            bool isFirstFrame_ = true;

            std::ifstream inputFile_;
            std::shared_ptr<AVCodecAudioDecoder> inneraudioDec_;
            std::shared_ptr<ADecSignal> innersignal_;
            std::shared_ptr<InnerADecDemoCallback> innercb_;
            uint32_t frameCount_ = 0;

            std::queue<uint32_t> inIndexQueue_;
            std::queue<uint8_t*> inBufQueue_;
            std::queue<uint32_t> outIndexQueue_;

            std::string outputFilePath;
            std::string inputFilePath;
            OH_AVFormat* curFormat = nullptr;
            bool isGetOutputDescription = false;

            int32_t timerFlag = 0;
            int32_t runTimes = 0;
            double totalTime = 0.0;
            double otherTime = 0.0;
            struct timeval startTime, endTime;
            struct timeval start, end;
            struct timeval inputStart, inputEnd;
            struct timeval outputStart, outputEnd;

            AVFormatContext* fmpt_ctx = nullptr;
            AVFrame* frame;
            AVPacket pkt;
            AVCodecContext* codec_ctx = nullptr;
        };
    }
}
#endif // AVDECODER_DEMO_COMMON_H