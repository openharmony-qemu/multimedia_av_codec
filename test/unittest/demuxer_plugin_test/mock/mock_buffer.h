
/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_BUFFER_H
#define MOCK_BUFFER_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "common/status.h"
#include "plugin/plugin_buffer.h"


namespace OHOS {
namespace Media {
namespace Plugins {

class MockBufferInterface {
    public : 
    virtual ~MockBufferInterface() = default;
    virtual std::shared_ptr<Memory> GetMemory(uint32_t index = 0) = 0;
};

template <typename... T>
class MockBufferAdapter : public T... {
public:
    MOCK_METHOD(std::shared_ptr<Memory>, GetMemory, (uint32_t), (override));
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MOCK_BUFFER_H