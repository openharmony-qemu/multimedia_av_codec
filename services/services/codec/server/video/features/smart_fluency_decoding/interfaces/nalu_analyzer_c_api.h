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

#ifndef NALU_ANALYZER_C_API_H
#define NALU_ANALYZER_C_API_H

#include "smart_fluency_decoding_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* NaluAnalyzerHandle;

typedef void (*NaluStreamAuxInfoCallback)(const StreamAuxInfo* info, void* userData);

NaluAnalyzerHandle CreateNaluAnalyzer(int32_t codecType);

void DestroyNaluAnalyzer(NaluAnalyzerHandle analyzer);

void SetCallback(NaluAnalyzerHandle analyzer, NaluStreamAuxInfoCallback cb, void* userData);

bool IsNonReferenceFrame(NaluAnalyzerHandle analyzer, const uint8_t* bufferAddr, uint32_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif // NALU_ANALYZER_C_API_H
