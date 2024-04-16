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

#include "heap_memory_thread.h"
#include "common/native_mfmagic.h"

using namespace std;
namespace {
constexpr size_t MAX_HEAPNUM = 512;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
HeapMemoryThread::HeapMemoryThread()
{
    heapMemoryLoop_ = make_unique<thread>(&HeapMemoryThread::HeapMemoryLoop, this);

    std::string name = "heap_memory_thread";
    pthread_setname_np(heapMemoryLoop_->native_handle(), name.substr(0, 15).c_str()); // 15: max thread name
}

HeapMemoryThread::~HeapMemoryThread()
{
    isStopLoop_ = true;
    if (heapMemoryLoop_ != nullptr && heapMemoryLoop_->joinable()) {
        heapMemoryLoop_->join();
    }
}

void HeapMemoryThread::HeapMemoryLoop() const
{
    queue<uint8_t *> memoryList;
    while (!isStopLoop_) {
        uint8_t *memory = new uint8_t[sizeof(OH_AVMemory)];
        uint8_t *buffer = new uint8_t[sizeof(OH_AVBuffer)];
        uint8_t *format = new uint8_t[sizeof(OH_AVFormat)];
        memset_s(memory, sizeof(OH_AVMemory), 0, sizeof(OH_AVMemory));
        memset_s(buffer, sizeof(OH_AVBuffer), 0, sizeof(OH_AVBuffer));
        memset_s(format, sizeof(OH_AVFormat), 0, sizeof(OH_AVFormat));
        while (memoryList.size() >= MAX_HEAPNUM) {
            uint8_t *memoryFront = memoryList.front();
            delete memoryFront;
            memoryList.pop();
        }
        memoryList.push(memory);
        memoryList.push(buffer);
        memoryList.push(format);
    }
    while (!memoryList.empty()) {
        uint8_t *memoryFront = memoryList.front();
        delete memoryFront;
        memoryList.pop();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
