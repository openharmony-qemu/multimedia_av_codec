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
 * Description: implementation of message queue thread
 */

#include "msg_handle_loop.h"
#include <chrono>
#include <cinttypes>
#include "qos.h"
#include "hcodec_log.h"

using namespace std;

MsgHandleLoop::MsgHandleLoop()
{
    m_thread = thread(&MsgHandleLoop::MainLoop, this);
}

MsgHandleLoop::~MsgHandleLoop()
{
    Stop();
}

void MsgHandleLoop::Stop()
{
    {
        lock_guard<mutex> lock(m_mtx);
        m_threadNeedStop = true;
        m_threadCond.notify_all();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void MsgHandleLoop::SendAsyncMsg(MsgType type, const ParamSP &msg, uint32_t delayUs)
{
    lock_guard<mutex> lock(m_mtx);
    TimeUs nowUs = GetNowUs();
    TimeUs msgProcessTime = (delayUs > INT64_MAX - nowUs) ? INT64_MAX : (nowUs + static_cast<int64_t>(delayUs));
    if (m_msgQueue.find(msgProcessTime) != m_msgQueue.end()) {
        LOGW("DUPLICATIVE MSG TIMESTAMP!!!");
        msgProcessTime++;
    }
    m_msgQueue[msgProcessTime] = MsgInfo {type, ASYNC_MSG_ID, msg};
    m_threadCond.notify_all();
}

bool MsgHandleLoop::SendSyncMsg(MsgType type, const ParamSP &msg, ParamSP &reply, uint32_t waitMs)
{
    MsgId id = GenerateMsgId();
    {
        lock_guard<mutex> lock(m_mtx);
        TimeUs time = GetNowUs();
        if (m_msgQueue.find(time) != m_msgQueue.end()) {
            LOGW("DUPLICATIVE MSG TIMESTAMP!!!");
            time++;
        }
        m_msgQueue[time] = MsgInfo {type, id, msg};
        m_threadCond.notify_all();
    }

    unique_lock<mutex> lock(m_replyMtx);
    const auto pred = [this, id]() {
        return m_replies.find(id) != m_replies.end();
    };
    if (waitMs == 0) {
        m_replyCond.wait(lock, pred);
    } else {
        if (!m_replyCond.wait_for(lock, chrono::milliseconds(waitMs), pred)) {
            LOGE("type=%u wait reply timeout", type);
            return false;
        }
    }
    reply = m_replies[id];
    m_replies.erase(id);
    return true;
}

void MsgHandleLoop::PostReply(MsgId id, const ParamSP &reply)
{
    if (id == ASYNC_MSG_ID) {
        return;
    }
    lock_guard<mutex> lock(m_replyMtx);
    m_replies[id] = reply;
    m_replyCond.notify_all();
}

MsgId MsgHandleLoop::GenerateMsgId()
{
    lock_guard<mutex> lock(m_mtx);
    m_lastMsgId++;
    if (m_lastMsgId == ASYNC_MSG_ID) {
        m_lastMsgId++;
    }
    return m_lastMsgId;
}

void MsgHandleLoop::MainLoop()
{
    LOGI("increase thread priority");
    pthread_setname_np(pthread_self(), "OS_HCodecLoop");
    OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    while (true) {
        MsgInfo info;
        {
            unique_lock<mutex> lock(m_mtx);
            m_threadCond.wait(lock, [this] {
                return m_threadNeedStop || !m_msgQueue.empty();
            });
            if (m_threadNeedStop) {
                LOGI("stopped, remain %zu msg unprocessed", m_msgQueue.size());
                break;
            }
            TimeUs processUs = m_msgQueue.begin()->first;
            TimeUs nowUs = GetNowUs();
            if (processUs > nowUs) {
                m_threadCond.wait_for(lock, chrono::microseconds(processUs - nowUs));
                continue;
            }
            info = m_msgQueue.begin()->second;
            m_msgQueue.erase(m_msgQueue.begin());
        }
        OnMsgReceived(info);
    }
}

MsgHandleLoop::TimeUs MsgHandleLoop::GetNowUs()
{
    auto now = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count();
}

