/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <fuzzer/FuzzedDataProvider.h>
#include <memory>

#include "demuxer/sample_queue.h"
#include "demuxer/sample_queue_controller.h"

using namespace OHOS::Media;

namespace {
constexpr size_t MIN_SIZE = 4;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_SIZE) {
        return 0;
    }

    FuzzedDataProvider provider(data, size);
    auto controller = std::make_shared<SampleQueueController>();
    auto sampleQueue = std::make_shared<SampleQueue>();

    SampleQueue::Config config;
    config.queueId_ = provider.ConsumeIntegral<int32_t>();
    config.queueSize_ = provider.ConsumeIntegralInRange<uint32_t>(1, SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE);
    sampleQueue->Init(config);

    int32_t trackId = provider.ConsumeIntegral<int32_t>();
    controller->CheckWaterLineStopProduce(trackId, sampleQueue);
    controller->CheckWaterLineStartConsume(trackId, sampleQueue);
    controller->GetCacheDuration(sampleQueue);

    controller->CheckWaterLineStopProduce(trackId, nullptr);
    controller->CheckWaterLineStartConsume(trackId, nullptr);
    controller->GetCacheDuration(nullptr);

    return 0;
}
