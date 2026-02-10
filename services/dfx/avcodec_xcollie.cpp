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

#include "avcodec_xcollie.h"
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#ifdef HICOLLIE_ENABLE
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"
#endif

#include "avcodec_errors.h"
#include "avcodec_dump_utils.h"
#include "avcodec_log.h"
#include "avcodec_sysevent.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecXCollie"};
    constexpr uint32_t DUMP_XCOLLIE_INDEX = 0x01'00'00'00;
    constexpr uint8_t DUMP_OFFSET_16 = 16;
    constexpr uint8_t DUMP_OFFSET_8 = 8;
    constexpr uint64_t COLLIE_INVALID_INDEX = 0;
}

namespace OHOS {
namespace MediaAVCodec {
static std::string GetTimeString(std::time_t time)
{
    std::stringstream ss;
    struct tm timeTm;
    ss << std::put_time(localtime_r(&time, &timeTm), "%F %T");
    return ss.str();
}

AVCodecXCollie &AVCodecXCollie::GetInstance()
{
    static AVCodecXCollie instance;
    return instance;
}

AVCodecXCollie::~AVCodecXCollie()
{
    destroyed_.store(true, std::memory_order_release);
}

int32_t AVCodecXCollie::SetTimer(const std::string &name, bool recovery, bool dumpLog, uint32_t timeout,
                                 std::function<void(void *)> callback)
{
    CHECK_AND_RETURN_RET_LOG(!destroyed_.load(std::memory_order_acquire), COLLIE_INVALID_INDEX,
                             "AVCodecXCollie is destructed");

    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!destroyed_.load(std::memory_order_relaxed), COLLIE_INVALID_INDEX,
                             "AVCodecXCollie is destructed");

    unsigned int flag = HiviewDFX::XCOLLIE_FLAG_NOOP;
    flag |= (recovery ? HiviewDFX::XCOLLIE_FLAG_RECOVERY : 0);
    flag |= (dumpLog ? HiviewDFX::XCOLLIE_FLAG_LOG : 0);

    auto timerInfo = std::make_shared<TimerInfo>(
        name, std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()), timeout);
    auto id = HiviewDFX::XCollie::GetInstance().SetTimer(
        name.data(), timeout, callback, reinterpret_cast<void *>(timerInfo.get()), flag);
    if (id != HiviewDFX::INVALID_ID) {
        dfxDumper_.emplace(id, timerInfo);
    }
    return id;
}

int32_t AVCodecXCollie::SetInterfaceTimer(const std::string &name, bool isService, bool recovery, uint32_t timeout)
{
#ifdef HICOLLIE_ENABLE
    std::function<void (void *)> func = isService ?
        [](void *data) { AVCodecXCollie::ServiceInterfaceTimerCallback(data); } :
        [](void *data) { AVCodecXCollie::ClientInterfaceTimerCallback(data); };

    return SetTimer(name, recovery, true, timeout, func);
#else
    return COLLIE_INVALID_INDEX;
#endif
}

void AVCodecXCollie::CancelTimer([[maybe_unused]]int32_t timerId)
{
#ifdef HICOLLIE_ENABLE
    if (timerId == COLLIE_INVALID_INDEX) {
        return;
    }
    CHECK_AND_RETURN_LOG(!destroyed_.load(std::memory_order_acquire), "AVCodecXCollie is destructed");
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(!destroyed_.load(std::memory_order_relaxed), "AVCodecXCollie is destructed");
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);

    auto it = dfxDumper_.find(timerId);
    if (it == dfxDumper_.end()) {
        return;
    }
    dfxDumper_.erase(it);
#endif
}

int32_t AVCodecXCollie::Dump(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(!destroyed_.load(std::memory_order_acquire), AVCS_ERR_OK, "AVCodecXCollie is destructed");
    using namespace std::string_literals;
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!destroyed_.load(std::memory_order_relaxed), AVCS_ERR_OK, "AVCodecXCollie is destructed");
    if (dfxDumper_.empty()) {
        return AVCS_ERR_OK;
    }

    std::string dumpString = "[AVCodec_XCollie]\n";
    AVCodecDumpControler dumpControler;
    uint32_t dumperIndex = 1;
    for (const auto &iter : dfxDumper_) {
        uint32_t timeInfoIndex = 1;
        auto titleIndex = DUMP_XCOLLIE_INDEX + (dumperIndex << DUMP_OFFSET_16);
        dumpControler.AddInfo(titleIndex, "Timer_"s + std::to_string(dumperIndex));
        dumpControler.AddInfo(titleIndex + (timeInfoIndex++ << DUMP_OFFSET_8), "TimerName", iter.second->name);
        dumpControler.AddInfo(titleIndex + (timeInfoIndex++ << DUMP_OFFSET_8),
            "StartTime", GetTimeString(iter.second->startTime).c_str());
        dumpControler.AddInfo(titleIndex + (timeInfoIndex++ << DUMP_OFFSET_8),
            "TimeLeft",
            std::to_string(iter.second->timeout + iter.second->startTime -
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));
        dumperIndex++;
    }

    dumpControler.GetDumpString(dumpString);
    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
        dumpString.clear();
    }
    return AVCS_ERR_OK;
}

void AVCodecXCollie::ServiceInterfaceTimerCallback(void *data)
{
    static uint32_t threadDeadlockCount_ = 0;
    threadDeadlockCount_++;
    std::string name = data != nullptr ? reinterpret_cast<TimerInfo *>(data)->name.c_str() : "";

    AVCODEC_LOGE("Service task %{public}s timeout", name.c_str());
    FaultEventWrite(FaultType::FAULT_TYPE_FREEZE, std::string("Service task ") +
        name + std::string(" timeout"), "Service");

    static constexpr uint32_t threshold = 1; // >= 1 Restart service
    if (threadDeadlockCount_ >= threshold) {
        FaultEventWrite(FaultType::FAULT_TYPE_FREEZE,
            "Process timeout, AVCodec service process exit.", "Service");
        AVCODEC_LOGF("Process timeout, AVCodec service process exit.");
        _exit(-1);
    }
}

void AVCodecXCollie::ClientInterfaceTimerCallback(void *data)
{
    std::string name = data != nullptr ? reinterpret_cast<TimerInfo *>(data)->name.c_str() : "";
    AVCODEC_LOGE("Client task %{public}s timeout", name.c_str());
    FaultEventWrite(FaultType::FAULT_TYPE_FREEZE, std::string("Client task ") +
        name + std::string(" timeout"), "Client");
}
} // namespace MediaAVCodec
} // namespace OHOS