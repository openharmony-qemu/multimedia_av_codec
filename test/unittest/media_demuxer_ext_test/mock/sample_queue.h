/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef SAMPLE_QUEUE_H
#define SAMPLE_QUEUE_H

#include <set>
#include <deque>
#include <memory>
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_define.h"
#include "plugin/plugin_time.h"


namespace OHOS {
namespace Media {
class SampleQueueCallback {
public:
    virtual ~SampleQueueCallback() = default;
    virtual Status OnSelectBitrateOk(int64_t startPts, uint32_t bitRate) = 0;
    virtual Status OnSampleQueueBufferAvailable(int32_t queueId) = 0;
    virtual Status OnSampleQueueBufferConsume(int32_t queueId) = 0;
};

enum class SelectBitrateStatus : uint32_t {
    NORMAL = 0, // has no selectbitrate commond
    READY_SWITCH, // with selectbitrate commond but not satisfy switch condition
    SWITCHING, // reach switch condition wait ResponseForSwitchDone then to convert NORMAL
};

class SampleQueue : public std::enable_shared_from_this<SampleQueue> {
public:
    static constexpr uint32_t MAX_SAMPLE_QUEUE_SIZE = 1;
    static constexpr uint32_t DEFAULT_SAMPLE_QUEUE_SIZE = 1;
    static constexpr uint32_t FD_SAMPLE_QUEUE_SIZE = 300;
    static constexpr uint32_t MAX_SAMPLE_BUFFER_CAP = 10 * 1024 * 1024;
    static constexpr uint32_t DEFAULT_VIDEO_SAMPLE_BUFFER_CAP = 1 * 1024 * 1024;
    static constexpr uint32_t DEFAULT_SAMPLE_BUFFER_CAP = 4 * 1024;
    static constexpr int64_t MIN_SWITCH_BITRATE_TIME_US = 3000000;
    static constexpr size_t MAX_BITRATE_SWITCH_WAIT_NUMBER = 1;
    struct Config {
        int32_t queueId_{0};
        std::string queueName_{""};
        uint32_t queueSize_{DEFAULT_SAMPLE_QUEUE_SIZE};
        uint32_t bufferCap_{DEFAULT_SAMPLE_BUFFER_CAP};
        bool isSupportBitrateSwitch_{false};
        bool isFlvLiveStream_{false};
        bool isNeedSetLarge_ {false};
    };
    SampleQueue() = default;
    virtual ~SampleQueue() = default;
    MOCK_METHOD(Status, Init, (const Config& config), ());
    MOCK_METHOD(Status, SetSampleQueueCallback, (std::shared_ptr<SampleQueueCallback> sampleQueueCb), ());
    
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetBufferQueueProducer, (), ());
    
    MOCK_METHOD(Status, RequestBuffer, (std::shared_ptr<AVBuffer>& sampleBuffer,
        const AVBufferConfig& config, int32_t timeoutMs), ());
    MOCK_METHOD(Status, PushBuffer, (std::shared_ptr<AVBuffer>& sampleBuffer, bool available), ());
    
    MOCK_METHOD(Status, AcquireCopyToDstBuffer, (std::shared_ptr<AVBuffer>& dstBuffer), ());
    MOCK_METHOD(Status, QuerySizeForNextAcquireBuffer, (size_t& size), ());
    
    MOCK_METHOD(Status, Clear, (), ());
    
    MOCK_METHOD(Status, ReadySwitchBitrate, (uint32_t bitrate), ());
    MOCK_METHOD(Status, UpdateLastEndSamplePts, (int64_t lastEndSamplePts), ());
    MOCK_METHOD(Status, ResponseForSwitchDone, (int64_t startPtsOnSwitch), ());
    
    MOCK_METHOD(void, OnBufferAvailable, (), ());
    MOCK_METHOD(void, OnBufferConsumer, (), ());
    MOCK_METHOD(uint64_t, GetCacheDuration, (), ());
    MOCK_METHOD(void, UpdateQueueId, (int32_t queueId), ());
    MOCK_METHOD(uint32_t, GetMemoryUsage, (), ());
    MOCK_METHOD(Status, AcquireBuffer, (std::shared_ptr<AVBuffer>& sampleBuffer), ());
    MOCK_METHOD(Status, ReleaseBuffer, (std::shared_ptr<AVBuffer>& sampleBuffer), ());
    MOCK_METHOD(Status, SetLargerQueueSize, (uint32_t size), ());
    MOCK_METHOD(bool, IsEmpty, (), ());
    MOCK_METHOD(Status, AddQueueSize, (uint32_t size), ());
    MOCK_METHOD(uint32_t, GetFilledBufferSize, (), ());
    MOCK_METHOD(Status, UpdateLastOutSamplePts, (int64_t), ());
    MOCK_METHOD(Status, UpdateLastEnterSamplePts, (int64_t), ());
    MOCK_METHOD(int64_t, GetLastEnterSamplePts, (), ());
    MOCK_METHOD(int64_t, GetLastOutSamplePts, (), ());
    MOCK_METHOD(Status, CopyBufferSlice, (std::shared_ptr<AVBuffer>&, std::shared_ptr<AVBuffer>&, int32_t), ());
    MOCK_METHOD(Status, RollbackBuffer, (std::shared_ptr<AVBuffer>&), ());
    MOCK_METHOD(uint64_t, NewGetCacheDuration, (), ());

    Config config_{};
    std::weak_ptr<SampleQueueCallback> sampleQueueCb_;

    std::shared_ptr<AVBufferQueue> sampleBufferQueue_;
    sptr<AVBufferQueueProducer> sampleBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> sampleBufferQueueConsumer_;

    int64_t lastEnterSamplePts_{Plugins::HST_TIME_NONE};
    int64_t lastOutSamplePts_{Plugins::HST_TIME_NONE};
    int64_t lastEndSamplePts_{Plugins::HST_TIME_NONE};
    int64_t startPtsToSwitch_{Plugins::HST_TIME_NONE};
    SelectBitrateStatus switchStatus_{SelectBitrateStatus::NORMAL};
    std::mutex statusMutex_;

    std::mutex ptsMutex_;
    std::set<int64_t> keyFramePtsSet_;

    std::list<std::shared_ptr<AVBuffer>> rollbackBufferQueue_;

    std::mutex waitListMutex_;
    std::deque<uint32_t> switchBitrateWaitList_;
    uint32_t nextSwitchBitrate_{0};
};
} // namespace Media
} // namespace OHOS
#endif // SAMPLE_QUEUE_H