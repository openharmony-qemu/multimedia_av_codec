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
#ifndef HISTREAMER_AUDIO_SINK_FILTER_UNIT_TEST_H
#define HISTREAMER_AUDIO_SINK_FILTER_UNIT_TEST_H

#include <atomic>
#include "gtest/gtest.h"
#include "media_sync_manager.h"
#include "audio_sink.h"
#include "plugin/plugin_info.h"
#include "filter/filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class AudioSinkFilterUnitTest : public testing::Test {
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
    std::shared_ptr<AudioSinkFilter> audioSinkFilter_{ nullptr };
};
class MyFilterLinkCallback : public FilterLinkCallback {
public:
    ~MyFilterLinkCallback() = default;
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta)
    {
        return;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        return;
    }
    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        return;
    }
};
class MyEventReceiver : public EventReceiver {
public:
    ~MyEventReceiver() = default;
    void OnEvent(const Event& event)
    {
        return;
    }
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_AUDIO_SINK_FILTER_UNIT_TEST_H