/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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
 
#ifndef SEI_PARSER_H
#define SEI_PARSER_H
 
#include <string>
#include <vector>
 
#include "surface_type.h"
#include "filter/filter.h"
#include "buffer/avbuffer_queue.h"
 
namespace OHOS {
namespace Media {
struct SeiPayloadInfo;
struct SeiPayloadInfoGroup;
 
class SeiParser {
public:
    SeiParser() = delete;
    ~SeiParser() = default;
    static Status ParseSeiPayload(std::string mimeType, std::shared_ptr<AVBuffer> buffer,
        const std::shared_ptr<SeiPayloadInfoGroup> &group, const std::vector<int32_t> &targetTypeVec);
 
private:
    static int32_t GetSeiTypeOrSize(uint8_t *&bodyPtr, const uint8_t *maxPtr);
    static Status FindNextSeiNaluPos(uint8_t *&startPtr, uint8_t *maxPtr, std::string mime);
    static Status ParseSeiRbsp(uint8_t *&bodyPtr, uint8_t *maxPtr, const std::shared_ptr<SeiPayloadInfoGroup> &group,
        const std::vector<int32_t> &targetTypeVec);
    static Status IteratorMsgCheckEmuBytes(
        const std::shared_ptr<AVBuffer> buffer, uint8_t *&payloadPtr, const uint8_t *maxPtr, const int32_t payloadSize);
};
 
struct SeiPayloadInfo {
    int32_t payloadType;
    std::shared_ptr<AVBuffer> payload;
};
 
struct SeiPayloadInfoGroup {
    int64_t playbackPosition;
    std::vector<SeiPayloadInfo> vec;
};
 
class SeiParserListener : public IBrokerListener {
public:
    explicit SeiParserListener(std::string mimeType, sptr<AVBufferQueueProducer> producer,
        std::shared_ptr<Pipeline::EventReceiver> eventReceiver)
        : mimeType_(mimeType),
          producer_(producer),
          eventReceiver_(eventReceiver)
    {
    }
 
    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
 
    void OnBufferFilled(std::shared_ptr<AVBuffer> &avBuffer) override;
 
    void SetPayloadTypeVec(const std::vector<int32_t> &vector)
    {
        payloadTypeVec_.clear();
        payloadTypeVec_.assign(vector.begin(), vector.end());
    }
 
private:
    std::string mimeType_{};
    wptr<AVBufferQueueProducer> producer_{};
    std::weak_ptr<Pipeline::EventReceiver> eventReceiver_{};
    std::vector<int32_t> payloadTypeVec_{ 5 };
};
}  // namespace Media
}  // namespace OHOS
#endif // SEI_PARSER_H