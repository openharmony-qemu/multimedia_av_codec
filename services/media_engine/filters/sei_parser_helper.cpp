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
 
#include "sei_parser_helper.h"
 
#include <unordered_set>

#include "avcodec_trace.h"
#include "common/event.h"
#include "common/log.h"
#include "scope_guard.h"
#include "media_core.h"
#include "meta/format.h"
#include "plugin/plugin_time.h"
 
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "SeiParserHelper" };
 
constexpr uint16_t ANNEX_B_PREFIX_LEN = 4;
constexpr uint8_t SEI_UUID_LEN = 16;
constexpr int32_t SEI_PAYLOAD_SIZE_MAX = 1024 * 1024 - SEI_UUID_LEN;
constexpr int32_t KILO_BYTE = 1024;
 
const std::string TYPE_AVC = "video/avc";
const std::string TYPE_HEVC = "video/hevc";
 
constexpr uint16_t HEVC_SEI_TYPE_ONE = 0x4E;  // second bit to 7th bit at first Byte of nalu header is 39 0100 1110
constexpr uint16_t HEVC_SEI_TYPE_TWO = 0x50;  // second bit to 7th bit at first Byte of nalu header is 40 0101 0000
constexpr uint16_t HEVC_NAL_UNIT_TYPE_FLAG = (0x80 | 0x7E);  // 0x80 for forbidden bit, 0x7E for nalu type 1111 1110
constexpr uint16_t HEVC_SEI_HEAD_LEN = 2;
 
constexpr uint16_t AVC_SEI_TYPE = 0x06;                     // 4th bit to 8th bit at nalu header is 6
constexpr uint16_t AVC_NAL_UNIT_TYPE_FLAG = (0x80 | 0x1F);  // 0x80 for forbidden bit, 0x1F for nalu type 1001 1111
constexpr uint16_t AVC_SEI_HEAD_LEN = 1;

constexpr uint8_t EMULATION_GUIDE_0_LEN = 2;
constexpr uint8_t EMULATION_PREVENTION_CODE = 0X03;
}  // namespace
 
