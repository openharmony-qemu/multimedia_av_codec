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

#ifndef AVCODEC_XCOLLIE_H
#define AVCODEC_XCOLLIE_H

#include <string>
#include <map>
#include <shared_mutex>
#include <functional>
#include <ctime>
#include <memory>
#include <atomic>

namespace OHOS {
namespace MediaAVCodec {
class AVCodecXCollie {
public:
    static AVCodecXCollie &GetInstance();
    int32_t SetTimer(const std::string &name, bool recovery, bool dumpLog, uint32_t timeout,
                     std::function<void(void *)> callback);
    int32_t SetInterfaceTimer(const std::string &name, bool isService, bool recovery, uint32_t timeout);
    void CancelTimer(int32_t timerId);
    int32_t Dump(int32_t fd);
    constexpr static uint32_t timerTimeout = 10;
private:
    struct TimerInfo {
        std::string name;
        std::time_t startTime;
        uint32_t timeout;

        TimerInfo(std::string argName, std::time_t argStartTime, uint32_t argTimeOut)
            : name(argName), startTime(argStartTime), timeout(argTimeOut) {}
    };

    AVCodecXCollie() = default;
    ~AVCodecXCollie();

    std::shared_mutex mutex_;
    std::map<int32_t, std::shared_ptr<TimerInfo>> dfxDumper_;
    std::atomic<bool> destroyed_{false};

// For interfacec timer
private:
    static void ServiceInterfaceTimerCallback(void *data);
    static void ClientInterfaceTimerCallback(void *data);
};

class AVCodecXcollieTimer {
public:
    AVCodecXcollieTimer(const std::string &name, bool recovery, bool dumpLog, uint32_t timeout,
                        std::function<void(void *)> callback)
    {
        index_ = AVCodecXCollie::GetInstance().SetTimer(name, recovery, dumpLog, timeout, callback);
    };

    ~AVCodecXcollieTimer()
    {
        AVCodecXCollie::GetInstance().CancelTimer(index_);
    }
private:
    int32_t index_ = 0;
};

class AVCodecXcollieInterfaceTimer {
public:
    AVCodecXcollieInterfaceTimer(const std::string &name, bool isService = true,
        bool recovery = false, uint32_t timeout = 30)
    {
        index_ = AVCodecXCollie::GetInstance().SetInterfaceTimer(name, isService, recovery, timeout);
    };

    ~AVCodecXcollieInterfaceTimer()
    {
        AVCodecXCollie::GetInstance().CancelTimer(index_);
    }
private:
    int32_t index_ = 0;
};

#define COLLIE_LISTEN(statement, args...)                               \
    {                                                                   \
        AVCodecXcollieInterfaceTimer xCollie(args);                     \
        statement;                                                      \
    }
#define CLIENT_COLLIE_LISTEN(statement, name)                           \
    {                                                                   \
        AVCodecXcollieInterfaceTimer xCollie(name, false, false, 30);   \
        statement;                                                      \
    }
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_XCOLLIE_H