/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "test_utils.h"
#include "hcodec_api.h"
#include "hcodec_utils.h"
#include "hcodec_log.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace std::chrono;

string GetCodecName(bool isEncoder, const string& mime)
{
    vector<CapabilityData> caps;
    int32_t ret = GetHCodecCapabilityList(caps);
    if (ret != 0) {
        return {};
    }
    AVCodecType targetType = isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
    auto it = find_if(caps.begin(), caps.end(), [&mime, targetType](const CapabilityData& cap) {
        return cap.mimeType == mime && cap.codecType == targetType;
    });
    if (it != caps.end()) {
        return it->codecName;
    }
    return {};
}

CostRecorder& CostRecorder::Instance()
{
    static CostRecorder inst;
    return inst;
}

void CostRecorder::Clear()
{
    records_.clear();
}

void CostRecorder::Update(steady_clock::time_point begin, const string& apiName)
{
    auto cost = duration_cast<microseconds>(steady_clock::now() - begin).count();
    records_[apiName].totalCost += cost;
    records_[apiName].totalCnt++;
}

void CostRecorder::Print() const
{
    for (const auto& one : records_) {
        TLOGI("%s everage cost %.3f ms", one.first.c_str(),
              one.second.totalCost / US_TO_MS / one.second.totalCnt);
    }
}
}