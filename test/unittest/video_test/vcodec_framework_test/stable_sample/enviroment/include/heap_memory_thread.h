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

#ifndef HEAP_MEMORY_THREAD_H
#define HEAP_MEMORY_THREAD_H
#include <atomic>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>

namespace OHOS {
namespace MediaAVCodec {
class HeapMemoryThread {
public:
    HeapMemoryThread();
    ~HeapMemoryThread();

private:
    void HeapMemoryLoop() const;
    std::unique_ptr<std::thread> heapMemoryLoop_ = nullptr;
    std::atomic<bool> isStopLoop_ = false;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // HEAP_MEMORY_THREAD_H