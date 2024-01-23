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

#include "AudioDecoderDemoCommon.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "media_description.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;

namespace OHOS {
namespace MediaAVCodec {
void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
    cout << "Error received, errorCode:" << errorCode << endl;
}

void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
    cout << "OnOutputFormatChanged received" << endl;
}

void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    ADecSignal *signal_ = static_cast<ADecSignal *>(userData);
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                             void *userData)
{
    (void)codec;
    ADecSignal *signal_ = static_cast<ADecSignal *>(userData);
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->outBufferQueue_.push(data);
    if (attr) {
        cout << "OnOutputBufferAvailable received, index:" << index << ", attr->size:" << attr->size << endl;
        signal_->attrQueue_.push(*attr);
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
    signal_->outCond_.notify_all();
}
} // namespace MediaAVCodec
} // namespace OHOS

AudioDecoderDemo::AudioDecoderDemo()
{
    signal_ = new ADecSignal();
    innersignal_ = make_shared<ADecSignal>();
}

AudioDecoderDemo::~AudioDecoderDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

void AudioDecoderDemo::setTimerFlag(int32_t flag)
{
    timerFlag = flag;
}

OH_AVCodec *AudioDecoderDemo::NativeCreateByMime(const char *mime)
{
    return OH_AudioDecoder_CreateByMime(mime);
}

OH_AVCodec *AudioDecoderDemo::NativeCreateByName(const char *name)
{
    return OH_AudioDecoder_CreateByName(name);
}

OH_AVErrCode AudioDecoderDemo::NativeDestroy(OH_AVCodec *codec)
{
    stopThread();
    return OH_AudioDecoder_Destroy(codec);
}

OH_AVErrCode AudioDecoderDemo::NativeSetCallback(OH_AVCodec *codec, OH_AVCodecAsyncCallback callback)
{
    return OH_AudioDecoder_SetCallback(codec, callback, signal_);
}

OH_AVErrCode AudioDecoderDemo::NativeConfigure(OH_AVCodec *codec, OH_AVFormat *format)
{
    return OH_AudioDecoder_Configure(codec, format);
}

OH_AVErrCode AudioDecoderDemo::NativePrepare(OH_AVCodec *codec)
{
    return OH_AudioDecoder_Prepare(codec);
}

OH_AVErrCode AudioDecoderDemo::NativeStart(OH_AVCodec *codec)
{
    if (!isRunning_.load()) {
        cout << "Native Start!!!" << endl;
        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioDecoderDemo::updateInputData, this);
        outputLoop_ = make_unique<thread>(&AudioDecoderDemo::updateOutputData, this);
    }
    OH_AVErrCode ret = OH_AudioDecoder_Start(codec);
    sleep(1);
    return ret;
}

OH_AVErrCode AudioDecoderDemo::NativeStop(OH_AVCodec *codec)
{
    stopThread();
    return OH_AudioDecoder_Stop(codec);
}

OH_AVErrCode AudioDecoderDemo::NativeFlush(OH_AVCodec *codec)
{
    stopThread();
    return OH_AudioDecoder_Flush(codec);
}

OH_AVErrCode AudioDecoderDemo::NativeReset(OH_AVCodec *codec)
{
    stopThread();
    return OH_AudioDecoder_Reset(codec);
}

OH_AVFormat *AudioDecoderDemo::NativeGetOutputDescription(OH_AVCodec *codec)
{
    return OH_AudioDecoder_GetOutputDescription(codec);
}

OH_AVErrCode AudioDecoderDemo::NativeSetParameter(OH_AVCodec *codec, OH_AVFormat *format)
{
    return OH_AudioDecoder_SetParameter(codec, format);
}

OH_AVErrCode AudioDecoderDemo::NativePushInputData(OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr)
{
    return OH_AudioDecoder_PushInputData(codec, index, attr);
}

OH_AVErrCode AudioDecoderDemo::NativeFreeOutputData(OH_AVCodec *codec, uint32_t index)
{
    return OH_AudioDecoder_FreeOutputData(codec, index);
}

OH_AVErrCode AudioDecoderDemo::NativeIsValid(OH_AVCodec *codec, bool *isVaild)
{
    return OH_AudioDecoder_IsValid(codec, isVaild);
}

void AudioDecoderDemo::stopThread()
{
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
        inputLoop_ = nullptr;
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_ = nullptr;
    }

    while (!signal_->inQueue_.empty())
        signal_->inQueue_.pop();
    while (!signal_->outQueue_.empty())
        signal_->outQueue_.pop();
    while (!signal_->inBufferQueue_.empty())
        signal_->inBufferQueue_.pop();
    while (!signal_->outBufferQueue_.empty())
        signal_->outBufferQueue_.pop();
    while (!signal_->attrQueue_.empty())
        signal_->attrQueue_.pop();

    while (!inIndexQueue_.empty())
        inIndexQueue_.pop();
    while (!inBufQueue_.empty())
        inBufQueue_.pop();
    while (!outIndexQueue_.empty())
        outIndexQueue_.pop();
}

void AudioDecoderDemo::updateInputData()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "input wait to stop, exit" << endl;
            break;
        }

        cout << "inQueue_ size is " << signal_->inQueue_.size() << endl;
        cout << "inputBuf size is " << signal_->inBufferQueue_.size() << endl;
        uint32_t inputIndex = signal_->inQueue_.front();
        inIndexQueue_.push(inputIndex);
        signal_->inQueue_.pop();

        uint8_t *inputBuf = OH_AVMemory_GetAddr(signal_->inBufferQueue_.front());
        inBufQueue_.push(inputBuf);
        signal_->inBufferQueue_.pop();
        cout << "input index is " << inputIndex << endl;
    }
}

