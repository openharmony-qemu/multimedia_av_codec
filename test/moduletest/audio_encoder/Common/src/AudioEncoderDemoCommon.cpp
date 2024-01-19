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

#include "AudioEncoderDemoCommon.h"
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
        void OnError(OH_AVCodec* codec, int32_t errorCode, void* userData)
        {
            (void)codec;
            (void)errorCode;
            (void)userData;
            cout << "Error received, errorCode:" << errorCode << endl;
        }

        void OnOutputFormatChanged(OH_AVCodec* codec, OH_AVFormat* format, void* userData)
        {
            (void)codec;
            (void)format;
            (void)userData;
            cout << "OnOutputFormatChanged received" << endl;
        }

        void OnInputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data, void* userData)
        {
            (void)codec;
            AEncSignal* signal_ = static_cast<AEncSignal*>(userData);
            cout << "OnInputBufferAvailable received, index:" << index << endl;
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inQueue_.push(index);
            signal_->inBufferQueue_.push(data);
            signal_->inCond_.notify_all();
        }

        void OnOutputBufferAvailable(OH_AVCodec* codec, uint32_t index,
            OH_AVMemory* data, OH_AVCodecBufferAttr* attr, void* userData)
        {
            (void)codec;
            AEncSignal* signal_ = static_cast<AEncSignal*>(userData);
            cout << "OnOutputBufferAvailable received, index:" << index << endl;
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outQueue_.push(index);
            signal_->outBufferQueue_.push(data);
            if (attr) {
                cout << "OnOutputBufferAvailable received, index:" << index <<
                    ", attr->size:" << attr->size << endl;
                signal_->attrQueue_.push(*attr);
            } else {
                cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
            }
            signal_->outCond_.notify_all();
        }
    }
}

AudioEncoderDemo::AudioEncoderDemo()
{
    signal_ = new AEncSignal();
    innersignal_ = make_shared<AEncSignal>();
}

AudioEncoderDemo::~AudioEncoderDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

OH_AVCodec* AudioEncoderDemo::NativeCreateByMime(const char* mime)
{
    return OH_AudioEncoder_CreateByMime(mime);
}

OH_AVCodec* AudioEncoderDemo::NativeCreateByName(const char* name)
{
    return OH_AudioEncoder_CreateByName(name);
}

OH_AVErrCode AudioEncoderDemo::NativeDestroy(OH_AVCodec *codec)
{
    stopThread();
    return OH_AudioEncoder_Destroy(codec);
}

OH_AVErrCode AudioEncoderDemo::NativeSetCallback(OH_AVCodec* codec, OH_AVCodecAsyncCallback callback)
{
    return OH_AudioEncoder_SetCallback(codec, callback, signal_);
}

OH_AVErrCode AudioEncoderDemo::NativeConfigure(OH_AVCodec* codec, OH_AVFormat* format)
{
    return OH_AudioEncoder_Configure(codec, format);
}

OH_AVErrCode AudioEncoderDemo::NativePrepare(OH_AVCodec* codec)
{
    return OH_AudioEncoder_Prepare(codec);
}

OH_AVErrCode AudioEncoderDemo::NativeStart(OH_AVCodec* codec)
{
    if (!isRunning_.load()) {
        cout << "Native Start!!!" << endl;
        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioEncoderDemo::updateInputData, this);
        outputLoop_ = make_unique<thread>(&AudioEncoderDemo::updateOutputData, this);
    }
    OH_AVErrCode ret = OH_AudioEncoder_Start(codec);
    sleep(1);
    return ret;
}

OH_AVErrCode AudioEncoderDemo::NativeStop(OH_AVCodec* codec)
{
    stopThread();
    return OH_AudioEncoder_Stop(codec);
}

OH_AVErrCode AudioEncoderDemo::NativeFlush(OH_AVCodec* codec)
{
    stopThread();
    return OH_AudioEncoder_Flush(codec);
}

OH_AVErrCode AudioEncoderDemo::NativeReset(OH_AVCodec* codec)
{
    stopThread();
    return OH_AudioEncoder_Reset(codec);
}

OH_AVFormat* AudioEncoderDemo::NativeGetOutputDescription(OH_AVCodec* codec)
{
    return OH_AudioEncoder_GetOutputDescription(codec);
}

