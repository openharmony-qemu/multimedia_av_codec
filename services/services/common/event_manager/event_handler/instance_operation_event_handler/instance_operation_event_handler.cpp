/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "instance_operation_event_handler.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "instance_info.h"
#include "res_sched_client.h"
#include "event_info_extented_key.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceOperationEventHandler"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace ResourceSchedule;
using namespace ResourceSchedule::ResType;
InstanceOperationEventHandler &InstanceOperationEventHandler::GetInstance()
{
    static InstanceOperationEventHandler instance;
    return instance;
}

bool GetInstanceEncodeOperationMeta(const Media::Meta &meta, pid_t &pid, uid_t &uid, int32_t &instanceId,
                                    std::string &processName)
{
    meta.GetData(Media::Tag::AV_CODEC_CALLER_PID, pid);
    CHECK_AND_RETURN_RET_LOGW(pid > 0, false, "get pid failed.(%{public}d)", pid);
    meta.GetData(Media::Tag::AV_CODEC_CALLER_UID, uid);
    CHECK_AND_RETURN_RET_LOGW(uid >= 0, false, "get uid failed.(%{public}d)", uid);
    meta.GetData(Media::Tag::AV_CODEC_CALLER_PROCESS_NAME, processName);
    CHECK_AND_RETURN_RET_LOGW(processName != "", false, "get processName failed");
    meta.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId);
    CHECK_AND_RETURN_RET_LOGW(instanceId != INVALID_INSTANCE_ID, false, "get instance id failed.(%{public}d)",
                              instanceId);
    return true;
}

void InstanceOperationEventHandler::OnInstanceEncodeBegin(Media::Meta &meta)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = INVALID_PID;
    uid_t uid = -1;
    int32_t instanceId = INVALID_INSTANCE_ID;
    std::string processName = "";
    bool ret = GetInstanceEncodeOperationMeta(meta, pid, uid, instanceId, processName);
    CHECK_AND_RETURN_LOGW(ret, "get meta value failed");

    auto iter = encodeExecutingMap_.find(pid);
    if (iter == encodeExecutingMap_.end()) {
        std::unordered_map<std::string, std::string> payLoad = {{"pid", std::to_string(pid)},
                                                                {"uid", std::to_string(uid)},
                                                                {"bundleName", processName}};
        AVCODEC_LOGD("codec encode begin.pid:%{public}d,uid:%{public}d,bundleName:%{public}s", pid, uid,
                     processName.c_str());
        ResSchedClient::GetInstance().ReportData(RES_TYPE_CODEC_ENCODE_STATUS_CHANGED, CODEC_ENCODE_BEGIN, payLoad);
    }
    encodeExecutingMap_[pid].insert(instanceId);
}

void InstanceOperationEventHandler::OnInstanceEncodeEnd(Media::Meta &meta)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = INVALID_PID;
    uid_t uid = -1;
    int32_t instanceId = INVALID_INSTANCE_ID;
    std::string processName = "";
    bool ret = GetInstanceEncodeOperationMeta(meta, pid, uid, instanceId, processName);
    CHECK_AND_RETURN_LOGW(ret, "get meta value failed");

    auto iter = encodeExecutingMap_.find(pid);
    if (iter == encodeExecutingMap_.end()) {
        return;
    }
    std::set<int32_t> &instanceSet = iter->second;
    if (instanceSet.empty()) {
        encodeExecutingMap_.erase(iter);
        return;
    }
    instanceSet.erase(instanceId);
    if (instanceSet.empty()) {
        encodeExecutingMap_.erase(iter);
        std::unordered_map<std::string, std::string> payLoad = {{"pid", std::to_string(pid)},
                                                                {"uid", std::to_string(uid)},
                                                                {"bundleName", processName}};
        AVCODEC_LOGD("codec encode end.pid:%{public}d,uid:%{public}d,bundleName:%{public}s", pid, uid,
                     processName.c_str());
        ResSchedClient::GetInstance().ReportData(RES_TYPE_CODEC_ENCODE_STATUS_CHANGED, CODEC_ENCODE_END, payLoad);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS