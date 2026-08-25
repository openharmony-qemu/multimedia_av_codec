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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "avcodec_videocall.h"
#include "avcodec_errors.h"
#include <fuzzer/FuzzedDataProvider.h>

#define FUZZ_PROJECT_NAME "avcodecvideocall_fuzzer"

using namespace std;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
const int64_t ONE = 1;
const int64_t TWO = 2;
const int64_t THREE = 3;
const int64_t FOUR = 4;

class FuzzLowPowerCallback : public MediaCodecLowPowerConsumptionCallback {
public:
    void OnLowPowerConsumptionAvailable(std::shared_ptr<StrategyInfo> strategy) override
    {
        if (strategy != nullptr) {
            lastBundleName = strategy->bundleName;
            lastStrategyId = strategy->strategyId;
        }
    }
    std::string lastBundleName;
    int32_t lastStrategyId = -1;
};

bool AvcodecVideoCall001FuzzTest(FuzzedDataProvider *fdp)
{
    // Fuzz VideoCallNotifyPiPState with random bundleName and isFloatingWindow
    std::string bundleName = fdp->ConsumeRandomLengthString(256);
    bool isFloatingWindow = fdp->ConsumeBool();
    AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    return true;
}

bool AvcodecVideoCall002FuzzTest(FuzzedDataProvider *fdp)
{
    // Fuzz RegisterCallback with valid callback, then UnRegisterCallback
    auto callback = std::make_shared<FuzzLowPowerCallback>();
    int32_t ret = AVCodecVideoCall::RegisterCallback(callback);
    if (ret == AVCS_ERR_OK) {
        AVCodecVideoCall::UnRegisterCallback();
    }
    return true;
}

bool AvcodecVideoCall003FuzzTest(FuzzedDataProvider *fdp)
{
    // Fuzz RegisterCallback with nullptr
    AVCodecVideoCall::RegisterCallback(nullptr);
    return true;
}

bool AvcodecVideoCall004FuzzTest(FuzzedDataProvider *fdp)
{
    // Fuzz combined scenario: NotifyPiPState + Register + UnRegister in sequence
    std::string bundleName = fdp->ConsumeRandomLengthString(256);
    bool isFloatingWindow = fdp->ConsumeBool();
    AVCodecVideoCall::VideoCallNotifyPiPState(bundleName, isFloatingWindow);

    bool registerNullptr = fdp->ConsumeBool();
    if (registerNullptr) {
        AVCodecVideoCall::RegisterCallback(nullptr);
    } else {
        auto callback = std::make_shared<FuzzLowPowerCallback>();
        int32_t ret = AVCodecVideoCall::RegisterCallback(callback);
        if (ret == AVCS_ERR_OK) {
            // Fuzz UnRegisterCallback multiple times
            int32_t unregisterCount = fdp->ConsumeIntegralInRange<int32_t>(1, 5);
            for (int32_t i = 0; i < unregisterCount; i++) {
                AVCodecVideoCall::UnRegisterCallback();
            }
        }
    }
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    int32_t choose = fdp.ConsumeIntegralInRange<int32_t>(ONE, FOUR);
    if (choose == ONE) {
        OHOS::AvcodecVideoCall001FuzzTest(&fdp);
    } else if (choose == TWO) {
        OHOS::AvcodecVideoCall002FuzzTest(&fdp);
    } else if (choose == THREE) {
        OHOS::AvcodecVideoCall003FuzzTest(&fdp);
    } else {
        OHOS::AvcodecVideoCall004FuzzTest(&fdp);
    }
    return 0;
}
