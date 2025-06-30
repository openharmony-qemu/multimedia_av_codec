/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef FILTER_MOCK_H
#define FILTER_MOCK_H

#include <gmock/gmock.h>
#include "video_resize_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class MockEventReceiver : public EventReceiver {
public:
    MockEventReceiver() = default;
    ~MockEventReceiver() = default;
    
    MOCK_METHOD(void, OnEvent, (const Event& event), ());
    MOCK_METHOD(void, OnDfxEvent, (const DfxEvent& event), ());
    MOCK_METHOD(void, NotifyRelease, (), ());
    MOCK_METHOD(void, OnMemoryUsageEvent, (const DfxEvent& event), ());
};

class MockFilterCallback : public FilterCallback {
public:
    MockFilterCallback() = default;
    ~MockFilterCallback() = default;
    
    MOCK_METHOD(Status, OnCallback,
        (const std::shared_ptr<Filter>& filter, FilterCallBackCommand cmd, StreamType outType), ());
    MOCK_METHOD(void, NotifyRelease, (), ());
};

class MockFilterLinkCallback : public FilterLinkCallback {
public:
    MockFilterLinkCallback() = default;
    ~MockFilterLinkCallback() = default;
    
    MOCK_METHOD(void, OnLinkedResult, (const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta), ());
    MOCK_METHOD(void, OnUnlinkedResult, (std::shared_ptr<Meta>& meta), ());
    MOCK_METHOD(void, OnUpdatedResult, (std::shared_ptr<Meta>& meta), ());
};

class MockFilter : public Filter {
public:
    MockFilter(std::string name, FilterType type, bool asyncMode):Filter(name, type, asyncMode) {}
    ~MockFilter() = default;

    MOCK_METHOD(void, Init, (const std::shared_ptr<EventReceiver>& receiver,
        const std::shared_ptr<FilterCallback>& callback), ());
    MOCK_METHOD(void, Init, (const std::shared_ptr<EventReceiver>& receiver,
        const std::shared_ptr<FilterCallback>& callback, const std::shared_ptr<InterruptMonitor>& monitor), ());
    MOCK_METHOD(Status, HandleFormatChange, (std::shared_ptr<Meta>& meta), ());
    MOCK_METHOD(Status, DoSetPerfRecEnabled, (bool isPerfRecEnabled), ());
    MOCK_METHOD(Status, DoInitAfterLink, (), ());
    MOCK_METHOD(Status, DoPrepare, (), ());
    MOCK_METHOD(Status, DoStart, (), ());
    MOCK_METHOD(Status, DoPause, (), ());
    MOCK_METHOD(Status, DoPauseDragging, (), ());
    MOCK_METHOD(Status, DoPauseAudioAlign, (), ());
    MOCK_METHOD(Status, DoResume, (), ());
    MOCK_METHOD(Status, DoResumeDragging, (), ());
    MOCK_METHOD(Status, DoResumeAudioAlign, (), ());
    MOCK_METHOD(Status, DoStop, (), ());
    MOCK_METHOD(Status, DoFlush, (), ());
    MOCK_METHOD(Status, DoRelease, (), ());
    MOCK_METHOD(Status, DoFreeze, (), ());
    MOCK_METHOD(Status, DoUnFreeze, (), ());
    MOCK_METHOD(Status, DoPreroll, (), ());
    MOCK_METHOD(Status, DoWaitPrerollDone, (bool render), ());
    MOCK_METHOD(Status, DoSetPlayRange, (int64_t start, int64_t end), ());
    MOCK_METHOD(Status, DoProcessInputBuffer, (int recvArg, bool dropFrame), ());
    MOCK_METHOD(Status, DoProcessOutputBuffer, (int recvArg, bool dropFrame, bool byIdx,
        uint32_t idx, int64_t renderTime), ());
    MOCK_METHOD(void,  SetParameter, (const std::shared_ptr<Meta>& meta), ());
    MOCK_METHOD(void,  GetParameter, (std::shared_ptr<Meta>& meta), ());
    MOCK_METHOD(Status, LinkNext, (const std::shared_ptr<Filter>& nextFilter, StreamType outType), ());
    MOCK_METHOD(Status, UpdateNext, (const std::shared_ptr<Filter>& nextFilter, StreamType outType), ());
    MOCK_METHOD(Status, UnLinkNext, (const std::shared_ptr<Filter>& nextFilter, StreamType outType), ());
    FilterType GetFilterType()
    {
        return FilterType::AUDIO_DATA_SOURCE;
    }
    MOCK_METHOD(Status, OnLinked, (StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback), ());
    MOCK_METHOD(Status, OnUpdated, (StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback), ());
    MOCK_METHOD(Status, OnUnLinked, (StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback), ());
    MOCK_METHOD(Status, ClearAllNextFilters, (), ());
    MOCK_METHOD(Status, SetMuted, (bool isMuted), ());
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS

#endif // FILTER_MOCK_H