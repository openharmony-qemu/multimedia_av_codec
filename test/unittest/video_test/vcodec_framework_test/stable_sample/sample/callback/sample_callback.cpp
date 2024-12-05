/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include "unittest_utils.h"
#include "unittest_log.h"
#include "vdec_sample.h"

namespace OHOS {
namespace MediaAVCodec {
void OnErrorVoid(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}
void OnStreamChangedVoid(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}
void InDataVoid(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)userData;
}
void OutDataVoid(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)attr;
    (void)userData;
}
void InBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}
void OutBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}

void InDataHandle(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    codecSample->HandleInputFrame(std::make_shared<CodecBufferInfo>(index, data));
}

void OutDataHandle(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    codecSample->HandleOutputFrame(std::make_shared<CodecBufferInfo>(index, data, *attr));
}

void InBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    codecSample->HandleInputFrame(std::make_shared<CodecBufferInfo>(index, buffer));
}

void OutBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    codecSample->HandleOutputFrame(std::make_shared<CodecBufferInfo>(index, buffer));
}

void InDataOperate(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(codecSample->Operate(), AV_ERR_OK);
        return;
    }
    codecSample->HandleInputFrame(std::make_shared<CodecBufferInfo>(index, data));
}

void OutDataOperate(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(codecSample->Operate(), AV_ERR_OK);
        return;
    }
    codecSample->HandleOutputFrame(std::make_shared<CodecBufferInfo>(index, data, *attr));
}

void InBufferOperate(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(codecSample->Operate(), AV_ERR_OK);
        return;
    }
    codecSample->HandleInputFrame(std::make_shared<CodecBufferInfo>(index, buffer));
}

void OutBufferOperate(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(codecSample->Operate(), AV_ERR_OK);
        return;
    }
    codecSample->HandleOutputFrame(std::make_shared<CodecBufferInfo>(index, buffer));
}

void InDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    signal->inQueue_.Enqueue(std::make_shared<CodecBufferInfo>(index, data));
}

void OutDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    signal->outQueue_.Enqueue(std::make_shared<CodecBufferInfo>(index, data, *attr));
}

void InBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    signal->inQueue_.Enqueue(std::make_shared<CodecBufferInfo>(index, buffer));
}

void OutBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    signal->outQueue_.Enqueue(std::make_shared<CodecBufferInfo>(index, buffer));
}

void InputBufferLoop(std::shared_ptr<VCodecSignal> &signal)
{
    auto codecSample = signal->codec_.lock();
    pthread_setname_np(pthread_self(), "inloop");
    while (signal->isRunning_.load()) {
        auto bufferInfo = signal->inQueue_.Dequeue();
        if (bufferInfo == nullptr || !bufferInfo->IsValid()) {
            continue;
        }
        codecSample->HandleInputFrame(bufferInfo);
    }
}

void OutputBufferLoop(std::shared_ptr<VCodecSignal> &signal)
{
    auto codecSample = signal->codec_.lock();
    pthread_setname_np(pthread_self(), "outloop");
    while (signal->isRunning_.load()) {
        auto bufferInfo = signal->outQueue_.Dequeue();
        if (bufferInfo == nullptr || !bufferInfo->IsValid()) {
            continue;
        }
        codecSample->HandleOutputFrame(bufferInfo);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS