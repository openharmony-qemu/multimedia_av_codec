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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "mpeg4demuxerplugin_sample.h"

#define FUZZ_PROJECT_NAME "demuxerpluginmpeg4_fuzzer"

namespace OHOS {
namespace {
constexpr size_t MIN_INPUT_SIZE = 32;
constexpr uint32_t MAX_TIMEOUT_MS = 100;
constexpr uint32_t MAX_CACHE_LIMIT = 1024 * 1024;
constexpr uint32_t MAX_PROBE_SIZE = 1024 * 1024;
constexpr int64_t MAX_SEEK_TIME_MS = 60 * 1000;
constexpr int64_t MAX_READ_TIMEOUT_MS = 1000;

uint32_t ConsumeBoundedU32(FuzzedDataProvider &provider, uint32_t maxValue)
{
    return provider.ConsumeIntegralInRange<uint32_t>(0, maxValue);
}

int64_t ConsumeBoundedI64(FuzzedDataProvider &provider, int64_t minValue,
                          int64_t maxValue)
{
    return provider.ConsumeIntegralInRange<int64_t>(minValue, maxValue);
}

Media::Mpeg4DemuxerPluginFuzzConfig ConsumeConfig(FuzzedDataProvider &provider)
{
    Media::Mpeg4DemuxerPluginFuzzConfig config;
    config.useEmptyBuffer = provider.ConsumeBool();
    config.useConfigs = provider.ConsumeBool();
    config.interruptState = provider.ConsumeBool();
    config.stopReadPacketState = provider.ConsumeBool();
    config.invalidTrackId = ConsumeBoundedU32(provider, UINT16_MAX);
    config.timeoutMs = ConsumeBoundedU32(provider, MAX_TIMEOUT_MS);
    config.cacheLimit = ConsumeBoundedU32(provider, MAX_CACHE_LIMIT);
    config.trackCacheLimit = ConsumeBoundedU32(provider, MAX_CACHE_LIMIT);
    config.trackCacheWindowMs = ConsumeBoundedU32(provider, MAX_TIMEOUT_MS);
    config.frameId = provider.ConsumeIntegral<uint32_t>();
    config.gopId = provider.ConsumeIntegral<uint32_t>();
    config.relativePresentationTimeUs = provider.ConsumeIntegral<uint64_t>();
    config.probSize =
        static_cast<int32_t>(ConsumeBoundedU32(provider, MAX_PROBE_SIZE));
    config.seekTimeMs =
        ConsumeBoundedI64(provider, -MAX_SEEK_TIME_MS, MAX_SEEK_TIME_MS);
    config.secondSeekTimeMs =
        ConsumeBoundedI64(provider, -MAX_SEEK_TIME_MS, MAX_SEEK_TIME_MS);
    config.readTimeoutMs =
        ConsumeBoundedI64(provider, -MAX_READ_TIMEOUT_MS, MAX_READ_TIMEOUT_MS);
    config.dts = provider.ConsumeIntegral<int64_t>();
    return config;
}

void RunDemuxerPluginMpeg4FuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_INPUT_SIZE) {
        return;
    }
    FuzzedDataProvider provider(data, size);
    Media::Mpeg4DemuxerPluginFuzzConfig config = ConsumeConfig(provider);
    std::vector<uint8_t> streamData = provider.ConsumeRemainingBytes<uint8_t>();
    if (streamData.empty()) {
        return;
    }

    auto demuxerTest = std::make_shared<Media::Mpeg4DemuxerPluginFuzzTest>();
    if (!demuxerTest->InitWithData(streamData.data(), streamData.size())) {
        return;
    }
    demuxerTest->Run(config);
}
} // namespace

void DemuxerPluginMpeg4FuzzTest(const uint8_t *data, size_t size)
{
    RunDemuxerPluginMpeg4FuzzTest(data, size);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DemuxerPluginMpeg4FuzzTest(data, size);
    return 0;
}