void AudioDecoderDemo::updateOutputData()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "output wait to stop, exit" << endl;
            break;
        }
        cout << "outQueue_ size is " << signal_->outQueue_.size() << ", outBufferQueue_ size is "
             << signal_->outBufferQueue_.size() << ", attrQueue_ size is " << signal_->attrQueue_.size() << endl;
        uint32_t outputIndex = signal_->outQueue_.front();
        outIndexQueue_.push(outputIndex);
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        cout << "output index is " << outputIndex << endl;
    }
}

void AudioDecoderDemo::InnerUpdateInputData()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(innersignal_->inMutex_);
        innersignal_->inCond_.wait(lock,
                                   [this]() { return (innersignal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "input wait to stop, exit" << endl;
            break;
        }

        uint32_t inputIndex = innersignal_->inQueue_.front();
        inIndexQueue_.push(inputIndex);
        innersignal_->inQueue_.pop();
        cout << "input index is " << inputIndex << endl;
    }
}

void AudioDecoderDemo::InnerUpdateOutputData()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.wait(lock,
                                    [this]() { return (innersignal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "output wait to stop, exit" << endl;
            break;
        }

        uint32_t outputIndex = innersignal_->outQueue_.front();
        outIndexQueue_.push(outputIndex);
        innersignal_->outQueue_.pop();
        innersignal_->infoQueue_.pop();
        innersignal_->flagQueue_.pop();
        cout << "output index is " << outputIndex << endl;
    }
}

uint32_t AudioDecoderDemo::NativeGetInputIndex()
{
    while (inIndexQueue_.empty())
        sleep(1);
    uint32_t inputIndex = inIndexQueue_.front();
    inIndexQueue_.pop();
    return inputIndex;
}

uint8_t *AudioDecoderDemo::NativeGetInputBuf()
{
    while (inBufQueue_.empty())
        sleep(1);
    uint8_t *inputBuf = inBufQueue_.front();
    inBufQueue_.pop();
    return inputBuf;
}

uint32_t AudioDecoderDemo::NativeGetOutputIndex()
{
    if (outIndexQueue_.empty()) {
        return ERROR_INDEX;
    }
    uint32_t outputIndex = outIndexQueue_.front();
    outIndexQueue_.pop();
    return outputIndex;
}

void AudioDecoderDemo::HandleEOS(const uint32_t &index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;

    gettimeofday(&inputStart, NULL);
    av_packet_unref(&pkt);
    gettimeofday(&inputEnd, NULL);
    otherTime += (inputEnd.tv_sec - inputStart.tv_sec) + (inputEnd.tv_usec - inputStart.tv_usec) / DEFAULT_TIME_NUM;

    if (timerFlag == TIMER_INPUT) {
        gettimeofday(&start, NULL);
    }
    OH_AudioDecoder_PushInputData(audioDec_, index, info);
    if (timerFlag == TIMER_INPUT) {
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;
        runTimes++;
    }
}

int32_t AudioDecoderDemo::NativePushInput(uint32_t index, OH_AVMemory *buffer)
{
    OH_AVCodecBufferAttr info;
    info.size = pkt.size;
    info.offset = 0;
    info.pts = pkt.pts;
    memcpy_s(OH_AVMemory_GetAddr(buffer), pkt.size, pkt.data, pkt.size);

    int32_t ret = AV_ERR_OK;
    if (isFirstFrame_) {
        info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        if (timerFlag == TIMER_INPUT) {
            gettimeofday(&start, NULL);
        }
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
        if (timerFlag == TIMER_INPUT) {
            gettimeofday(&end, NULL);
            totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;
            runTimes++;
        }
        isFirstFrame_ = false;
    } else {
        info.flags = AVCODEC_BUFFER_FLAGS_NONE;
        if (timerFlag == TIMER_INPUT) {
            gettimeofday(&start, NULL);
        }
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
        if (timerFlag == TIMER_INPUT) {
            gettimeofday(&end, NULL);
            totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;
            runTimes++;
        }
    }
    return ret;
}

void AudioDecoderDemo::NativeInputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        cout << "input wait !!!" << endl;
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();

        gettimeofday(&inputStart, NULL);
        int32_t ret = av_read_frame(fmpt_ctx, &pkt);
        gettimeofday(&inputEnd, NULL);
        otherTime += (inputEnd.tv_sec - inputStart.tv_sec) + (inputEnd.tv_usec - inputStart.tv_usec) / DEFAULT_TIME_NUM;

        if (ret < 0) {
            HandleEOS(index);
            signal_->inBufferQueue_.pop();
            signal_->inQueue_.pop();
            std::cout << "end buffer\n";
            break;
        }
        std::cout << "start read frame: size:" << pkt.size << ",pts:" << pkt.pts << "\n";

        ret = NativePushInput(index, buffer);

        av_packet_unref(&pkt);
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();

        if (ret != AV_ERR_OK) {
            cout << "Fatal error, exit! ret = " << ret << endl;
            break;
        }
    }
}

