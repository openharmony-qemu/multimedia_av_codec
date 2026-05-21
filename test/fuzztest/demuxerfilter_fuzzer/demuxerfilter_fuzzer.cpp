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

#include <memory>
#include <fuzzer/FuzzedDataProvider.h>
#include "demuxer_filter.h"

using namespace OHOS::Media;
using namespace OHOS::Media::Pipeline;

namespace {
constexpr size_t FUZZ_INPUT_MIN_SIZE = 4;
constexpr int NUMBER_1080 = 1080;
constexpr int NUMBER_1920 = 1920;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    if (data == nullptr || size < FUZZ_INPUT_MIN_SIZE) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);
    (void)provider;

    auto filter = std::make_shared<DemuxerFilter>("demuxer_filter_fuzz", FilterType::FILTERTYPE_DEMUXER);
    filter->GetSeekableRanges();
    filter->GetLoadedRanges();
    TrackSelectionFilter selectionFilter;
    selectionFilter.preferredVideoMimeTypes.push_back("video/avc");
    selectionFilter.preferredAudioLanguages.push_back("en");
    selectionFilter.preferredSubtitleLanguages.push_back("zh");
    filter->SetTrackSelectionFilter(selectionFilter);

    auto playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->audioLanguage = "en";
    playStrategy->subtitleLanguage = "zh";
    playStrategy->preferHDR = provider.ConsumeBool();
    playStrategy->width = provider.ConsumeIntegralInRange<uint32_t>(1, NUMBER_1920);
    playStrategy->height = provider.ConsumeIntegralInRange<uint32_t>(1, NUMBER_1080);
    filter->SetPlayStrategy(playStrategy);
    filter->SetPlayStrategy(nullptr);
    return 0;
}
