/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef MEM_MGR_CLIENT_H
#define MEM_MGR_CLIENT_H

#include <mutex>
#include "mem_mgr_proxy.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class MemMgrClient : public NoCopyable {
public:
    MemMgrClient() noexcept;
    ~MemMgrClient();
    static MemMgrClient &GetInstance();
    bool IsAlived();
    int32_t NotifyProcessStatus(int32_t status);
    int32_t SetCritical(bool isKeyService);

private:
    pid_t pid_ = 0;
    bool isKeyService_ = false;
    sptr<Memory::MemMgrProxy> memMgrProxy_ = nullptr;
    std::mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEM_MGR_CLIENT_H