void AudioDecoderDemo::NativeGetDescription()
{
    if (isGetOutputDescription && curFormat == nullptr) {
        cout << "before GetOutputDescription" << endl;
        curFormat = OH_AudioDecoder_GetOutputDescription(audioDec_);
    }
    if (timerFlag == TIMER_GETOUTPUTDESCRIPTION) {
        gettimeofday(&start, NULL);
        curFormat = OH_AudioDecoder_GetOutputDescription(audioDec_);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;
        runTimes++;
    }
}

void AudioDecoderDemo::NativeWriteOutput(std::ofstream &pcmFile, uint32_t index, OH_AVCodecBufferAttr attr,
                                         OH_AVMemory *data)
{
    if (data != nullptr) {
        cout << "OutputFunc write file,buffer index" << index << ", data size = :" << attr.size << endl;

        gettimeofday(&outputStart, NULL);
        pcmFile.write(reinterpret_cast<char *>(OH_AVMemory_GetAddr(data)), attr.size);
        gettimeofday(&outputEnd, NULL);
        totalTime +=
            (outputEnd.tv_sec - outputStart.tv_sec) + (outputEnd.tv_usec - outputStart.tv_usec) / DEFAULT_TIME_NUM;
        runTimes++;
    }

    if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
        cout << "decode eos" << endl;
        isRunning_.store(false);
    }
}

void AudioDecoderDemo::NativeOutputFunc()
{
    std::ofstream pcmFile;
    pcmFile.open(outputFilePath, std::ios::out | std::ios::binary);
    if (!pcmFile.is_open()) {
        std::cout << "open " << outputFilePath << " failed!" << std::endl;
    }

    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        cout << "output wait !!!" << endl;
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *data = signal_->outBufferQueue_.front();

        NativeWriteOutput(pcmFile, index, attr, data);

        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();

        if (timerFlag == TIMER_FREEOUTPUT) {
            gettimeofday(&start, NULL);
        }
        OH_AVErrCode ret = OH_AudioDecoder_FreeOutputData(audioDec_, index);
        if (timerFlag == TIMER_FREEOUTPUT) {
            gettimeofday(&end, NULL);
            totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;
            runTimes++;
        }
        if (ret != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
        NativeGetDescription();
    }
    pcmFile.close();
}

void AudioDecoderDemo::NativeGetVorbisConf(OH_AVFormat *format)
{
    int32_t ret = 0;
    int audio_stream_index = -1;
    for (uint32_t i = 0; i < fmpt_ctx->nb_streams; i++) {
        if (fmpt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index == -1) {
        cout << "Error: Cannot find audio stream" << endl;
        exit(1);
    }
    AVCodecParameters *codec_params = fmpt_ctx->streams[audio_stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (codec == NULL) {
        cout << "Error: Cannot find decoder for codec " << codec_params->codec_id << endl;
        exit(1);
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        cout << "Error: Cannot allocate codec context" << endl;
        exit(1);
    }
    ret = avcodec_parameters_to_context(codec_ctx, codec_params);
    if (ret < 0) {
        cout << "Error: Cannot set codec context parameters" << endl;
        exit(1);
    }
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        cout << "Error: Cannot open codec" << endl;
        exit(1);
    }
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG, (uint8_t *)(codec_ctx->extradata), codec_ctx->extradata_size);
}

void AudioDecoderDemo::NativeCreateToStart(const char *name, OH_AVFormat *format)
{
    OH_AVErrCode result;
    int32_t ret = 0;
    audioDec_ = OH_AudioDecoder_CreateByName(name);
    if (audioDec_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    result = OH_AudioDecoder_SetCallback(audioDec_, cb_, signal_);
    cout << "SetCallback ret is: " << result << endl;

    if (strcmp(name, "OH.Media.Codec.Decoder.Audio.Vorbis") == 0) {
        NativeGetVorbisConf(format);
    }

    result = OH_AudioDecoder_Configure(audioDec_, format);
    cout << "Configure ret is: " << result << endl;
    if (result < 0) {
        cout << "Configure fail! ret is " << result << endl;
        return;
    }

    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    result = OH_AudioDecoder_Prepare(audioDec_);
    cout << "Prepare ret is: " << result << endl;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeOutputFunc, this);
    result = OH_AudioDecoder_Start(audioDec_);
    cout << "Start ret is: " << result << endl;
}

void AudioDecoderDemo::NativeStopDec()
{
    OH_AVErrCode result;

    result = OH_AudioDecoder_Stop(audioDec_);
    cout << "Stop ret is: " << result << endl;

    result = OH_AudioDecoder_Destroy(audioDec_);
    cout << "Destroy ret is: " << result << endl;

    stopThread();
}

void AudioDecoderDemo::NativeCloseFFmpeg()
{
    av_frame_free(&frame);
    if (codec_ctx != nullptr) {
        avcodec_free_context(&codec_ctx);
    }
    avformat_close_input(&fmpt_ctx);
}

void AudioDecoderDemo::NativeFFmpegConf(const char *name, OH_AVFormat *format)
{
    int ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }

    if (strcmp(name, "OH.Media.Codec.Decoder.Audio.Vorbis") == 0) {
        NativeGetVorbisConf(format);
    }

    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
}

void AudioDecoderDemo::NativeStopAndClear()
{
    NativeStopDec();
    NativeCloseFFmpeg();
}

void AudioDecoderDemo::NativeRunCase(std::string inputFile, std::string outputFile, const char *name,
                                     OH_AVFormat *format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();

    if (timerFlag != 0) {
        cout << "total time is " << totalTime << ", run times is " << runTimes << endl;
    }
}

