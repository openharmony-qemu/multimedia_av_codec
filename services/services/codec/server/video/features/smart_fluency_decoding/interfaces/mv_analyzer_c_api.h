/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MV_ANALYZER_C_API_H
#define MV_ANALYZER_C_API_H

#include <stddef.h>
#include "smart_fluency_decoding_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* MvAnalyzerHandle;

MvAnalyzerHandle CreateMvAnalyzer(int32_t codecType);

void DestroyMvAnalyzer(MvAnalyzerHandle parser);

void SetStreamAuxInfo(MvAnalyzerHandle parser, const StreamAuxInfo* info);

void SetVideoInfo(MvAnalyzerHandle parser, uint32_t width, uint32_t height);

MVStats ParseMvData(MvAnalyzerHandle parser, const uint8_t* buffer, size_t bufferSize);

void ResetMvAnalyzer(MvAnalyzerHandle parser);

size_t GetTargetMvDataSize(MvAnalyzerHandle parser);

#ifdef __cplusplus
}
#endif

#endif // MV_ANALYZER_C_API_H