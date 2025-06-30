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

#ifndef DEMUXER_FILTER_UNITTEST_H
#define DEMUXER_FILTER_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "demuxer_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {

class DemuxerFilterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    std::shared_ptr<DemuxerFilter> demuxerFilter_ { nullptr };
};

class MockFilterCallback : public FilterCallback {
public:
    MOCK_METHOD(Status, OnCallback,
        (const std::shared_ptr<Filter>& filter, FilterCallBackCommand cmd, StreamType outType), (override));
    MOCK_METHOD(void, NotifyRelease, (), (override));
};

} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // DEMUXER_FILTER_UNITTEST_H