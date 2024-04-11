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
#include "system_ability.h"
#define PRINT_HILOG
#include "unittest_log.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "SystemAbilityMock"};
std::mutex g_mutex;
std::weak_ptr<OHOS::SystemAbilityMock> g_mockObject;
} // namespace

namespace OHOS {
void SystemAbility::RegisterMock(std::shared_ptr<SystemAbilityMock> &mock)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("systemAbility:0x%" PRIXPTR, FAKE_POINTER(mock.get()));
    g_mockObject = mock;
}

SystemAbility::SystemAbility(bool runOnCreate)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("runOnCreate:%d", runOnCreate);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->SystemAbilityCtor(runOnCreate);
}

SystemAbility::SystemAbility(const int32_t serviceId, bool runOnCreate)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("serviceId:%d, runOnCreate:%d", serviceId, runOnCreate);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->SystemAbilityCtor(serviceId, runOnCreate);
}

SystemAbility::~SystemAbility()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->SystemAbilityDtor();
}

bool SystemAbility::Publish(SystemAbility *systemAbility)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("systemAbility:0x%" PRIXPTR, FAKE_POINTER(systemAbility));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, false, "mock object is nullptr");
    return mock->Publish(systemAbility);
}

void SystemAbility::OnDump()
{
    UNITTEST_INFO_LOG("");
}

void SystemAbility::OnStart()
{
    UNITTEST_INFO_LOG("");
}

void SystemAbility::OnStop()
{
    UNITTEST_INFO_LOG("");
}
} // namespace OHOS