OH_AVErrCode AudioEncoderDemo::NativeSetParameter(OH_AVCodec* codec, OH_AVFormat* format)
{
    return OH_AudioEncoder_SetParameter(codec, format);
}

OH_AVErrCode AudioEncoderDemo::NativePushInputData(OH_AVCodec* codec, uint32_t index,
    OH_AVCodecBufferAttr attr)
{
    return OH_AudioEncoder_PushInputData(codec, index, attr);
}

OH_AVErrCode AudioEncoderDemo::NativeFreeOutputData(OH_AVCodec* codec, uint32_t index)
{
    return OH_AudioEncoder_FreeOutputData(codec, index);
}

OH_AVErrCode AudioEncoderDemo::NativeIsValid(OH_AVCodec* codec, bool* isVaild)
{
    return OH_AudioEncoder_IsValid(codec, isVaild);
}

void AudioEncoderDemo::stopThread()
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

    while (!signal_->inQueue_.empty()) signal_->inQueue_.pop();
    while (!signal_->outQueue_.empty()) signal_->outQueue_.pop();
    while (!signal_->inBufferQueue_.empty()) signal_->inBufferQueue_.pop();
    while (!signal_->outBufferQueue_.empty()) signal_->outBufferQueue_.pop();
    while (!signal_->attrQueue_.empty()) signal_->attrQueue_.pop();

    while (!inIndexQueue_.empty()) inIndexQueue_.pop();
    while (!inBufQueue_.empty()) inBufQueue_.pop();
    while (!outIndexQueue_.empty()) outIndexQueue_.pop();
}


void AudioEncoderDemo::updateInputData()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            return (signal_->inQueue_.size() > 0 || !isRunning_.load());
            });

        if (!isRunning_.load()) {
            cout << "input wait to stop, exit" << endl;
            break;
        }

        cout << "inQueue_ size is " << signal_->inQueue_.size() <<
            ", inputBuf size is " << signal_->inBufferQueue_.size() << endl;
        uint32_t inputIndex = signal_->inQueue_.front();
        inIndexQueue_.push(inputIndex);
        signal_->inQueue_.pop();

        uint8_t* inputBuf = OH_AVMemory_GetAddr(signal_->inBufferQueue_.front());
        inBufQueue_.push(inputBuf);
        signal_->inBufferQueue_.pop();
        cout << "input index is " << inputIndex << endl;
    }
}

void AudioEncoderDemo::updateOutputData()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() {
            return (signal_->outQueue_.size() > 0 || !isRunning_.load());
            });

        if (!isRunning_.load()) {
            cout << "output wait to stop, exit" << endl;
            break;
        }
        cout << "outQueue_ size is " << signal_->outQueue_.size() << ", outBufferQueue_ size is " <<
            signal_->outBufferQueue_.size() << ", attrQueue_ size is " << signal_->attrQueue_.size() << endl;
        uint32_t outputIndex = signal_->outQueue_.front();
        outIndexQueue_.push(outputIndex);
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        cout << "output index is " << outputIndex << endl;
    }
}

uint32_t AudioEncoderDemo::NativeGetInputIndex()
{
    while (inIndexQueue_.empty()) sleep(1);
    uint32_t inputIndex = inIndexQueue_.front();
    inIndexQueue_.pop();
    return inputIndex;
}

uint8_t* AudioEncoderDemo::NativeGetInputBuf()
{
    while (inBufQueue_.empty()) sleep(1);
    uint8_t* inputBuf = inBufQueue_.front();
    inBufQueue_.pop();
    return inputBuf;
}

uint32_t AudioEncoderDemo::NativeGetOutputIndex()
{
    if (outIndexQueue_.empty()) {
        return ERROR_INDEX;
    }
    uint32_t outputIndex = outIndexQueue_.front();
    outIndexQueue_.pop();
    return outputIndex;
}

