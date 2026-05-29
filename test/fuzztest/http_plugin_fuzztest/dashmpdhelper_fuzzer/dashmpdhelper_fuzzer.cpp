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

#include "dash_mpd_downloader.h"

#include <fuzzer/FuzzedDataProvider.h>
#include <memory>
#include <string>

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
    auto downloader = std::make_shared<DashMpdDownloader>(nullptr);

    std::string input = provider.ConsumeRandomLengthString(64);
    downloader->ContainsNonDigit(input);

    uint32_t frameRate = 0;
    std::string frameRateStr = "24/1";
    downloader->ParseFrameRateToUint32(frameRateStr, frameRate);

    std::string mimeType = provider.ConsumeRandomLengthString(32);
    OHOS::Media::Plugins::StreamType type =
        provider.ConsumeBool() ? OHOS::Media::Plugins::StreamType::VIDEO : OHOS::Media::Plugins::StreamType::AUDIO;
    downloader->GetMimeType(mimeType, type);

    return 0;
}