void AudioDecoderDemo::NativeRunCaseWithoutCreate(OH_AVCodec *handle, std::string inputFile, std::string outputFile,
                                                  OH_AVFormat *format, const char *name, bool needConfig)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    audioDec_ = handle;
    OH_AVErrCode result;

    int32_t ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
            << "\n";
        exit(1);
    }

    if (needConfig) {
        result = OH_AudioDecoder_Configure(audioDec_, format);
        cout << "Configure ret is: " << result << endl;
        if (result < 0) {
            cout << "Configure fail! ret is " << result << endl;
            return;
        }

        result = OH_AudioDecoder_Prepare(audioDec_);
        cout << "Prepare ret is: " << result << endl;
    }

    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeOutputFunc, this);
    result = OH_AudioDecoder_Start(audioDec_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    stopThread();

    NativeCloseFFmpeg();
}

void AudioDecoderDemo::NativeRunCasePerformance(std::string inputFile, std::string outputFile, const char *name,
                                                OH_AVFormat *format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    gettimeofday(&startTime, NULL);
    audioDec_ = OH_AudioDecoder_CreateByName(name);
    gettimeofday(&start, NULL);
    if (audioDec_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    OH_AVErrCode result;
    NativeFFmpegConf(name, format);

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    gettimeofday(&end, NULL);
    otherTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;

    result = OH_AudioDecoder_SetCallback(audioDec_, cb_, signal_);
    cout << "SetCallback ret is: " << result << endl;

    result = OH_AudioDecoder_Configure(audioDec_, format);
    cout << "Configure ret is: " << result << endl;
    if (result < 0) {
        cout << "Configure fail! ret is " << result << endl;
        return;
    }

    result = OH_AudioDecoder_Prepare(audioDec_);
    cout << "Prepare ret is: " << result << endl;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeOutputFunc, this);
    result = OH_AudioDecoder_Start(audioDec_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopDec();
    gettimeofday(&endTime, NULL);
    totalTime = (endTime.tv_sec - start.tv_sec) + (startTime.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM - otherTime;

    NativeCloseFFmpeg();
    cout << "cur decoder is " << name << ", total time is " << totalTime << endl;
}

void AudioDecoderDemo::NativeRunCaseFlush(std::string inputFile, std::string outputFileFirst,
                                          std::string outputFileSecond, const char *name, OH_AVFormat *format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    OH_AVErrCode result;
    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    // Stop
    stopThread();
    NativeCloseFFmpeg();

    // Flush
    result = OH_AudioDecoder_Flush(audioDec_);
    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;

    NativeFFmpegConf(name, format);

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeOutputFunc, this);
    result = OH_AudioDecoder_Start(audioDec_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();
}

void AudioDecoderDemo::NativeRunCaseReset(std::string inputFile, std::string outputFileFirst,
                                          std::string outputFileSecond, const char *name, OH_AVFormat *format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    OH_AVErrCode result;
    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    // Stop
    stopThread();
    NativeCloseFFmpeg();

    // Reset
    result = OH_AudioDecoder_Reset(audioDec_);

    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;

    NativeFFmpegConf(name, format);

    result = OH_AudioDecoder_Configure(audioDec_, format);
    if (result < 0) {
        cout << "Configure fail! ret is " << result << endl;
        return;
    }

    result = OH_AudioDecoder_Prepare(audioDec_);
    cout << "Prepare ret is: " << result << endl;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeOutputFunc, this);
    result = OH_AudioDecoder_Start(audioDec_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();
}

OH_AVFormat *AudioDecoderDemo::NativeRunCaseGetOutputDescription(std::string inputFile, std::string outputFile,
                                                                 const char *name, OH_AVFormat *format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;
    isGetOutputDescription = true;

    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();
    return curFormat;
}

int32_t AudioDecoderDemo::TestReadDatFile(uint32_t index, OH_AVMemory *buffer)
{
    int64_t size;
    int64_t pts;

    inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_.eof() || inputFile_.gcount() == 0) {
        OH_AVCodecBufferAttr info;
        info.size = 0;
        info.offset = 0;
        info.pts = 0;
        info.flags = AVCODEC_BUFFER_FLAGS_EOS;

        OH_AudioDecoder_PushInputData(audioDec_, index, info);
        signal_->inBufferQueue_.pop();
        signal_->inQueue_.pop();
        std::cout << "end buffer\n";
        return CODE_ERROR;
    }
    if (inputFile_.gcount() != sizeof(size)) {
        cout << "Fatal: read size fail" << endl;
        return CODE_ERROR;
    }
    inputFile_.read(reinterpret_cast<char *>(&pts), sizeof(pts));
    if (inputFile_.gcount() != sizeof(pts)) {
        cout << "Fatal: read pts fail" << endl;
        return CODE_ERROR;
    }
    inputFile_.read((char *)OH_AVMemory_GetAddr(buffer), size);
    if (inputFile_.gcount() != size) {
        cout << "Fatal: read buffer fail" << endl;
        return CODE_ERROR;
    }

    OH_AVCodecBufferAttr info;
    info.size = size;
    info.offset = 0;
    info.pts = pts;

    int32_t ret = AVCS_ERR_OK;
    if (isFirstFrame_) {
        info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
        isFirstFrame_ = false;
    } else {
        info.flags = AVCODEC_BUFFER_FLAGS_NONE;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
    }
    return ret;
}

void AudioDecoderDemo::TestInputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();

        int32_t ret = TestReadDatFile(index, buffer);

        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;

        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit!!! ret is " << ret << endl;
            break;
        }
    }
    inputFile_.close();
}