void AudioEncoderDemo::HandleEOS(const uint32_t& index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AudioEncoder_PushInputData(audioEnc_, index, info);
    std::cout << "end buffer\n";
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void AudioEncoderDemo::NativePushInput(uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = inputBufSize;
    info.offset = 0;

    if (isFirstFrame_) {
        info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        if (timerFlag == TIMER_INPUT) {
            gettimeofday(&start, NULL);
        }
        OH_AudioEncoder_PushInputData(audioEnc_, index, info);
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
        OH_AudioEncoder_PushInputData(audioEnc_, index, info);
        if (timerFlag == TIMER_INPUT) {
            gettimeofday(&end, NULL);
            totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;
            runTimes++;
        }
    }
}

void AudioEncoderDemo::NativeInputFunc()
{
    inputFile_.open(inputFilePath, std::ios::binary);
    while (isRunning_.load()) {
        int32_t ret = AVCS_ERR_OK;
        unique_lock<mutex> lock(signal_->inMutex_);
        cout << "input wait !!!" << endl;
        signal_->inCond_.wait(lock, [this]() {
            return (signal_->inQueue_.size() > 0 || !isRunning_.load());
            });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();

        gettimeofday(&inputStart, NULL);
        if (!inputFile_.eof()) {
            if (OH_AVMemory_GetAddr(buffer) == nullptr) {
                cout << "buffer is nullptr" << endl;
            }
            inputFile_.read((char*)OH_AVMemory_GetAddr(buffer), inputBufSize);
        } else {
            HandleEOS(index);
            break;
        }
        gettimeofday(&inputEnd, NULL);
        otherTime += (inputEnd.tv_sec - inputStart.tv_sec) +
            (inputEnd.tv_usec - inputStart.tv_usec) / DEFAULT_TIME_NUM;

        NativePushInput(index);

        timeStamp_ += FRAME_DURATION_US;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    inputFile_.close();
}

void AudioEncoderDemo::NativeGetDescription()
{
    if (isGetOutputDescription && curFormat == nullptr) {
        cout << "before GetOutputDescription" << endl;
        curFormat = OH_AudioEncoder_GetOutputDescription(audioEnc_);
        if (curFormat == nullptr) {
            cout << "GetOutputDescription error !!!" << endl;
        }
    }
    if (timerFlag == TIMER_GETOUTPUTDESCRIPTION) {
        gettimeofday(&start, NULL);
        curFormat = OH_AudioEncoder_GetOutputDescription(audioEnc_);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / DEFAULT_TIME_NUM;
        runTimes++;
    }
}

void AudioEncoderDemo::NativeWriteOutput(std::ofstream &outputFile, uint32_t index,
    OH_AVCodecBufferAttr attr, OH_AVMemory* data)
{
    gettimeofday(&outputStart, NULL);
    if (data != nullptr) {
        cout << "OutputFunc write file,buffer index" << index << ", data size = " << attr.size << endl;
        outputFile.write(reinterpret_cast<char*>(OH_AVMemory_GetAddr(data)), attr.size);
    }
    gettimeofday(&outputEnd, NULL);
    otherTime += (outputEnd.tv_sec - outputStart.tv_sec) +
        (outputEnd.tv_usec - outputStart.tv_usec) / DEFAULT_TIME_NUM;

    cout << "attr.flags: " << attr.flags << endl;
    if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS || attr.size == 0) {
        cout << "encode eos" << endl;
        isRunning_.store(false);
    }
}


void AudioEncoderDemo::NativeOutputFunc()
{
    std::ofstream outputFile;
    outputFile.open(outputFilePath, std::ios::out | std::ios::binary);
    if (!outputFile.is_open()) {
        std::cout << "open " << outputFilePath << " failed!" << std::endl;
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        cout << "output wait !!!" << endl;
        signal_->outCond_.wait(lock, [this]() {
            return (signal_->outQueue_.size() > 0 || !isRunning_.load());
            });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory* data = signal_->outBufferQueue_.front();

        NativeWriteOutput(outputFile, index, attr, data);

        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();

        if (timerFlag == TIMER_FREEOUTPUT) {
            gettimeofday(&start, NULL);
        }
        OH_AVErrCode ret = OH_AudioEncoder_FreeOutputData(audioEnc_, index);
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
    outputFile.close();
}

void AudioEncoderDemo::setTimerFlag(int32_t flag)
{
    timerFlag = flag;
}

void AudioEncoderDemo::NativeCreateToStart(const char* name, OH_AVFormat* format)
{
    cout << "create name is " << name << endl;
    audioEnc_ = OH_AudioEncoder_CreateByName(name);
    if (audioEnc_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    if (strcmp(name, "OH.Media.Codec.Encoder.Audio.Flac") == 0) {
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, &inputBufSize);
    } else if (strcmp(name, "OH.Media.Codec.Encoder.Audio.AAC") == 0) {
        int channels;
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, &channels);
        inputBufSize = channels * AAC_FRAME_SIZE * AAC_DEFAULT_BYTES_PER_SAMPLE;
    }

    OH_AVErrCode result;

    cb_ = { &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable };
    result = OH_AudioEncoder_SetCallback(audioEnc_, cb_, signal_);
    cout << "SetCallback ret is: " << result << endl;

    result = OH_AudioEncoder_Configure(audioEnc_, format);
    cout << "Configure ret is: " << result << endl;
    if (result < 0) {
        cout << "Configure fail! ret is " << result << endl;
        return;
    }

    result = OH_AudioEncoder_Prepare(audioEnc_);
    cout << "Prepare ret is: " << result << endl;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeOutputFunc, this);
    result = OH_AudioEncoder_Start(audioEnc_);
    cout << "Start ret is: " << result << endl;
}

