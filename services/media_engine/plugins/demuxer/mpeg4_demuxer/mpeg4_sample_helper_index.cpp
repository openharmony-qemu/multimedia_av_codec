/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "MPEG4SampleHelper"
#include <algorithm>
#include <limits>

#include "common/log.h"
#include "mpeg4_sample_helper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4SampleHelper"};
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
void MPEG4SampleHelper::SetTimeOffset(int64_t timeOffset)
{
    timeOffset_ = timeOffset;
}

void MPEG4SampleHelper::CalculateDtsShift()
{
    dtsShift_ = 0;
    for (const auto& entry : cttsEntry_) {
        if (entry.delta < 0) {
            dtsShift_ = std::max(dtsShift_, static_cast<int64_t>(-entry.delta));
        }
    }
}

uint32_t MPEG4SampleHelper::GetSttsEntryCount()
{
    return sttsEntry_.size();
}

uint32_t MPEG4SampleHelper::GetSampleCountOfSttsEntry(uint32_t sttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(sttsEntryIndex >= 0 && sttsEntryIndex < sttsEntry_.size(), 0,
        "sttsEntryIndex(%{public}d) is invalid", sttsEntryIndex);
    return sttsEntry_[sttsEntryIndex].count;
}

uint32_t MPEG4SampleHelper::GetSampleDeltaOfSttsEntry(uint32_t sttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(sttsEntryIndex >= 0 && sttsEntryIndex < sttsEntry_.size(), 0,
        "sttsEntryIndex(%{public}d) is invalid", sttsEntryIndex);
    return sttsEntry_[sttsEntryIndex].delta;
}

uint32_t MPEG4SampleHelper::GetCttsEntryCount()
{
    return cttsEntry_.size();
}

uint32_t MPEG4SampleHelper::GetSampleCountOfCttsEntry(uint32_t cttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(cttsEntryIndex >= 0 && cttsEntryIndex < cttsEntry_.size(), 0,
        "cttsEntryIndex(%{public}d) is invalid", cttsEntryIndex);
    return cttsEntry_[cttsEntryIndex].count;
}

int32_t MPEG4SampleHelper::GetSampleOffsetOfCttsEntry(uint32_t cttsEntryIndex)
{
    FALSE_RETURN_V_MSG_E(cttsEntryIndex >= 0 && cttsEntryIndex < cttsEntry_.size(), 0,
        "cttsEntryIndex(%{public}d) is invalid", cttsEntryIndex);
    return cttsEntry_[cttsEntryIndex].delta;
}

int32_t MPEG4SampleHelper::GetMinOffsetOfCttsEntry()
{
    int32_t minSampleOffsetOfCttsEntry = INT32_MAX;
    for (uint32_t i = 0; i < cttsEntry_.size(); ++i) {
        minSampleOffsetOfCttsEntry = minSampleOffsetOfCttsEntry < cttsEntry_[i].delta ? minSampleOffsetOfCttsEntry :
            cttsEntry_[i].delta;
    }
    return std::min(0, minSampleOffsetOfCttsEntry);
}

std::vector<uint32_t> MPEG4SampleHelper::GetSyncSamplesIndexFromStssEntry()
{
    std::vector<uint32_t> syncSamplesIndex;
    for (uint32_t i = 0; i < stssEntry_.size(); ++i) {
        syncSamplesIndex.emplace_back(stssEntry_[i].index);
    }
    return syncSamplesIndex;
}

MPEG4SampleHelper::SdtpEntry MPEG4SampleHelper::GetSdtpEntry()
{
    return sdtpEntry_;
}

Status MPEG4SampleHelper::GetPtsOfFirstSample(int64_t& startPtsBasedOnTimescale)
{
    FALSE_RETURN_V_MSG_E(!sampleIndexEntry_.empty(), Status::ERROR_INVALID_DATA, "sampleIndexEntry_ is empty");
    return sampleIndexEntry_[0].GetPts(startPtsBasedOnTimescale) ? Status::OK : Status::ERROR_INVALID_DATA;
}

void MPEG4SampleHelper::SetFragmented(bool fragmented)
{
    MEDIA_LOG_I("SetFragmented in");
    isFragmented_ = fragmented;
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
