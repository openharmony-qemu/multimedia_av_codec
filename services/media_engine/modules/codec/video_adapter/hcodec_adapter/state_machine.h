/*
  * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HCODEC_STATE_H
#define HCODEC_STATE_H

#include <memory>
#include <string>
#include <utility>
#include "msg_handle_loop.h"

namespace OHOS::MediaAVCodec {
class State {
public:
    explicit State(std::string stateName) : stateName_(std::move(stateName)) {}
    const std::string GetName() const { return stateName_; }

protected:
    virtual ~State() = default;
    virtual void OnStateEntered() {};
    virtual void OnStateExited() {};
    virtual void OnMsgReceived(const MsgInfo &info) = 0;

    friend class StateMachine;

    std::string stateName_;
};

class StateMachine : public MsgHandleLoop {
public:
    StateMachine() = default;

protected:
    virtual ~StateMachine() = default;
    void ChangeStateTo(const std::shared_ptr<State> &targetState);
    void OnMsgReceived(const MsgInfo &info) override;

    std::shared_ptr<State> currState_;
};
} // namespace OHOS::MediaAVCodec
#endif // HCODEC_STATE_H