void AudioEncoderDemo::NativeStopAndClear()
{
    OH_AVErrCode result;
    // Stop
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    result = OH_AudioEncoder_Stop(audioEnc_);
    cout << "Stop ret is: " << result << endl;

    while (!signal_->inQueue_.empty()) signal_->inQueue_.pop();
    while (!signal_->outQueue_.empty()) signal_->outQueue_.pop();
    while (!signal_->inBufferQueue_.empty()) signal_->inBufferQueue_.pop();
    while (!signal_->outBufferQueue_.empty()) signal_->outBufferQueue_.pop();
    while (!signal_->attrQueue_.empty()) signal_->attrQueue_.pop();
}

void AudioEncoderDemo::NativeRunCase(std::string inputFile, std::string outputFile,
    const char* name, OH_AVFormat* format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();
    OH_AVErrCode result = OH_AudioEncoder_Destroy(audioEnc_);
    cout << "Destroy ret is: " << result << endl;

    if (timerFlag != 0) {
        cout << "total time is " << totalTime << ", run times is " << runTimes << endl;
    }
}


void AudioEncoderDemo::NativeRunCaseWithoutCreate(OH_AVCodec* handle, std::string inputFile,
    std::string outputFile, OH_AVFormat* format, const char* name, bool needConfig)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    audioEnc_ = handle;

    if (strcmp(name, "OH.Media.Codec.Encoder.Audio.Flac") == 0) {
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, &inputBufSize);
    } else if (strcmp(name, "OH.Media.Codec.Encoder.Audio.AAC") == 0) {
        int channels;
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, &channels);
        inputBufSize = channels * AAC_FRAME_SIZE * AAC_DEFAULT_BYTES_PER_SAMPLE;
    }

    OH_AVErrCode result;
    if (needConfig) {
        result = OH_AudioEncoder_Configure(audioEnc_, format);
        cout << "Configure ret is: " << result << endl;
        if (result < 0) {
            cout << "Configure fail! ret is " << result << endl;
            return;
        }

        result = OH_AudioEncoder_Prepare(audioEnc_);
        cout << "Prepare ret is: " << result << endl;
    }

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeOutputFunc, this);
    result = OH_AudioEncoder_Start(audioEnc_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    // Stop
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
}


void AudioEncoderDemo::NativeRunCasePerformance(std::string inputFile, std::string outputFile,
    const char* name, OH_AVFormat* format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    OH_AVErrCode result;
    gettimeofday(&startTime, NULL);
    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();

    result = OH_AudioEncoder_Destroy(audioEnc_);
    cout << "Destroy ret is: " << result << endl;

    gettimeofday(&endTime, NULL);
    totalTime = (endTime.tv_sec - startTime.tv_sec) +
        (endTime.tv_usec - startTime.tv_usec) / DEFAULT_TIME_NUM - otherTime;

    cout << "cur decoder is " << name << ", total time is " << totalTime << endl;
}


void AudioEncoderDemo::NativeRunCaseFlush(std::string inputFile, std::string outputFileFirst,
    std::string outputFileSecond, const char* name, OH_AVFormat* format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    OH_AVErrCode result;
    NativeStopAndClear();

    // Flush
    result = OH_AudioEncoder_Flush(audioEnc_);
    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeOutputFunc, this);
    result = OH_AudioEncoder_Start(audioEnc_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();

    result = OH_AudioEncoder_Destroy(audioEnc_);
    cout << "Destroy ret is: " << result << endl;
}

