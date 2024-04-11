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
#include "avcodec_errors.h"
#include "param_wrapper.h"
#include "avcodec_dump_utils.h"
#include "avcodec_log.h"
#include "avcodec_sysevent.h"
#ifdef HICOLLIE_ENABLE
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"
#endif

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecXCollie"};
    constexpr uint32_t DUMP_XCOLLIE_INDEX = 0x01000000;
    constexpr uint8_t DUMP_OFFSET_16 = 16;
    constexpr uint64_t COLLIE_INVALID_INDEX = 0;
}

namespace OHOS {
namespace MediaAVCodec {
AVCodecXCollie &AVCodecXCollie::GetInstance()
{
    static AVCodecXCollie instance;
    return instance;
}

void AVCodecXCollie::ServiceTimerCallback(void *data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    threadDeadlockCount_++;
    std::string name = data != nullptr ? (char *)data : "";

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

void AVCodecXCollie::ClientTimerCallback(void *data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string name = data != nullptr ? (char *)data : "";
    AVCODEC_LOGE("Client task %{public}s timeout", name.c_str());
    FaultEventWrite(FaultType::FAULT_TYPE_FREEZE, std::string("Client task ") +
        name + std::string(" timeout"), "Client");
}

int32_t AVCodecXCollie::Dump(int32_t fd)
{
    if (dfxDumper_.empty()) {
        return AVCS_ERR_OK;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string dumpString = "[AVCodec_XCollie]\n";
    AVCodecDumpControler dumpControler;
    uint32_t dumperIndex = 1;
    for (const auto &iter : dfxDumper_) {
        dumpControler.AddInfo(DUMP_XCOLLIE_INDEX + (dumperIndex << DUMP_OFFSET_16), "Timer_Name", iter.second.second);
        dumperIndex++;
    }
    dumpControler.GetDumpString(dumpString);
    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
        dumpString.clear();
    }
    return AVCS_ERR_OK;
}


uint64_t AVCodecXCollie::SetTimer(const std::string &name, bool isService, bool recovery, uint32_t timeout)
{
#ifdef HICOLLIE_ENABLE
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::function<void (void *)> func;
    if (isService) {
        func = [this](void *data) { this->ServiceTimerCallback(data); };
    } else {
        func = [this](void *data) { this->ClientTimerCallback(data); };
    }
    
    unsigned int flag = HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_NOOP;
    if (recovery) {
        flag |= HiviewDFX::XCOLLIE_FLAG_RECOVERY;
    }
    uint64_t tempIndex = dumperIndex_++;
    dfxDumper_.emplace(tempIndex, std::pair<int32_t, std::string>(HiviewDFX::INVALID_ID, name));
    int32_t id = HiviewDFX::XCollie::GetInstance().SetTimer(name,
        timeout, func, (void *)dfxDumper_[tempIndex].second.c_str(), flag);
    if (id == HiviewDFX::INVALID_ID) {
        auto it = dfxDumper_.find(tempIndex);
        if (it != dfxDumper_.end()) {
            dfxDumper_.erase(it);
            return COLLIE_INVALID_INDEX;
        }
    }
    dfxDumper_[tempIndex].first = id;
    return tempIndex;
#else
    (void)dumperIndex_;
    return COLLIE_INVALID_INDEX;
#endif
}

void AVCodecXCollie::CancelTimer(uint64_t index)
{
#ifdef HICOLLIE_ENABLE
    if (index == COLLIE_INVALID_INDEX) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dfxDumper_.find(index);
    if (it == dfxDumper_.end()) {
        return;
    }
    int32_t id = it->second.first;
    dfxDumper_.erase(it);
    return HiviewDFX::XCollie::GetInstance().CancelTimer(id);
#else
    (void)index;
    return;
#endif
}
} // namespace MediaAVCodec
} // namespace OHOS