namespace OHOS {
namespace Media {
const std::map<std::string, HelperConstructFunc> SeiParserHelperFactory::HELPER_CONSTRUCTOR_MAP = {
    { TYPE_AVC,
        []() {
            return std::make_shared<AvcSeiParserHelper>();
        } },
    { TYPE_HEVC,
        []() {
            return std::make_shared<HevcSeiParserHelper>();
        } }
};

Status SeiParserHelper::ParseSeiPayload(
    std::shared_ptr<AVBuffer> buffer, std::shared_ptr<SeiPayloadInfoGroup> &group)
{
    FALSE_RETURN_V_MSG(!payloadTypeVec_.empty(), Status::ERROR_INVALID_DATA, "no listener type");
    FALSE_RETURN_V_MSG(buffer != nullptr, Status::ERROR_INVALID_DATA, "buffer is nullptr");
    FALSE_RETURN_V_MSG(buffer->memory_ != nullptr, Status::ERROR_INVALID_DATA, "memory is nullptr");
    MediaAVCodec::AVCodecTrace trace("ParseSeiPayload " + std::to_string(buffer->pts_) + " size " +
                                     std::to_string(buffer->memory_->GetSize() / KILO_BYTE));
 
    auto bufferParseRes = Status::ERROR_UNSUPPORTED_FORMAT;
    uint8_t seiNaluPrefixLen = ANNEX_B_PREFIX_LEN + 1 + 1 + SEI_UUID_LEN;
    uint8_t *naluStartPtr = buffer->memory_->GetAddr();
    uint8_t *maxPointer = naluStartPtr + buffer->memory_->GetSize();
    uint8_t *maxSeiPointer = maxPointer - seiNaluPrefixLen - 1;
    while (FindNextSeiNaluPos(naluStartPtr, maxSeiPointer)) {
        if (!group) {
            group = std::make_shared<SeiPayloadInfoGroup>();
        }
        auto naluParseRes = ParseSeiRbsp(naluStartPtr, maxPointer, group);
        bufferParseRes = (bufferParseRes == Status::OK ? bufferParseRes : naluParseRes);
    }
    if (group != nullptr && bufferParseRes == Status::OK) {
        group->playbackPosition = Plugins::Us2Ms(buffer->pts_);
    }
    return bufferParseRes;
}

void SeiParserHelper::SetPayloadTypeVec(const std::vector<int32_t> &vector)
{
    while (spinLock_.test_and_set(std::memory_order_acquire));
    payloadTypeVec_ = vector;
    spinLock_.clear(std::memory_order_release);
}
 
bool SeiParserHelper::FindNextSeiNaluPos(uint8_t *&startPtr, const uint8_t *const maxPtr)
{
    while (startPtr < maxPtr) {
        if (*startPtr & 0x00FE) {
            startPtr += 0x04;
            continue;
        }

        // find the first '1' after '0'
        if (*startPtr == 0) {
            startPtr++;
            continue;
        }

        // check if '1' after '000'
        if (*(reinterpret_cast<uint32_t *>(startPtr - 0x03)) != 0x01000000) {
            startPtr += 0x04;
            continue;
        }
        FALSE_CONTINUE_NOLOG(IsSeiNalu(++startPtr));
        return true;
    }
    return false;
}

bool AvcSeiParserHelper::IsSeiNalu(uint8_t *&headerPtr)
{
    uint8_t header = *headerPtr;
    auto naluType = header & AVC_NAL_UNIT_TYPE_FLAG;
    headerPtr += AVC_SEI_HEAD_LEN;
    if (naluType == AVC_SEI_TYPE) {
        return true;
    }
    return false;
}

bool HevcSeiParserHelper::IsSeiNalu(uint8_t *&headerPtr)
{
    uint8_t header = *headerPtr;
    auto naluType = header & HEVC_NAL_UNIT_TYPE_FLAG;
    headerPtr += HEVC_SEI_HEAD_LEN;
    if (naluType == HEVC_SEI_TYPE_ONE || naluType == HEVC_SEI_TYPE_TWO) {
        return true;
    }
    return false;
}
 
Status SeiParserHelper::ParseSeiRbsp(
    uint8_t *&bodyPtr, const uint8_t *const maxPtr, const std::shared_ptr<SeiPayloadInfoGroup> &group)
{
    Status unSupRetCode = Status::ERROR_UNSUPPORTED_FORMAT;
 
    // one sei nalu may has several sei message parts
    while (bodyPtr + SEI_UUID_LEN < maxPtr) {
        int32_t payloadType = GetSeiTypeOrSize(bodyPtr, maxPtr);
        int32_t payloadSize = GetSeiTypeOrSize(bodyPtr, maxPtr);
        FALSE_RETURN_V_NOLOG(
            payloadSize > 0 && payloadSize <= SEI_PAYLOAD_SIZE_MAX && bodyPtr + payloadSize < maxPtr, unSupRetCode);
 
        // if sei payload type is not 5, don't report, jump to next sei message
        while (spinLock_.test_and_set(std::memory_order_acquire));
        std::vector<int32_t> payloadTypeVec = payloadTypeVec_;
        spinLock_.clear(std::memory_order_release);
        if (std::find(payloadTypeVec.begin(), payloadTypeVec.end(), payloadType) == payloadTypeVec.end()) {
            auto res = FillTargetBuffer(nullptr, bodyPtr, maxPtr, payloadSize);
            FALSE_RETURN_V_NOLOG(res == Status::OK, res);
            continue;
        }
 
        AVBufferConfig config;
        config.size = payloadSize;
        config.memoryType = MemoryType::SHARED_MEMORY;
        auto avBuffer = AVBuffer::CreateAVBuffer(config);
        bool validMem = avBuffer != nullptr && avBuffer->memory_ != nullptr && avBuffer->memory_->GetAddr() != nullptr;
        FALSE_RETURN_V(validMem, Status::ERROR_NO_MEMORY);
        auto copyRes = FillTargetBuffer(avBuffer, bodyPtr, maxPtr, payloadSize);
        FALSE_RETURN_V_NOLOG(copyRes == Status::OK, copyRes);
 
        group->vec.push_back({ payloadType, avBuffer });
        unSupRetCode = Status::OK;
    }
    return unSupRetCode;
}
 
int32_t SeiParserHelper::GetSeiTypeOrSize(uint8_t *&bodyPtr, const uint8_t *const maxPtr)
{
    int32_t res = 0;
    while (*bodyPtr == 0xFF && bodyPtr + SEI_UUID_LEN < maxPtr) {
        res += 0xFF;
        bodyPtr++;
    }
    res += *bodyPtr++;
    return res;
}
 
Status SeiParserHelper::FillTargetBuffer(const std::shared_ptr<AVBuffer> buffer,
    uint8_t *&payloadPtr, const uint8_t *const maxPtr, const int32_t payloadSize)
{
    int32_t writtenSize = 0;
    uint8_t *targetPtr = (buffer == nullptr ? nullptr : buffer->memory_->GetAddr());
    for (int32_t zeroNum = 0; writtenSize < payloadSize && payloadPtr < maxPtr; payloadPtr++) {
        // in H.264 and H.265, 0x000000, 0x000001, 0x000002, 0x000003 will be replaced while encoding
        if (*payloadPtr == EMULATION_PREVENTION_CODE && zeroNum == EMULATION_GUIDE_0_LEN) {
            zeroNum = 0;
            continue;
        }
        zeroNum = *payloadPtr == 0 ? zeroNum + 1 : 0;
        if (targetPtr != nullptr) {
            targetPtr[writtenSize] = *payloadPtr;
        }
        writtenSize++;
    }
    FALSE_RETURN_V_MSG(
        writtenSize == payloadSize, Status::ERROR_UNSUPPORTED_FORMAT, "avalid data less than payloadSize");
    FALSE_RETURN_V_NOLOG(buffer != nullptr, Status::OK);
    buffer->memory_->SetSize(writtenSize);
    return Status::OK;
}

std::shared_ptr<SeiParserHelper> SeiParserHelperFactory::CreateHelper(const std::string &mimeType)
{
    auto constructor = HELPER_CONSTRUCTOR_MAP.find(mimeType);
    FALSE_RETURN_V_MSG(
        constructor != HELPER_CONSTRUCTOR_MAP.end(), nullptr, "unsupported mime %{public}s", mimeType.c_str());
    return (constructor->second)();
}

SeiParserListener::SeiParserListener(const std::string &mimeType, sptr<AVBufferQueueProducer> producer,
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver, bool isFlowLimited)
    : producer_(producer),
      eventReceiver_(eventReceiver),
      isFlowLimited_(isFlowLimited)
{
    seiParserHelper_ = SeiParserHelperFactory::CreateHelper(mimeType);
    FALSE_RETURN_MSG(seiParserHelper_ != nullptr, "Create SeiParserHelper failed for %{public}s", mimeType.c_str());

    sptr<IBrokerListener> tmpListener = this;
    producer_->RemoveBufferFilledListener(tmpListener);
    producer_->SetBufferFilledListener(tmpListener);
}
 
void SeiParserListener::OnBufferFilled(std::shared_ptr<AVBuffer> &avBuffer)
{
    FALSE_RETURN_MSG(avBuffer != nullptr, "avbuffer is nullptr");
    FALSE_RETURN_MSG(producer_ != nullptr, "report sei failed, buffer queue producer is nullptr");
    ON_SCOPE_EXIT(0)
    {
        producer_->ReturnBuffer(avBuffer, true);
    };
    FALSE_RETURN_NOLOG(seiParserHelper_ != nullptr);
    FALSE_RETURN_NOLOG(eventReceiver_ != nullptr);

    FlowLimit(avBuffer);
    std::shared_ptr<SeiPayloadInfoGroup> group = nullptr;
    auto res = seiParserHelper_->ParseSeiPayload(avBuffer, group);
    FALSE_RETURN_NOLOG(res == Status::OK);
 
    Format seiInfoFormat;
    bool fmtRes = false;
    fmtRes = seiInfoFormat.PutIntValue(Tag::AV_PLAYER_SEI_PLAYBACK_POSITION, group->playbackPosition);
    FALSE_RETURN_MSG(fmtRes, "sei fill format failed");
 
    std::vector<Format> vec;
    for (SeiPayloadInfo &payloadInfo : group->vec) {
        FALSE_RETURN(payloadInfo.payload != nullptr && payloadInfo.payload->memory_ != nullptr);
 
        Format tmpFormat;
        fmtRes = tmpFormat.PutBuffer(Tag::AV_PLAYER_SEI_PAYLOAD, payloadInfo.payload->memory_->GetAddr(),
            payloadInfo.payload->memory_->GetSize());
        FALSE_RETURN_MSG(fmtRes, "put payload buffer failed");
        fmtRes = tmpFormat.PutIntValue(Tag::AV_PLAYER_SEI_PAYLOAD_TYPE, payloadInfo.payloadType);
        FALSE_RETURN_MSG(fmtRes, "put payload type failed");
        vec.push_back(tmpFormat);
    }
    seiInfoFormat.PutFormatVector(Tag::AV_PLAYER_SEI_PLAYBACK_GROUP, vec);
    eventReceiver_->OnEvent({ "SeiParserHelper", EventType::EVENT_SEI_INFO, seiInfoFormat });
}

void SeiParserListener::FlowLimit(const std::shared_ptr<AVBuffer> &avBuffer)
{
    FALSE_RETURN(isFlowLimited_ && syncCenter_ != nullptr);
    MediaAVCodec::AVCodecTrace trace("ParseSeiPayload FlowLimit");

    if (startPts_ == 0) {
        startPts_ = avBuffer->pts_;
    }

    auto mediaTimeUs = syncCenter_->GetMediaTimeNow();
    auto diff = avBuffer->pts_ - startPts_ - mediaTimeUs;
    FALSE_RETURN_NOLOG(diff > 0);

    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait_for(lock, std::chrono::microseconds(diff), [this] () { return isInterruptNeeded_.load(); });
}

void SeiParserListener::SetPayloadTypeVec(const std::vector<int32_t> &vector)
{
    FALSE_RETURN_MSG(seiParserHelper_ != nullptr, "seiParserHelper is nullptr");
    seiParserHelper_->SetPayloadTypeVec(vector);
}

void SeiParserListener::OnInterrupted(bool isInterruptNeeded)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isInterruptNeeded_ = isInterruptNeeded;
    }
    cond_.notify_all();
}

int32_t SeiParserListener::SetSeiMessageCbStatus(
    bool status, const std::vector<int32_t> &payloadTypes)
{
    if (status) {
        payloadTypes_ = payloadTypes;
        SetPayloadTypeVec(payloadTypes_);
        return 0;
    }
    if (payloadTypes_.empty()) {
        payloadTypes_ = {};
        SetPayloadTypeVec(payloadTypes_);
        return 0;
    }
    payloadTypes_.erase(
        std::remove_if(payloadTypes_.begin(), payloadTypes_.end(), [&payloadTypes](int value) {
            return std::find(payloadTypes.begin(), payloadTypes.end(), value) != payloadTypes.end();
        }), payloadTypes_.end());
    SetPayloadTypeVec(payloadTypes_);
    return 0;
}
}  // namespace Media
}  // namespace OHOS