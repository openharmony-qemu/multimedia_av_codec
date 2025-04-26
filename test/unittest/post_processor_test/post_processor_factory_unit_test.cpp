/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "post_processor_factory_unit_test.h"

#include "gmock/gmock.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void PostProcessorFacotoryUnitTest::SetUpTestCase(void) {}

void PostProcessorFacotoryUnitTest::TearDownTestCase(void) {}

void PostProcessorFacotoryUnitTest::SetUp(void)
{
    factory_ = std::make_shared<VideoPostProcessorFactory>();
    EXPECT_NE(factory_, nullptr);
}

void PostProcessorFacotoryUnitTest::TearDown(void)
{
    factory_ = nullptr;
}

HWTEST_F(PostProcessorFacotoryUnitTest, PostProcessorFacotory_001, TestSize.Level1)
{
    std::shared_ptr<Meta> meta;
    EXPECT_EQ(factory_->CreateVideoPostProcessorPriv(VideoPostProcessorType::NONE), nullptr);
    EXPECT_EQ(factory_->IsPostProcessorSupportedPriv(VideoPostProcessorType::NONE, meta), false);
}

HWTEST_F(PostProcessorFacotoryUnitTest, PostProcessorFacotory_002, TestSize.Level1)
{
    std::shared_ptr<Meta> meta;
    VideoPostProcessorInstanceGenerator generator = nullptr;
    VideoPostProcessorSupportChecker checker = nullptr;
    factory_->RegisterPostProcessor<MockPostProcessor>(VideoPostProcessorType::NONE, generator);
    factory_->RegisterChecker(VideoPostProcessorType::NONE, checker);
    EXPECT_NE(factory_->CreateVideoPostProcessorPriv(VideoPostProcessorType::NONE), nullptr);
    EXPECT_EQ(factory_->IsPostProcessorSupportedPriv(VideoPostProcessorType::NONE, meta), true);
}

HWTEST_F(PostProcessorFacotoryUnitTest, PostProcessorFacotory_003, TestSize.Level1)
{
    std::shared_ptr<Meta> meta;
    VideoPostProcessorInstanceGenerator generator = []() -> std::shared_ptr<BaseVideoPostProcessor> {
        return std::make_shared<MockPostProcessor>();
    };
    VideoPostProcessorSupportChecker checker = [](const std::shared_ptr<Meta>& meta) -> bool {
        return true;
    };
    factory_->RegisterPostProcessor<MockPostProcessor>(VideoPostProcessorType::NONE, generator);
    factory_->RegisterChecker(VideoPostProcessorType::NONE, checker);
    EXPECT_NE(factory_->CreateVideoPostProcessorPriv(VideoPostProcessorType::NONE), nullptr);
    EXPECT_EQ(factory_->IsPostProcessorSupportedPriv(VideoPostProcessorType::NONE, meta), true);
}

HWTEST_F(PostProcessorFacotoryUnitTest, PostProcessorFacotory_004, TestSize.Level1)
{
    std::shared_ptr<Meta> meta;
    VideoPostProcessorInstanceGenerator generator = []() -> std::shared_ptr<BaseVideoPostProcessor> {
        return nullptr;
    };
    VideoPostProcessorSupportChecker checker = [](const std::shared_ptr<Meta>& meta) -> bool {
        return false;
    };
    factory_->RegisterPostProcessor<MockPostProcessor>(VideoPostProcessorType::NONE, generator);
    factory_->RegisterChecker(VideoPostProcessorType::NONE, checker);
    EXPECT_EQ(factory_->CreateVideoPostProcessorPriv(VideoPostProcessorType::NONE), nullptr);
    EXPECT_EQ(factory_->IsPostProcessorSupportedPriv(VideoPostProcessorType::NONE, meta), false);
}

}  // namespace Media
}  // namespace OHOS