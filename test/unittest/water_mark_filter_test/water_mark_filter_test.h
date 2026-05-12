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
 
#ifndef WATER_MARK_FILTER_TEST_H
#define WATER_MARK_FILTER_TEST_H
 
#include "gtest/gtest.h"
#include "mock/filter_mock.h"
#include "mock/surface_mock.h"
#include "water_mark_filter.h"
#include "mock/avbuffer.h"
 
namespace OHOS {
namespace Media {
namespace Pipeline {
class WaterMarkFilterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
 
protected:
    std::shared_ptr<WaterMarkFilter> waterMarkFilter_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockEventReceiver> eventReceiver_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockFilterCallback> filterCallback_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockFilterLinkCallback> onLinkedResultCallback_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockFilter> nextFilter_{ nullptr };
    sptr<OHOS::MockSurface> mockSurface_{ nullptr };
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // WATER_MARK_FILTER_TEST_H