void AudioDecoderDemo::TestRunCase(std::string inputFile, std::string outputFile, const char *name, OH_AVFormat *format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    inputFile_.open(inputFilePath, std::ios::binary);

    audioDec_ = OH_AudioDecoder_CreateByName(name);
    if (audioDec_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }
    OH_AVErrCode result;

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    result = OH_AudioDecoder_SetCallback(audioDec_, cb_, signal_);
    cout << "SetCallback ret is: " << result << endl;

    result = OH_AudioDecoder_Configure(audioDec_, format);
    cout << "Configure ret is: " << result << endl;
    if (result < 0) {
        cout << "Configure fail! ret is " << result << endl;
        return;
    }

    result = OH_AudioDecoder_Prepare(audioDec_);
    cout << "Prepare ret is: " << result << endl;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::TestInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::NativeOutputFunc, this);
    result = OH_AudioDecoder_Start(audioDec_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();
}

void AudioDecoderDemo::TestFFmpeg(std::string inputFile)
{
    int32_t ret = 0;
    ret = avformat_open_input(&fmpt_ctx, inputFile.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFile << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }

    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    while (true) {
        ret = av_read_frame(fmpt_ctx, &pkt);
        if (ret < 0) {
            av_packet_unref(&pkt);
            break;
        }
        av_packet_unref(&pkt);
    }

    av_frame_free(&frame);
    if (codec_ctx != nullptr) {
        avcodec_free_context(&codec_ctx);
    }
    avformat_close_input(&fmpt_ctx);
}

// inner

int32_t AudioDecoderDemo::InnerCreateByMime(const std::string &mime)
{
    inneraudioDec_ = AudioDecoderFactory::CreateByMime(mime);
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    std::cout << "InnerCreateByMime" << endl;
    return AVCS_ERR_OK;
}

int32_t AudioDecoderDemo::InnerCreateByName(const std::string &name)
{
    inneraudioDec_ = AudioDecoderFactory::CreateByName(name);
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    std::cout << "InnerCreateByName" << endl;
    return AVCS_ERR_OK;
}

int32_t AudioDecoderDemo::InnerSetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    return inneraudioDec_->SetCallback(callback);
}

int32_t AudioDecoderDemo::InnerConfigure(const Format &format)
{
    cout << "InnerConfigure" << endl;
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioDec_->Configure(format);
}

int32_t AudioDecoderDemo::InnerStart()
{
    if (!isRunning_.load()) {
        cout << "InnerStart" << endl;
        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerUpdateInputData, this);
        outputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerUpdateOutputData, this);
    }
    int32_t ret = inneraudioDec_->Start();
    sleep(1);
    return ret;
}

int32_t AudioDecoderDemo::InnerPrepare()
{
    cout << "InnerPrepare" << endl;
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioDec_->Prepare();
}

int32_t AudioDecoderDemo::InnerStop()
{
    cout << "InnerStop" << endl;
    InnerStopThread();
    return inneraudioDec_->Stop();
}

int32_t AudioDecoderDemo::InnerFlush()
{
    cout << "InnerFlush" << endl;
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    int32_t ret = inneraudioDec_->Flush();

    while (!innersignal_->inQueue_.empty())
        innersignal_->inQueue_.pop();
    while (!innersignal_->inInnerBufQueue_.empty())
        innersignal_->inInnerBufQueue_.pop();
    while (!innersignal_->outInnerBufQueue_.empty())
        innersignal_->outInnerBufQueue_.pop();
    while (!innersignal_->outQueue_.empty())
        innersignal_->outQueue_.pop();
    while (!innersignal_->infoQueue_.empty())
        innersignal_->infoQueue_.pop();
    while (!innersignal_->flagQueue_.empty())
        innersignal_->flagQueue_.pop();

    while (!inIndexQueue_.empty())
        inIndexQueue_.pop();
    while (!inBufQueue_.empty())
        inBufQueue_.pop();
    while (!outIndexQueue_.empty())
        outIndexQueue_.pop();

    return ret;
}

int32_t AudioDecoderDemo::InnerReset()
{
    InnerStopThread();
    cout << "InnerReset" << endl;
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioDec_->Reset();
}

int32_t AudioDecoderDemo::InnerRelease()
{
    cout << "InnerRelease" << endl;
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioDec_->Release();
}

int32_t AudioDecoderDemo::InnerSetParameter(const Format &format)
{
    cout << "InnerSetParameter" << endl;
    if (inneraudioDec_ == nullptr) {
        std::cout << "InnerDecoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioDec_->SetParameter(format);
}

int32_t AudioDecoderDemo::InnerDestroy()
{
    int32_t ret = AVCS_ERR_INVALID_OPERATION;
    InnerStopThread();
    if (inneraudioDec_ != nullptr) {
        ret = inneraudioDec_->Release();
    }
    inneraudioDec_ = nullptr;
    return ret;
}

int32_t AudioDecoderDemo::InnerQueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "InnerQueueInputBuffer" << endl;
    return inneraudioDec_->QueueInputBuffer(index, info, flag);
}

