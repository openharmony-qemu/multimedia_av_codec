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

using namespace std;
using namespace std::chrono;
using namespace OHOS::MediaAVCodec;

string GetCodecName(bool isEncoder, const string& mime)
{
    vector<CapabilityData> caps;
    int32_t ret = GetHCodecCapabilityList(caps);
    if (ret != 0) {
        return {};
    }
    AVCodecType targetType = isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
    for (const CapabilityData& cap : caps) {
        if (cap.mimeType == mime && cap.codecType == targetType) {
            return cap.codecName;
        }
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

void CostRecorder::Print()
{
    for (const auto& one : records_) {
        printf("%s everage cost %u us\n", one.first.c_str(),
            static_cast<uint32_t>(one.second.totalCost / one.second.totalCnt));
    }
}