void AudioEncoderDemo::NativeRunCaseReset(std::string inputFile, std::string outputFileFirst,
    std::string outputFileSecond, const char* name, OH_AVFormat* format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    OH_AVErrCode result;
    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();

    // Reset
    result = OH_AudioEncoder_Reset(audioEnc_);

    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;

    result = OH_AudioEncoder_Configure(audioEnc_, format);
    if (result < 0) {
        cout << "Configure fail! ret is " << result << endl;
        return;
    }

    result = OH_AudioEncoder_Prepare(audioEnc_);
    cout << "Prepare ret is: " << result << endl;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeOutputFunc, this);
    result = OH_AudioEncoder_Start(audioEnc_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();

    result = OH_AudioEncoder_Destroy(audioEnc_);
    cout << "Destroy ret is: " << result << endl;
}


OH_AVFormat* AudioEncoderDemo::NativeRunCaseGetOutputDescription(std::string inputFile,
    std::string outputFile, const char* name, OH_AVFormat* format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;
    isGetOutputDescription = true;

    OH_AVErrCode result;
    NativeCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();

    result = OH_AudioEncoder_Destroy(audioEnc_);
    cout << "Destroy ret is: " << result << endl;

    return curFormat;
}

// for test
void AudioEncoderDemo::TestOutputFunc()
{
    std::ofstream outputFile;
    outputFile.open(outputFilePath, std::ios::out | std::ios::binary);
    if (!outputFile.is_open()) {
        std::cout << "open " << outputFilePath << " failed!" << std::endl;
    }

    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        cout << "output wait !!!" << endl;
        signal_->outCond_.wait(lock, [this]() {
            return (signal_->outQueue_.size() > 0 || !isRunning_.load());
            });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory* data = signal_->outBufferQueue_.front();

        if (data != nullptr) {
            cout << "OutputFunc write file,buffer index" << index << ", data size = " << attr.size << endl;
            int64_t size = attr.size;
            outputFile.write((char*)(&size), sizeof(size));
            int64_t pts = attr.pts;
            outputFile.write((char*)(&pts), sizeof(pts));
            outputFile.write(reinterpret_cast<char*>(OH_AVMemory_GetAddr(data)), attr.size);
        }

        cout << "attr.flags: " << attr.flags << endl;
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS || attr.size == 0) {
            cout << "encode eos" << endl;
            isRunning_.store(false);
        }

        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();

        OH_AVErrCode ret = OH_AudioEncoder_FreeOutputData(audioEnc_, index);
        if (ret != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
    }
    outputFile.close();
}

void AudioEncoderDemo::TestRunCase(std::string inputFile, std::string outputFile,
    const char* name, OH_AVFormat* format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    cout << "create name is " << name << endl;
    audioEnc_ = OH_AudioEncoder_CreateByName(name);
    if (audioEnc_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    if (strcmp(name, "OH.Media.Codec.Encoder.Audio.Flac") == 0) {
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, &inputBufSize);
    } else if (strcmp(name, "OH.Media.Codec.Encoder.Audio.AAC") == 0) {
        int channels;
        OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, &channels);
        inputBufSize = channels * AAC_FRAME_SIZE * AAC_DEFAULT_BYTES_PER_SAMPLE;
    }

    OH_AVErrCode result;

    cb_ = { &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable };
    result = OH_AudioEncoder_SetCallback(audioEnc_, cb_, signal_);
    cout << "SetCallback ret is: " << result << endl;

    result = OH_AudioEncoder_Configure(audioEnc_, format);
    cout << "Configure ret is: " << result << endl;
    if (result < 0) {
        cout << "Configure fail! ret is " << result << endl;
        return;
    }

    result = OH_AudioEncoder_Prepare(audioEnc_);
    cout << "Prepare ret is: " << result << endl;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::NativeInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::TestOutputFunc, this);
    result = OH_AudioEncoder_Start(audioEnc_);
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }

    NativeStopAndClear();

    result = OH_AudioEncoder_Destroy(audioEnc_);
    cout << "Destroy ret is: " << result << endl;
}


// inner
int32_t AudioEncoderDemo::InnerCreateByMime(const std::string& mime)
{
    inneraudioEnc_ = AudioEncoderFactory::CreateByMime(mime);
    std::cout << "InnerCreateByMime" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return AVCS_ERR_OK;
}

