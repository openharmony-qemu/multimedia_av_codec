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
#include <string>

#include "hls/m3u8.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

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
    std::string playlist = provider.ConsumeRandomLengthString(256);
    std::string uri = provider.ConsumeRandomLengthString(64);
    uint32_t initResolution = provider.ConsumeIntegral<uint32_t>();

    M3U8MasterPlaylist master(playlist, uri, initResolution);

    int variantCount = provider.ConsumeIntegralInRange<int>(0, 4);
    for (int i = 0; i < variantCount; ++i) {
        auto variant = std::make_shared<M3U8VariantStream>(
            provider.ConsumeRandomLengthString(8),
            provider.ConsumeRandomLengthString(8),
            nullptr);
        variant->streamId_ = provider.ConsumeIntegral<int32_t>();
        variant->bandWidth_ = provider.ConsumeIntegral<uint64_t>();
        variant->width_ = provider.ConsumeIntegral<uint32_t>();
        variant->height_ = provider.ConsumeIntegral<uint32_t>();
        variant->frameRate_ = provider.ConsumeIntegral<uint64_t>();
        master.variants_.emplace_back(variant);
        if (master.defaultVariant_ == nullptr) {
            master.defaultVariant_ = variant;
        }
    }

    std::string codecs = provider.ConsumeRandomLengthString(128);
    master.IsVideoStream(codecs);
    master.GetCodec(OHOS::Media::Plugins::StreamType::VIDEO, codecs);
    master.GetCodec(OHOS::Media::Plugins::StreamType::AUDIO, codecs);
    master.GetMimeType(codecs);

    int32_t streamId = provider.ConsumeIntegral<int32_t>();
    master.ChooseStreamByStream(streamId);
    master.ChooseStreamByResolution();

    return 0;
}
