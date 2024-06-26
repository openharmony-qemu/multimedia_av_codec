/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "sample_utils.h"
#include <chrono>
#include <thread>
#include "av_codec_sample_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "SampleUtils"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
void ThreadSleep(bool isValid, int32_t interval)
{
    CHECK_AND_RETURN(isValid && interval > 0);

    thread_local auto lastPushTime = std::chrono::system_clock::now();
    auto beforeSleepTime = std::chrono::system_clock::now();
    std::this_thread::sleep_until(lastPushTime + std::chrono::milliseconds(interval));
    lastPushTime = std::chrono::system_clock::now();

    AVCODEC_LOGV("Sleep time: %{public}2.2fms",
        static_cast<std::chrono::duration<double, std::milli>>(lastPushTime - beforeSleepTime).count());
}
} // Sample
} // MediaAVCodec
} // OHOS