int32_t AudioEncoderDemo::InnerCreateByName(const std::string& name)
{
    inneraudioEnc_ = AudioEncoderFactory::CreateByName(name);
    std::cout << "InnerCreateByName" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return AVCS_ERR_OK;
}

int32_t AudioEncoderDemo::InnerSetCallback(const std::shared_ptr<AVCodecCallback>& callback)
{
    return inneraudioEnc_->SetCallback(callback);
}

int32_t AudioEncoderDemo::InnerConfigure(const Format& format)
{
    cout << "InnerConfigure" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioEnc_->Configure(format);
}

int32_t AudioEncoderDemo::InnerStart()
{
    cout << "InnerStart" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioEnc_->Start();
}

int32_t AudioEncoderDemo::InnerStartWithThread()
{
    if (!isRunning_.load()) {
        cout << "InnerStartWithThread" << endl;
        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerUpdateInputData, this);
        outputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerUpdateOutputData, this);
    }
    int32_t ret = inneraudioEnc_->Start();
    sleep(1);
    return ret;
}

int32_t AudioEncoderDemo::InnerPrepare()
{
    cout << "InnerPrepare" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioEnc_->Prepare();
}

int32_t AudioEncoderDemo::InnerStop()
{
    InnerStopThread();
    cout << "InnerStop" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }

    return inneraudioEnc_->Stop();
}

int32_t AudioEncoderDemo::InnerFlush()
{
    cout << "InnerFlush" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    int32_t ret = inneraudioEnc_->Flush();

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

int32_t AudioEncoderDemo::InnerReset()
{
    InnerStopThread();
    cout << "InnerReset" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioEnc_->Reset();
}

int32_t AudioEncoderDemo::InnerRelease()
{
    cout << "InnerRelease" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioEnc_->Release();
}

int32_t AudioEncoderDemo::InnerSetParameter(const Format& format)
{
    cout << "InnerSetParameter" << endl;
    if (inneraudioEnc_ == nullptr) {
        std::cout << "InnerEncoder create failed!" << std::endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    return inneraudioEnc_->SetParameter(format);
}

int32_t AudioEncoderDemo::InnerDestroy()
{
    InnerStopThread();
    int ret = InnerRelease();
    inneraudioEnc_ = nullptr;
    return ret;
}

int32_t AudioEncoderDemo::InnerQueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
    AVCodecBufferFlag flag)
{
    cout << "InnerQueueInputBuffer" << endl;
    return inneraudioEnc_->QueueInputBuffer(index, info, flag);
}

int32_t AudioEncoderDemo::InnerGetOutputFormat(Format& format)
{
    cout << "InnerGetOutputFormat" << endl;
    return inneraudioEnc_->GetOutputFormat(format);
}

int32_t AudioEncoderDemo::InnerReleaseOutputBuffer(uint32_t index)
{
    cout << "InnerReleaseOutputBuffer" << endl;
    return inneraudioEnc_->ReleaseOutputBuffer(index);
}

void AudioEncoderDemo::InnerStopThread()
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

    while (!innersignal_->inQueue_.empty()) innersignal_->inQueue_.pop();
    while (!innersignal_->outQueue_.empty()) innersignal_->outQueue_.pop();
    while (!innersignal_->infoQueue_.empty()) innersignal_->infoQueue_.pop();
    while (!innersignal_->flagQueue_.empty()) innersignal_->flagQueue_.pop();
    while (!innersignal_->inInnerBufQueue_.empty()) innersignal_->inInnerBufQueue_.pop();
    while (!innersignal_->outInnerBufQueue_.empty()) innersignal_->outInnerBufQueue_.pop();
}

void AudioEncoderDemo::InnerHandleEOS(const uint32_t& index)
{
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.size = 0;
    info.offset = 0;
    info.presentationTimeUs = 0;
    flag = AVCODEC_BUFFER_FLAG_EOS;
    inneraudioEnc_->QueueInputBuffer(index, info, flag);
    std::cout << "end buffer\n";
    innersignal_->inQueue_.pop();
}

