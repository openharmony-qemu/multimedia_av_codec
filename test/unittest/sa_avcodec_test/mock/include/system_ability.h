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

#ifndef SYSTEM_ABILITY_MOCK_H
#define SYSTEM_ABILITY_MOCK_H

#include <gmock/gmock.h>
#include <memory>
#include "hilog/log.h"
#include "iremote_object.h"

#ifdef REGISTER_SYSTEM_ABILITY_BY_ID
#undef REGISTER_SYSTEM_ABILITY_BY_ID
#endif
#define REGISTER_SYSTEM_ABILITY_BY_ID(a, b, c)

#define DECLARE_SYSTEM_ABILITY(className)                                                                              \
public:                                                                                                                \
    virtual std::string GetClassName()                                                                                 \
    {                                                                                                                  \
        return #className;                                                                                             \
    }
namespace OHOS {
class SystemAbility;
class SystemAbilityMock {
public:
    SystemAbilityMock() = default;
    ~SystemAbilityMock() = default;

    MOCK_METHOD(void, SystemAbilityCtor, (const int32_t serviceId, bool runOnCreate));
    MOCK_METHOD(void, SystemAbilityCtor, (bool runOnCreate));
    MOCK_METHOD(void, SystemAbilityDtor, ());
    MOCK_METHOD(bool, Publish, (SystemAbility * systemAbility));
};

class SystemAbility {
public:
    static void RegisterMock(std::shared_ptr<SystemAbilityMock> &mock);

protected:
    explicit SystemAbility(bool runOnCreate = false);
    SystemAbility(const int32_t serviceId, bool runOnCreate = false);
    virtual ~SystemAbility();
    virtual bool Publish(SystemAbility *systemAbility);
    virtual void OnDump();
    virtual void OnStart();
    virtual void OnStop();
};
} // namespace OHOS
#endif // SYSTEM_ABILITY_MOCK_H
