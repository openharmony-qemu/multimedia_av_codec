/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DASH_SEG_LIST_NODE_UNITTEST_H
#define DASH_SEG_LIST_NODE_UNITTEST_H

#include <memory>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "dash_seg_list_node.h"
#include "utils/time_utils.h"
#include "http_server_demo.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashSegListNodeUnittest : public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
protected:
    std::shared_ptr<DashSegListNode> dashPtr_ = nullptr;
};
}
}
}
}
#endif //DASH_SEG_LIST_NODE_UNITTEST_H
