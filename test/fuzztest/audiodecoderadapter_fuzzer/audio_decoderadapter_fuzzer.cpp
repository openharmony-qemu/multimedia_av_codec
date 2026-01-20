/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <cmath>
#include <iostream>
#include <fuzzer/FuzzedDataProvider.h>
#include "audio_decoder_adapter.h"
#include "audio_decoderadapter_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
static const int32_t MIN_SIZE_NUM = 4;
static const size_t MAX_ALLOWED_SIZE = 4096;
AudioDecoderAdapterFuzzer::AudioDecoderAdapterFuzzer()
{
}

AudioDecoderAdapterFuzzer::~AudioDecoderAdapterFuzzer()
{
}

void AudioDecoderAdapterFuzzer::ResetFuzzTest()
{
    auto audioDecoderAdapter = std::make_unique<AudioDecoderAdapter>();
    audioDecoderAdapter->Start();
    audioDecoderAdapter->Reset();
}

void AudioDecoderAdapterFuzzer::GetInputBufferQueueConsumerFuzzTest()
{
    auto audioDecoderAdapter = std::make_unique<AudioDecoderAdapter>();
    audioDecoderAdapter->Start();
    sptr<Media::AVBufferQueueConsumer> consumer = audioDecoderAdapter->GetInputBufferQueueConsumer();
    sptr<Media::AVBufferQueueProducer> producer = audioDecoderAdapter->GetOutputBufferQueueProducer();
}

void AudioDecoderAdapterFuzzer::ProcessInputBufferInnerFuzzTest(FuzzedDataProvider& provider)
{
    auto audioDecoderAdapter = std::make_unique<AudioDecoderAdapter>();
    bool isTriggeredByOutPort = provider.ConsumeBool();
    bool isFlush = provider.ConsumeBool();
    uint32_t bufferStatus = provider.ConsumeIntegralInRange<uint32_t>(0, UINT32_MAX);
    audioDecoderAdapter->Start();
    audioDecoderAdapter->ProcessInputBufferInner(isTriggeredByOutPort, isFlush, bufferStatus);
}

void AudioDecoderAdapterFuzzer::OnDumpInfoFuzzTest(FuzzedDataProvider& provider)
{
    auto audioDecoderAdapter = std::make_unique<AudioDecoderAdapter>();
    int32_t fd = provider.ConsumeIntegral<int32_t>();
    audioDecoderAdapter->Start();
    sptr<Media::AVBufferQueueConsumer> consumer = audioDecoderAdapter->GetInputBufferQueueConsumer();
    audioDecoderAdapter->OnDumpInfo(fd);
}

void AudioDecoderAdapterFuzzer::NotifyEosFuzzTest()
{
    auto audioDecoderAdapter = std::make_unique<AudioDecoderAdapter>();
    audioDecoderAdapter->Start();
    audioDecoderAdapter->NotifyEos();
}

 /* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < MIN_SIZE_NUM) || (size > MAX_ALLOWED_SIZE)) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);
 	/* Run your code on data */
    AudioDecoderAdapterFuzzer::ResetFuzzTest();
    AudioDecoderAdapterFuzzer::GetInputBufferQueueConsumerFuzzTest();
    AudioDecoderAdapterFuzzer::ProcessInputBufferInnerFuzzTest(provider);
    AudioDecoderAdapterFuzzer::OnDumpInfoFuzzTest(provider);
    AudioDecoderAdapterFuzzer::NotifyEosFuzzTest();
    return 0;
}
}
}