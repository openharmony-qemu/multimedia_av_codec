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

#ifndef HISTREAMER_FOUNDATION_AVBUFFER_QUEUE_CONSUMER_H
#define HISTREAMER_FOUNDATION_AVBUFFER_QUEUE_CONSUMER_H

namespace OHOS {
namespace Media {
class AVBufferQueueConsumer : public RefBase {
public:
    ~AVBufferQueueConsumer() override = default;
    AVBufferQueueConsumer(const AVBufferQueueConsumer &) = delete;
    AVBufferQueueConsumer operator=(const AVBufferQueueConsumer &) = delete;

    MOCK_METHOD(uint32_t, GetQueueSize, (), ());
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), ());
    MOCK_METHOD(bool, IsBufferInQueue, (const std::shared_ptr<AVBuffer> &buffer), ());
    MOCK_METHOD(Status, AcquireBuffer, (std::shared_ptr<AVBuffer> & outBuffer), ());
    MOCK_METHOD(Status, ReleaseBuffer, (const std::shared_ptr<AVBuffer> &inBuffer), ());
    MOCK_METHOD(Status, AttachBuffer, (std::shared_ptr<AVBuffer> & inBuffer, bool isFilled), ());
    MOCK_METHOD(Status, DetachBuffer, (const std::shared_ptr<AVBuffer> &outBuffer), ());
    MOCK_METHOD(Status, SetBufferAvailableListener, (sptr<IConsumerListener> & listener), ());
    MOCK_METHOD(
        Status, SetQueueSizeAndAttachBuffer, (uint32_t size, std::shared_ptr<AVBuffer> &buffer, bool isFilled), ());

protected:
    AVBufferQueueConsumer() = default;
};
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_FOUNDATION_AVBUFFER_QUEUE_CONSUMER_H
