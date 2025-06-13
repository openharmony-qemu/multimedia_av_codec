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

#ifndef SAMPLE_QUEUE_UNIT_TEST_H
#define SAMPLE_QUEUE_UNIT_TEST_H


#include "gtest/gtest.h"
#include "buffer/avbuffer_queue.h"
#include "sample_queue.h"

namespace OHOS {
namespace Media {

class SampleQueueCallbackMock : public SampleQueueCallback {
public:
    explicit SampleQueueCallbackMock() {}
    virtual ~SampleQueueCallbackMock() = default;
    virtual Status OnSelectBitrateOk(int64_t startPts, uint32_t bitRate)
    {
        std::cout << "<===> OnSelectBitrateOk startPts=" << startPts << " ,bitRate=" << bitRate << std::endl;
        switchPtsVec_.push_back(startPts);
        return Status::OK;
    }
    virtual Status OnSampleQueueBufferAvailable(int32_t queueId)
    {
        OnAvailableSum_++;
        return Status::OK;
    }
    virtual Status OnSampleQueueBufferConsume(int32_t queueId)
    {
        OnConsumeSum_++;
        return Status::OK;
    }

    int32_t OnConsumeSum_ = 0;
    int32_t OnAvailableSum_ = 0;
    std::vector<uint64_t> switchPtsVec_;
};

class SampleQueueUnitTest : public std::enable_shared_from_this<SampleQueueUnitTest>,
                            public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void) override;

    void TearDown(void) override;

    Status InitLargeSampleQueue();

    Status InitNormalSampleQueue();

    Status SetCallback();

    Status UpdateBufferInfo(
        const std::shared_ptr<AVBuffer>& buffer, int64_t pts, size_t bufferSize, bool isKeyFrame = false);

    void ProducerLoop(int64_t frameCount, int64_t frameIntervalMs, size_t bufferSize);

    void ConsumerLoop(int64_t frameCount, int64_t frameIntervalMs);

    std::shared_ptr<SampleQueue> sampleQueue_;

    std::shared_ptr<std::thread> producerThread_ = nullptr;

    uint32_t pushFrames_ = 0;

    bool isProducerThreadStop_ = false;

    std::shared_ptr<std::thread> consumerThread_ = nullptr;

    std::shared_ptr<SampleQueueCallbackMock> sqCb_;
};
}
}

#endif