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

#ifndef SUBTITLE_SINK_UNITTEST_H
#define SUBTITLE_SINK_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "mock/media_sync_manager.h"
#include "subtitle_sink.h"

namespace OHOS {
namespace Media {

class SubtitleSinkUnitTest : public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);

    std::shared_ptr<SubtitleSink> sink_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // SUBTITLE_SINK_UNITTEST_H