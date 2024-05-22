/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef SIDX_BOX_PARSER_H
#define SIDX_BOX_PARSER_H

#include "mpd_parser_def.h"
#include <memory>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class SidxBoxParser {
public:
    static int32_t ParseSidxBox(char *bitStream, uint32_t streamSize, int64_t sidxEndOffset,
                                DashList<std::shared_ptr<SubSegmentIndex>> &subSegIndexTable);

    SidxBoxParser() = default;
    ~SidxBoxParser();

private:
    static void BuildSubSegmentIndexes(char *bitStream, int64_t sidxEndOffset,
                                       DashList<std::shared_ptr<SubSegmentIndex>> &subSegIndexTable, uint32_t &currPos);

private:
    static constexpr uint32_t BASE_BOX_HEAD_SIZE = 8;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // SIDX_BOX_PARSER_H