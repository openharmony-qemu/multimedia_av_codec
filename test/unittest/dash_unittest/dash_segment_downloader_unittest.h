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

#ifndef DASH_SEGMENT_DOWNLOADER_UNITTEST_H
#define DASH_SEGMENT_DOWNLOADER_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "dash_segment_downloader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashSegmentDownloaderUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

private:
    std::shared_ptr<DashSegmentDownloader> segmentDownloader_ = nullptr;
};

class MockCallback : public Callback {
public:
    virtual ~MockCallback() = default;

    MOCK_METHOD(void, OnEvent, (const PluginEvent &event), (override));
    MOCK_METHOD(void, OnDfxEvent, (const PluginDfxEvent &event), (override));
    MOCK_METHOD(void, SetSelectBitRateFlag, (bool flag, uint32_t desBitRate), (override));
    MOCK_METHOD(bool, CanAutoSelectBitRate, (), (override));
};
} // HttpPlugin
} // Plugins
} // Media
} // OHOS
#endif //DASH_SEGMENT_DOWNLOADER_UNITTEST_H
