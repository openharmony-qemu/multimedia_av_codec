/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <mutex>
#include "avcodec_log.h"
#include "mem_mgr_client.h"
#define PRINT_HILOG
#include "unittest_log.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "MemMgrClientMock"};
std::mutex g_mutex;
std::weak_ptr<OHOS::Memory::MemMgrClientMock> g_mockObject;
} // namespace
namespace OHOS {
namespace Memory {
using namespace OHOS::MediaAVCodec;
void MemMgrClient::RegisterMock(std::shared_ptr<MemMgrClientMock> &mock)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("MemMgrClient:0x%" PRIXPTR, FAKE_POINTER(mock.get()));
    g_mockObject = mock;
}

MemMgrClient::MemMgrClient()
{
    UNITTEST_INFO_LOG("");
}

MemMgrClient::~MemMgrClient()
{
    UNITTEST_INFO_LOG("");
}

MemMgrClient& MemMgrClient::GetInstance()
{
    static MemMgrClient instance;
    return instance;
}

int32_t MemMgrClient::NotifyProcessStatus(pid_t pid, int32_t start, int32_t status, int32_t saId)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("pid:%d, status:%d, start:%d, saId:%d", pid, status, start, saId);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->NotifyProcessStatus(pid, start, status, saId);
}

int32_t MemMgrClient::SetCritical(pid_t pid, bool flag, int32_t saId)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("pid:%d, flag:%d, saId:%d", pid, flag, saId);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetCritical(pid, flag, saId);
}
} // namespace Memory
} // namespace OHOS
