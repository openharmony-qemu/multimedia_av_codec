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

#include "state_machine.h"
#include <cstddef>

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

static constexpr const char* STATE_NAMES[]{
    "Disabled",
    "Configured",
    "Prepared",
    "Running",
    "Stopped"
};

State StateMachine::Get() const
{
    return state_.load();
}

void StateMachine::Set(State state)
{
    state_.store(state);
}

const char* StateMachine::Name() const
{
    return STATE_NAMES[static_cast<size_t>(state_.load())];
}

} // namespace PostProcessing
} // namespace MediaAVCodec
} // namespace OHOS