int32_t AudioDecoderDemo::InnerGetOutputFormat(Format &format)
{
    cout << "InnerGetOutputFormat" << endl;
    return inneraudioDec_->GetOutputFormat(format);
}

int32_t AudioDecoderDemo::InnerReleaseOutputBuffer(uint32_t index)
{
    cout << "InnerReleaseOutputBuffer" << endl;
    return inneraudioDec_->ReleaseOutputBuffer(index);
}

void AudioDecoderDemo::InnerStopThread()
{
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->inMutex_);
        innersignal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
        inputLoop_ = nullptr;
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_ = nullptr;
    }

    while (!innersignal_->inQueue_.empty())
        innersignal_->inQueue_.pop();
    while (!innersignal_->inInnerBufQueue_.empty())
        innersignal_->inInnerBufQueue_.pop();
    while (!innersignal_->outInnerBufQueue_.empty())
        innersignal_->outInnerBufQueue_.pop();
    while (!innersignal_->outQueue_.empty())
        innersignal_->outQueue_.pop();
    while (!innersignal_->infoQueue_.empty())
        innersignal_->infoQueue_.pop();
    while (!innersignal_->flagQueue_.empty())
        innersignal_->flagQueue_.pop();

    while (!inIndexQueue_.empty())
        inIndexQueue_.pop();
    while (!inBufQueue_.empty())
        inBufQueue_.pop();
    while (!outIndexQueue_.empty())
        outIndexQueue_.pop();
}

uint32_t AudioDecoderDemo::InnerInputFuncRead(uint32_t index)
{
    int32_t ret = av_read_frame(fmpt_ctx, &pkt);
    if (ret < 0) {
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        info.size = 0;
        info.offset = 0;
        info.presentationTimeUs = 0;
        flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS;
        av_packet_unref(&pkt);
        inneraudioDec_->QueueInputBuffer(index, info, flag);
        innersignal_->inQueue_.pop();
        std::cout << "end buffer\n";
        return 1;
    }
    std::cout << "start read frame: size:" << pkt.size << ", pts:" << pkt.pts << ", index:" << index << "\n";
    return 0;
}

void AudioDecoderDemo::InnerInputFunc()
{
    while (isRunning_.load()) {
        std::unique_lock<std::mutex> lock(innersignal_->inMutex_);
        cout << "input wait !!!" << endl;
        innersignal_->inCond_.wait(lock,
                                   [this]() { return (innersignal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = innersignal_->inQueue_.front();
        auto buffer = innersignal_->inInnerBufQueue_.front();
        if (buffer == nullptr) {
            cout << "buffer is nullptr" << endl;
            isRunning_ = false;
            break;
        }

        uint32_t ret = InnerInputFuncRead(index);
        if (ret != 0)
            break;

        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;

        info.size = pkt.size;
        info.offset = 0;
        info.presentationTimeUs = pkt.pts;
        std::cout << "start read frame: size:" << &pkt.data << "\n";
        memcpy_s(buffer->GetBase(), pkt.size, pkt.data, pkt.size);

        ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
            ret = inneraudioDec_->QueueInputBuffer(index, info, flag);
            isFirstFrame_ = false;
        } else {
            flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
            ret = inneraudioDec_->QueueInputBuffer(index, info, flag);
        }

        innersignal_->inQueue_.pop();
        innersignal_->inInnerBufQueue_.pop();
        std::cout << "QueueInputBuffer " << index << "\n";
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            isRunning_ = false;
            break;
        }
    }
}

void AudioDecoderDemo::InnerOutputFunc()
{
    std::ofstream pcmFile;
    pcmFile.open(outputFilePath, std::ios::out | std::ios::binary);
    if (!pcmFile.is_open()) {
        std::cout << "open " << outputFilePath << " failed!" << std::endl;
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        cout << "output wait !!!" << endl;
        innersignal_->outCond_.wait(lock,
                                    [this]() { return (innersignal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }
        uint32_t index = innersignal_->outQueue_.front();
        auto buffer = innersignal_->outInnerBufQueue_.front();
        auto attr = innersignal_->infoQueue_.front();
        auto flag = innersignal_->flagQueue_.front();
        std::cout << "GetOutputBuffer : " << buffer << "\n";
        if (buffer != nullptr) {
            cout << "OutputFunc write file, buffer index = " << index << ", data size = " << attr.size << endl;
            pcmFile.write(reinterpret_cast<char *>(buffer->GetBase()), attr.size);
        }
        if (flag == AVCODEC_BUFFER_FLAG_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
        }
        innersignal_->outQueue_.pop();
        innersignal_->outInnerBufQueue_.pop();
        innersignal_->infoQueue_.pop();
        innersignal_->flagQueue_.pop();
        if (inneraudioDec_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }
    }
    pcmFile.close();
}

void AudioDecoderDemo::InnerRunCaseOHVorbis(const std::string &name, Format &format)
{
    int ret;
    if (name == "OH.Media.Codec.Decoder.Audio.Vorbis") {
        cout << "vorbis" << endl;
        int audio_stream_index = -1;
        for (uint32_t i = 0; i < fmpt_ctx->nb_streams; i++) {
            if (fmpt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = i;
                break;
            }
        }
        if (audio_stream_index == -1) {
            cout << "Error: Cannot find audio stream" << endl;
            exit(1);
        }

        cout << "audio_stream_index " << audio_stream_index << endl;
        AVCodecParameters *codec_params = fmpt_ctx->streams[audio_stream_index]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
        if (codec == NULL) {
            cout << "Error: Cannot find decoder for codec " << codec_params->codec_id << endl;
            exit(1);
        }

        codec_ctx = avcodec_alloc_context3(codec);
        if (codec_ctx == NULL) {
            cout << "Error: Cannot allocate codec context" << endl;
            exit(1);
        }
        ret = avcodec_parameters_to_context(codec_ctx, codec_params);
        if (ret < 0) {
            cout << "Error: Cannot set codec context parameters" << endl;
            exit(1);
        }
        ret = avcodec_open2(codec_ctx, codec, NULL);
        if (ret < 0) {
            cout << "Error: Cannot open codec" << endl;
            exit(1);
        }

        format.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)(codec_ctx->extradata),
                         codec_ctx->extradata_size);
    }
    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
}

int AudioDecoderDemo::InnerRunCasePre()
{
    int result;

    result = InnerPrepare();
    cout << "InnerPrepare ret is: " << result << endl;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerOutputFunc, this);
    result = inneraudioDec_->Start();
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    // Stop
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->inMutex_);
        innersignal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    result = inneraudioDec_->Stop();
    cout << "Stop ret is: " << result << endl;

    result = InnerDestroy();
    cout << "Destroy ret is: " << result << endl;
    InnerStopThread();
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmpt_ctx);

    return 0;
}

