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

#ifndef HISTREAMER_WRITE_BITRATE_CACULATOR_H
#define HISTREAMER_WRITE_BITRATE_CACULATOR_H

#include "common/log.h"
#include "osal/utils/steady_clock.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr uint32_t BYTES_TO_BIT = 8;
constexpr uint32_t SECOND_TO_MILLIONSECOND = 1000;

class WriteBitrateCaculator {
public:
    WriteBitrateCaculator() = default;
    ~WriteBitrateCaculator() = default;
    void StartClock()
    {
        if (isTiming_) {
            return;
        }
        isTiming_ = true;
        steadyClock_.Reset();
        startTime_ = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    }
    void StopClock()
    {
        if (!isTiming_) {
            return;
        }
        CaculateWriteBitrate();
        writeBytes_ = 0;
        isTiming_ = false;
        stopTime_ = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    }
    void ResetClock()
    {
        isTiming_ = true;
        CaculateWriteBitrate();
        writeBytes_ = 0;
        steadyClock_.Reset();
        startTime_ = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    }
    uint64_t GetWriteTime()
    {
        writeTime_ = stopTime_ > startTime_ ? stopTime_ - startTime_ : 0;
        return writeTime_;
    }
    void UpdateWriteBytes(size_t writeBytes)
    {
        writeBytes_ += static_cast<uint64_t>(writeBytes);
    }
    void CaculateWriteBitrate()
    {
        stopTime_ = steadyClock_.ElapsedMilliseconds();
        uint64_t writeTime = GetWriteTime();
        if (writeTime == 0) {
            writeBitrate_ = 0;
            return;
        }
        writeBitrate_ = static_cast<float>(writeBytes_ * BYTES_TO_BIT * SECOND_TO_MILLIONSECOND) / writeTime;
    }
    uint64_t GetWriteBitrate()
    {
        CaculateWriteBitrate();
        return writeBitrate_;
    }

private:
    SteadyClock steadyClock_;
    bool isTiming_ {false};
    uint64_t startTime_ {0};
    uint64_t stopTime_ {0};
    uint64_t writeTime_ {0};
    uint64_t writeBytes_ {0};
    uint64_t writeBitrate_ {0};
};

}
}
}
}
#endif