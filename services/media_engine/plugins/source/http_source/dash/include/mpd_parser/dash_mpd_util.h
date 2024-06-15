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

#ifndef DASH_MPD_UTIL_H
#define DASH_MPD_UTIL_H

#include <cstdint>
#include "mpd_parser_def.h"
#include "dash_mpd_def.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
bool DashUrlIsAbsolute(const std::string &url);
void DashAppendBaseUrl(std::string &srcUrl, DashList<std::string> baseUrlList);
void DashAppendBaseUrl(std::string &srcUrl, std::string baseUrl);
void BuildSrcUrl(std::string &srcUrl, std::string &baseUrl);

/**
 * @brief    Replace SegmentTemplate Identifier By Substitute Parameter
 *
 * @param    segTmpltStr            SegmentTemplate Url String
 * @param    segTmpltIdentifier     SegmentTemplate Identifer
 * @param    substitutionStr        substitute paramter
 *
 * @return   0 -- success or -1 -- failed
 */
int32_t DashSubstituteTmpltStr(std::string &segTmpltStr, const std::string &segTmpltIdentifier,
                               std::string substitutionStr);

int32_t DashStrToDuration(const std::string &str, uint32_t &duration);
uint32_t DashGetAttrIndex(const std::string &attrName, const char *const *nodeAttrs, uint32_t attrNums);
void DashParseRange(const std::string &rangeStr, int64_t &startRange, int64_t &endRange);
bool DashStreamIsHdr(DashList<DashDescriptor*> essentialPropertyList);
void GetSubstitutionStr(std::string &substitutionStr, std::string &str);
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_MPD_UTIL_H