void AudioDecoderDemo::InnerRunCase(std::string inputFile, std::string outputFile, const std::string &name,
                                    Format &format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    InnerCreateByName(name);
    if (inneraudioDec_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    int32_t ret = 0;
    int32_t result;
    ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }
    InnerRunCaseOHVorbis(name, format);

    innersignal_ = getSignal();
    cout << "innersignal_: " << innersignal_ << endl;
    innercb_ = make_unique<InnerADecDemoCallback>(innersignal_);
    result = InnerSetCallback(innercb_);
    cout << "SetCallback ret is: " << result << endl;

    result = InnerConfigure(format);
    cout << "Configure ret is: " << result << endl;
    if (result != 0) {
        cout << "Configure fail!!" << endl;
        return;
    }
    ret = InnerRunCasePre();
    if (ret != 0)
        return;
}

void AudioDecoderDemo::InnerRunCaseFlushAlloc(Format &format)
{
    int result;
    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    innersignal_ = getSignal();
    cout << "innersignal_: " << innersignal_ << endl;
    innercb_ = make_unique<InnerADecDemoCallback>(innersignal_);
    result = InnerSetCallback(innercb_);
    cout << "SetCallback ret is: " << result << endl;

    result = InnerConfigure(format);
    cout << "Configure ret is: " << result << endl;
    if (result != 0) {
        cout << "Configure fail!!" << endl;
    }
}

void AudioDecoderDemo::InnerRunCaseFlushOHVorbis(const std::string &name, Format &format)
{
    int ret;
    if (name == "OH.Media.Codec.Decoder.Audio.Vorbis") {
        int audio_stream_index = -1;
        for (uint32_t i = 0; i < fmpt_ctx->nb_streams; i++) {
            if (fmpt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = i;
                break;
            }
        }
        if (audio_stream_index == -1) {
            cout << "Error: Cannot find audio stream" << endl;
            exit(1);
        }

        AVCodecParameters *codec_params = fmpt_ctx->streams[audio_stream_index]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
        if (codec == NULL) {
            cout << "Error: Cannot find decoder for codec " << codec_params->codec_id << endl;
            exit(1);
        }

        codec_ctx = avcodec_alloc_context3(codec);
        if (codec_ctx == NULL) {
            cout << "Error: Cannot allocate codec context" << endl;
            exit(1);
        }
        ret = avcodec_parameters_to_context(codec_ctx, codec_params);
        if (ret < 0) {
            cout << "Error: Cannot set codec context parameters" << endl;
            exit(1);
        }
        ret = avcodec_open2(codec_ctx, codec, NULL);
        if (ret < 0) {
            cout << "Error: Cannot open codec" << endl;
            exit(1);
        }

        format.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)(codec_ctx->extradata),
                         codec_ctx->extradata_size);
    }
    InnerRunCaseFlushAlloc(format);
}

int AudioDecoderDemo::InnerRunCaseFlushPre()
{
    int result = InnerPrepare();
    cout << "InnerPrepare ret is: " << result << endl;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerOutputFunc, this);
    result = inneraudioDec_->Start();
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->inMutex_);
        innersignal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    InnerStopThread();
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmpt_ctx);
    return 0;
}

void AudioDecoderDemo::InnerRunCaseFlushPost()
{
    int result;
    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerOutputFunc, this);
    result = inneraudioDec_->Start();
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->inMutex_);

        innersignal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    result = inneraudioDec_->Stop();
    cout << "Stop ret is: " << result << endl;

    result = InnerDestroy();
    cout << "Destroy ret is: " << result << endl;

    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmpt_ctx);
}

void AudioDecoderDemo::InnerRunCaseFlush(std::string inputFile, std::string outputFileFirst,
                                         std::string outputFileSecond, const std::string &name, Format &format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    InnerCreateByName(name);
    if (inneraudioDec_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    int32_t ret = 0;
    ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }
    InnerRunCaseFlushOHVorbis(name, format);

    ret = InnerRunCaseFlushPre();
    if (ret != 0)
        return;

    InnerFlush();
    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;
    ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }

    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }
    InnerRunCaseFlushPost();
}

