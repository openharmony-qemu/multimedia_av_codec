/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SYNC_MANAGER_UNITTEST_H
#define MEDIA_SYNC_MANAGER_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "media_sync_manager.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace OHOS::Media::Plugins;

class MediaSyncManagerUnitTest : public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
    
    std::shared_ptr<MediaSyncManager> manager_ = nullptr;
};

} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // MEDIA_SYNC_MANAGER_UNITTEST_H