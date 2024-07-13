/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef MEDIA_PIPELINE_FILE_SOURCE_PLUGIN_UNIT_TEST_H
#define MEDIA_PIPELINE_FILE_SOURCE_PLUGIN_UNIT_TEST_H

#include "gtest/gtest.h"
#include "file_source_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileSource {
class FileSourceUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    std::shared_ptr<FileSourcePlugin> fileSourcePlugin_{nullptr};
};
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_FILE_SOURCE_PLUGIN_UNIT_TEST_H