void AudioDecoderDemo::InnerRunCaseResetAlloc(Format &format)
{
    int result;
    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    innersignal_ = getSignal();
    cout << "innersignal_: " << innersignal_ << endl;
    innercb_ = make_unique<InnerADecDemoCallback>(innersignal_);
    result = InnerSetCallback(innercb_);
    cout << "SetCallback ret is: " << result << endl;

    result = InnerConfigure(format);
    cout << "Configure ret is: " << result << endl;
    if (result != 0) {
        cout << "Configure fail!!" << endl;
        return;
    }
}

void AudioDecoderDemo::InnerRunCaseResetOHVorbis(const std::string &name, Format &format)
{
    int ret;
    if (name == "OH.Media.Codec.Decoder.Audio.Vorbis") {
        int audio_stream_index = -1;
        for (uint32_t i = 0; i < fmpt_ctx->nb_streams; i++) {
            if (fmpt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = i;
                break;
            }
        }
        if (audio_stream_index == -1) {
            cout << "Error: Cannot find audio stream" << endl;
            exit(1);
        }
        AVCodecParameters *codec_params = fmpt_ctx->streams[audio_stream_index]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
        if (codec == NULL) {
            cout << "Error: Cannot find decoder for codec " << codec_params->codec_id << endl;
            exit(1);
        }

        codec_ctx = avcodec_alloc_context3(codec);
        if (codec_ctx == NULL) {
            cout << "Error: Cannot allocate codec context" << endl;
            exit(1);
        }
        ret = avcodec_parameters_to_context(codec_ctx, codec_params);
        if (ret < 0) {
            cout << "Error: Cannot set codec context parameters" << endl;
            exit(1);
        }
        ret = avcodec_open2(codec_ctx, codec, NULL);
        if (ret < 0) {
            cout << "Error: Cannot open codec" << endl;
            exit(1);
        }

        format.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)(codec_ctx->extradata),
                         codec_ctx->extradata_size);
    }

    InnerRunCaseResetAlloc(format);
}
int AudioDecoderDemo::InnerRunCaseResetPre()
{
    int result = InnerPrepare();
    cout << "InnerPrepare ret is: " << result << endl;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerOutputFunc, this);
    result = inneraudioDec_->Start();
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->inMutex_);
        innersignal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }

    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmpt_ctx);

    return 0;
}

void AudioDecoderDemo::InnerRunCaseResetInPut()
{
    int ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }

    frame = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
}

void AudioDecoderDemo::InnerRunCaseResetPost()
{
    int result = InnerPrepare();
    cout << "InnerPrepare ret is: " << result << endl;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioDecoderDemo::InnerOutputFunc, this);
    result = inneraudioDec_->Start();
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->inMutex_);
        innersignal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    result = inneraudioDec_->Stop();
    cout << "Stop ret is: " << result << endl;

    result = InnerDestroy();
    cout << "Destroy ret is: " << result << endl;

    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmpt_ctx);
}

void AudioDecoderDemo::InnerRunCaseReset(std::string inputFile, std::string outputFileFirst,
                                         std::string outputFileSecond, const std::string &name, Format &format)
{
    int32_t result;
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    InnerCreateByName(name);
    if (inneraudioDec_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    int32_t ret = 0;
    ret = avformat_open_input(&fmpt_ctx, inputFilePath.c_str(), NULL, NULL);
    if (ret < 0) {
        std::cout << "open " << inputFilePath << " failed!!!" << ret << "\n";
        exit(1);
    }
    if (avformat_find_stream_info(fmpt_ctx, NULL) < 0) {
        std::cout << "get file stream failed"
                  << "\n";
        exit(1);
    }
    InnerRunCaseResetOHVorbis(name, format);

    ret = InnerRunCaseResetPre();
    if (ret != 0)
        return;

    result = InnerReset();
    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;

    InnerRunCaseResetInPut();
    result = InnerConfigure(format);
    cout << "Configure ret is: " << result << endl;
    if (result != 0) {
        cout << "Configure fail!!" << endl;
        return;
    }

    InnerRunCaseResetPost();
}

std::shared_ptr<ADecSignal> AudioDecoderDemo::getSignal()
{
    return innersignal_;
}

InnerADecDemoCallback::InnerADecDemoCallback(shared_ptr<ADecSignal> signal) : innersignal_(signal) {}

void InnerADecDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void InnerADecDemoCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
    cout << "OnOutputFormatChanged received" << endl;
}

void InnerADecDemoCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    if (innersignal_ == nullptr) {
        std::cout << "buffer is null 1" << endl;
    }
    unique_lock<mutex> lock(innersignal_->inMutex_);

    innersignal_->inQueue_.push(index);
    innersignal_->inInnerBufQueue_.push(buffer);
    if (innersignal_ == nullptr) {
        std::cout << "buffer is null 2" << endl;
    }
    innersignal_->inCond_.notify_all();
    if (innersignal_ == nullptr) {
        std::cout << "buffer is null 3" << endl;
    }
}

void InnerADecDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                    std::shared_ptr<AVSharedMemory> buffer)
{
    (void)info;
    (void)flag;
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(innersignal_->outMutex_);
    innersignal_->outQueue_.push(index);
    innersignal_->infoQueue_.push(info);
    innersignal_->flagQueue_.push(flag);
    innersignal_->outInnerBufQueue_.push(buffer);
    innersignal_->outCond_.notify_all();
}