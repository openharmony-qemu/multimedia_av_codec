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
 *
 * Description: API of message queue thread
 */

#ifndef MSGQUEUETHREAD_H
#define MSGQUEUETHREAD_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <map>
#include <list>
#include "param_bundle.h"

using MsgType = int32_t;
using MsgId = uint64_t;
struct MsgInfo {
    MsgType type;
    MsgId id;
    ParamSP param;
};

class MsgHandleLoop {
protected:
    MsgHandleLoop();
    ~MsgHandleLoop();
    void SendAsyncMsg(MsgType type, const ParamSP &msg, uint32_t delayUs = 0);
    bool SendSyncMsg(MsgType type, const ParamSP &msg, ParamSP &reply, uint32_t waitMs = 0);
    virtual void OnMsgReceived(const MsgInfo &info) = 0;
    void PostReply(MsgId id, const ParamSP &reply);
    void Stop();
    static constexpr MsgId ASYNC_MSG_ID = 0;

private:
    void MainLoop();
    MsgId GenerateMsgId();
    using TimeUs = int64_t;
    static TimeUs GetNowUs();

private:
    std::thread m_thread;
    std::mutex m_mtx;
    bool m_threadNeedStop = false;
    MsgId m_lastMsgId = 0;
    std::map<TimeUs, MsgInfo> m_msgQueue;  // msg will be sorted by timeUs
    std::condition_variable m_threadCond;

    std::mutex m_replyMtx;
    std::map<MsgId, ParamSP> m_replies;
    std::condition_variable m_replyCond;
};


#endif