void AudioEncoderDemo::InnerInputFunc()
{
    inputFile_.open(inputFilePath, std::ios::binary);
    if (!inputFile_.is_open()) {
        std::cout << "open file " << inputFilePath << " failed" << std::endl;
        isRunning_.store(false);
        return;
    }
    while (isRunning_.load()) {
        std::unique_lock<std::mutex> lock(innersignal_->inMutex_);
        innersignal_->inCond_.wait(lock, [this]() {
            return (innersignal_->inQueue_.size() > 0 || !isRunning_.load());
            });
        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = innersignal_->inQueue_.front();
        auto buffer = innersignal_->inInnerBufQueue_.front();
        if (buffer == nullptr) {
            isRunning_.store(false);
            std::cout << "buffer is null:" << index << "\n";
            break;
        }

        if (!inputFile_.eof()) {
            inputFile_.read((char*)buffer->GetBase(), inputBufSize);
        } else {
            InnerHandleEOS(index);
            break;
        }

        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        info.size = inputBufSize;
        info.offset = 0;

        int32_t ret = AVCS_ERR_OK;

        flag = AVCODEC_BUFFER_FLAG_NONE;
        ret = inneraudioEnc_->QueueInputBuffer(index, info, flag);
        timeStamp_ += FRAME_DURATION_US;
        innersignal_->inQueue_.pop();
        innersignal_->inInnerBufQueue_.pop();

        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            isRunning_ = false;
            break;
        }
    }
    inputFile_.close();
}

void AudioEncoderDemo::InnerOutputFunc()
{
    std::ofstream outputFile;
    outputFile.open(outputFilePath, std::ios::out | std::ios::binary);
    if (!outputFile.is_open()) {
        std::cout << "open " << outputFilePath << " failed!" << std::endl;
        return;
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(innersignal_->outMutex_);
        innersignal_->outCond_.wait(lock, [this]() {
            return (innersignal_->outQueue_.size() > 0 || !isRunning_.load());
            });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = innersignal_->outQueue_.front();
        auto flag = innersignal_->flagQueue_.front();
        auto buffer = innersignal_->outInnerBufQueue_.front();
        if (buffer == nullptr) {
            cout << "get output buffer is nullptr" << ", index:" << index << endl;
        }
        if (buffer != nullptr) {
            auto info = innersignal_->infoQueue_.front();

            cout << "OutputFunc write file,buffer index" << index << ", data size = :" << info.size << endl;
            outputFile.write((char*)buffer->GetBase(), info.size);
        }

        if (flag == AVCODEC_BUFFER_FLAG_EOS) {
            cout << "encode eos" << endl;
            isRunning_.store(false);
            break;
        }
        if (inneraudioEnc_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }
        innersignal_->outQueue_.pop();
        innersignal_->infoQueue_.pop();
        innersignal_->flagQueue_.pop();
        innersignal_->outInnerBufQueue_.pop();
    }
    outputFile.close();
}

void AudioEncoderDemo::InnerCreateToStart(const std::string& name, Format& format)
{
    int32_t result;
    
    cout << "create name is " << name << endl;
    InnerCreateByName(name);
    if (inneraudioEnc_ == nullptr) {
        cout << "create fail!!" << endl;
        return;
    }

    if (name == "OH.Media.Codec.Encoder.Audio.Flac") {
        inputBufSize = INPUT_FRAME_BYTES;
    } else if (name == "OH.Media.Codec.Encoder.Audio.AAC") {
        int32_t channels;
        format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
        inputBufSize = channels * AAC_FRAME_SIZE * AAC_DEFAULT_BYTES_PER_SAMPLE;
    }

    cout << "create done" << endl;
    innersignal_ = getSignal();
    cout << "innersignal_: " << innersignal_ << endl;
    innercb_ = make_unique<InnerAEnDemoCallback>(innersignal_);
    result = InnerSetCallback(innercb_);
    cout << "SetCallback ret is: " << result << endl;

    result = InnerConfigure(format);
    cout << "Configure ret is: " << result << endl;
    if (result != 0) {
        cout << "Configure fail!! ret is " << result << endl;
        return;
    }
    result = InnerPrepare();
    cout << "InnerPrepare ret is: " << result << endl;

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerOutputFunc, this);
    result = InnerStart();
    cout << "Start ret is: " << result << endl;
}

void AudioEncoderDemo::InnerStopAndClear()
{
    int32_t result;
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
    result = InnerStop();
    cout << "Stop ret is: " << result << endl;
}

