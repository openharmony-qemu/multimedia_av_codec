/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PTS_AND_INDEX_CONVERSION_TEST_UTILS_H
#define PTS_AND_INDEX_CONVERSION_TEST_UTILS_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "pts_and_index_conversion.h"

namespace OHOS::Media::PtsAndIndexConversionTestUtils {

constexpr size_t MAX_CONTAINER_DEPTH = 10;
constexpr uint32_t EXCESSIVE_ENTRY_COUNT = 36001;

bool RunLargeSizeReproduction(const char *targetType);
bool RunDeepContainerReproduction(int &childStatus);
bool ParseSmallSizeMedia(const char *targetType, const std::vector<uint8_t> &payload,
    TimeAndIndexConversion &conversion);
bool ParseDeepContainerMedia(size_t depth, TimeAndIndexConversion &conversion);

std::vector<uint8_t> MakeTimeEntryPayload(uint32_t entryCount, uint32_t secondValue);
std::vector<uint8_t> MakeHdlrPayload();
std::vector<uint8_t> MakeMdhdPayload(uint8_t version);

} // namespace OHOS::Media::PtsAndIndexConversionTestUtils

#endif
