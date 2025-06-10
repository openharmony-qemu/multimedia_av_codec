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

#ifndef CALLBACK_MOCK_H
#define CALLBACK_MOCK_H

#include <gmock/gmock.h>
#include "source.h"

namespace OHOS {
namespace Media {
class MockCallback : public CallbackImpl {
public:
    MockCallback() = default;
    ~MockCallback() = default;
    
    MOCK_METHOD(void, OnEvent, (const Plugins::PluginEvent &event), ());
    MOCK_METHOD(void, OnDfxEvent, (const Plugins::PluginDfxEvent &event), ());
    MOCK_METHOD(void, SetSelectBitRateFlag, (bool flag, uint32_t desBitRate), ());
    MOCK_METHOD(bool, CanAutoSelectBitRate, (), ());
};
}  // namespace Media
}  // namespace OHOS

#endif // CALLBACK_MOCK_H