void AudioEncoderDemo::InnerRunCase(std::string inputFile, std::string outputFile,
    const std::string& name, Format& format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFile;

    int32_t result;

    InnerCreateToStart(name, format);
    while (isRunning_.load()) {
        sleep(1);
    }

    // Stop
    InnerStopAndClear();

    result = InnerDestroy();
    cout << "Destroy ret is: " << result << endl;
}


void AudioEncoderDemo::InnerRunCaseFlush(std::string inputFile, std::string outputFileFirst,
    std::string outputFileSecond, const std::string& name, Format& format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    int32_t result;
    InnerCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }
    InnerStopAndClear();

    // flush
    result = InnerFlush();
    cout << "InnerFlush ret is: " << result << endl;
    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerOutputFunc, this);
    result = InnerStart();
    cout << "Start ret is: " << result << endl;

    while (isRunning_.load()) {
        sleep(1);
    }
    // Stop
    InnerStopAndClear();

    result = InnerDestroy();
    cout << "Destroy ret is: " << result << endl;
}

void AudioEncoderDemo::InnerRunCaseReset(std::string inputFile, std::string outputFileFirst,
    std::string outputFileSecond, const std::string& name, Format& format)
{
    inputFilePath = inputFile;
    outputFilePath = outputFileFirst;

    int32_t result;
    InnerCreateToStart(name, format);

    while (isRunning_.load()) {
        sleep(1);
    }
    // Stop
    InnerStopAndClear();

    // reset
    result = InnerReset();
    cout << "InnerReset ret is: " << result << endl;

    inputFilePath = inputFile;
    outputFilePath = outputFileSecond;

    result = InnerConfigure(format);
    cout << "Configure ret is: " << result << endl;
    if (result != 0) {
        cout << "Configure fail!!" << endl;
        return;
    }
    result = InnerPrepare();
    cout << "Configure ret is: " << result << endl;

    // Start
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerInputFunc, this);
    outputLoop_ = make_unique<thread>(&AudioEncoderDemo::InnerOutputFunc, this);
    result = InnerStart();
    cout << "Start ret is: " << result << endl;
    while (isRunning_.load()) {
        sleep(1);
    }

    InnerStopAndClear();

    result = InnerDestroy();
    cout << "Destroy ret is: " << result << endl;
}

std::shared_ptr<AEncSignal> AudioEncoderDemo::getSignal()
{
    return innersignal_;
}


InnerAEnDemoCallback::InnerAEnDemoCallback(shared_ptr<AEncSignal> signal) : innersignal_(signal) {}

void InnerAEnDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void InnerAEnDemoCallback::OnOutputFormatChanged(const Format& format)
{
    (void)format;
    cout << "OnOutputFormatChanged received" << endl;
}

void InnerAEnDemoCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    if (innersignal_ == nullptr) {
        std::cout << "buffer is null 1" << endl;
    }
    unique_lock<mutex> lock(innersignal_->inMutex_);
    innersignal_->inQueue_.push(index);
    innersignal_->inInnerBufQueue_.push(buffer);
    innersignal_->inCond_.notify_all();
}

void InnerAEnDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
    AVCodecBufferFlag flag, std::shared_ptr<AVSharedMemory> buffer)
{
    (void)info;
    (void)flag;
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(innersignal_->outMutex_);
    innersignal_->outQueue_.push(index);
    innersignal_->infoQueue_.push(info);
    innersignal_->flagQueue_.push(flag);
    innersignal_->outInnerBufQueue_.push(buffer);
    cout << "**********out info size = " << info.size << endl;
    innersignal_->outCond_.notify_all();
}

uint32_t AudioEncoderDemo::InnerGetInputIndex()
{
    while (inIndexQueue_.empty()) sleep(1);
    uint32_t inputIndex = inIndexQueue_.front();
    inIndexQueue_.pop();
    return inputIndex;
}

uint32_t AudioEncoderDemo::InnerGetOutputIndex()
{
    if (outIndexQueue_.empty()) {
        return ERROR_INDEX;
    }
    uint32_t outputIndex = outIndexQueue_.front();
    outIndexQueue_.pop();
    return outputIndex;
}

void AudioEncoderDemo::InnerUpdateInputData()
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

void AudioEncoderDemo::InnerUpdateOutputData()
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
