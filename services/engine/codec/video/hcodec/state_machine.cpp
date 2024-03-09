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

#include "state_machine.h"
#include "hcodec_log.h"

namespace OHOS::MediaAVCodec {
void StateMachine::ChangeStateTo(const std::shared_ptr<State> &targetState)
{
    if (currState_ == targetState) {
        LOGI("already %s", currState_->stateName_.c_str());
        return;
    }
    std::shared_ptr<State> lastState = currState_;
    currState_ = targetState;
    if (lastState == nullptr) {
        LOGI("change to %s", currState_->stateName_.c_str());
    } else {
        LOGI("%s -> %s", lastState->stateName_.c_str(), currState_->stateName_.c_str());
        lastState->OnStateExited();
    }
    currState_->OnStateEntered();
}

void StateMachine::OnMsgReceived(const MsgInfo &info)
{
    currState_->OnMsgReceived(info);
}
}