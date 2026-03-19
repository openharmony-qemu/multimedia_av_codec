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

#ifndef SEI_PARSER_HELPER_H
#define SEI_PARSER_HELPER_H

#include <string>
#include <vector>
#include "gmock/gmock.h"
#include "surface_type.h"
#include "filter/filter.h"
#include "buffer/avbuffer_queue.h"
#include "media_sync_manager.h"
#include "osal/task/autospinlock.h"

namespace OHOS {
namespace Media {
class SeiParserHelper;
struct SeiPayloadInfo;
struct SeiPayloadInfoGroup;

using HelperConstructFunc = std::function<std::shared_ptr<SeiParserHelper>()>;

class SeiParserHelper {
public:
    virtual ~SeiParserHelper() = default;

    Status ParseSeiPayload(const std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<SeiPayloadInfoGroup> &group);
    void SetPayloadTypeVec(const std::vector<int32_t> &vector);

protected:
    SeiParserHelper() = default;

private:

    static int32_t GetSeiTypeOrSize(uint8_t *&bodyPtr, const uint8_t *const maxPtr);
    static Status FillTargetBuffer(const std::shared_ptr<AVBuffer> buffer,
        uint8_t *&payloadPtr, const uint8_t *const maxPtr, const int32_t payloadSize);

    virtual bool IsSeiNalu(uint8_t *&headerPtr) = 0;
    bool FindNextSeiNaluPos(uint8_t *&startPtr, const uint8_t *const maxPtr);
    Status ParseSeiRbsp(
        uint8_t *&bodyPtr, const uint8_t *const maxPtr, const std::shared_ptr<SeiPayloadInfoGroup> &group);
    static uint32_t GetNaluStartSeq();

    std::vector<int32_t> payloadTypeVec_{};
    SpinLock spinLock_;
};

class AvcSeiParserHelper : public SeiParserHelper {
public:
    AvcSeiParserHelper() = default;

private:
    bool IsSeiNalu(uint8_t *&headerPtr) override;
};

class HevcSeiParserHelper : public SeiParserHelper {
public:
    HevcSeiParserHelper() = default;

private:
    bool IsSeiNalu(uint8_t *&headerPtr) override;
};

class SeiParserHelperFactory {
public:
    static std::shared_ptr<SeiParserHelper> CreateHelper(const std::string &mimeType);

private:
    static const std::map<std::string, HelperConstructFunc> HELPER_CONSTRUCTOR_MAP;
};

struct SeiPayloadInfo {
    int32_t payloadType;
    std::shared_ptr<AVBuffer> payload;
};

struct SeiPayloadInfoGroup {
    int64_t playbackPosition = 0;
    std::vector<SeiPayloadInfo> vec;
};

class SeiParserListener : public IBrokerListener {
public:
    explicit SeiParserListener(const std::string &mimeType, sptr<AVBufferQueueProducer> producer,
        std::shared_ptr<Pipeline::EventReceiver> eventReceiver, bool isFlowLimited)
        : producer_(producer),
          eventReceiver_(eventReceiver),
          isFlowLimited_(isFlowLimited) {};
    ~SeiParserListener() = default;
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
    MOCK_METHOD(void, OnBufferFilled, (std::shared_ptr<AVBuffer>& avBuffer), (override));
    MOCK_METHOD(void, SetPayloadTypeVec, (const std::vector<int32_t>& vector), ());
    MOCK_METHOD(void, OnInterrupted, (bool isInterruptNeeded), ());
    MOCK_METHOD(void, SetSyncCenter, (std::shared_ptr<Pipeline::IMediaSyncCenter> syncCenter), ());
    MOCK_METHOD(Status, SetSeiMessageCbStatus, (bool status, const std::vector<int32_t>& payloadTypes), ());
    MOCK_METHOD(void, FlowLimit, (const std::shared_ptr<AVBuffer> &avBuffer), ());
private:
    sptr<AVBufferQueueProducer> producer_{};
    std::shared_ptr<SeiParserHelper> seiParserHelper_{};
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_{};
    bool isFlowLimited_ { false };
    std::atomic<bool> isInterruptNeeded_ { false };
    std::mutex mutex_ {};
    std::condition_variable cond_ {};
    std::shared_ptr<Pipeline::IMediaSyncCenter> syncCenter_;
    int64_t startPts_ = 0;
    std::vector<int32_t> payloadTypes_{};
};
}  // namespace Media
}  // namespace OHOS
#endif  // SEI_PARSER_HELPER_H