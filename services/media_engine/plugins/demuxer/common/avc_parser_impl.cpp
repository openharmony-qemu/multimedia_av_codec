/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <set>
#include "common/log.h"
#include "meta/media_types.h"
#include "avc_parser_impl.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "AvcParser"};
constexpr uint8_t START_CODE[] = {0x00, 0x00, 0x01};
constexpr int32_t SIZE_CODE_LEN = 4;
}

namespace OHOS {
namespace Media {
namespace Plugins {
AvcParserImpl::~AvcParserImpl()
{
    annexbExtraDataVec_.clear();
    annexbFrameVec_.clear();
}

bool AvcParserImpl::IsAnnexbFrame(const uint8_t *sample, int32_t size)
{
    FALSE_RETURN_V(size >= SIZE_CODE_LEN, false);
    FALSE_RETURN_V(sample != nullptr, false);
    auto *iter = std::search(sample, sample + SIZE_CODE_LEN, START_CODE, START_CODE + sizeof(START_CODE));
    if (iter == sample || (iter == sample + 1 && sample[0] == 0x00)) {
        return true;
    }
    return false;
}

void AvcParserImpl::WriteStartCode(std::vector<uint8_t> &vec)
{
    vec.emplace_back(0x00);
    vec.emplace_back(0x00);
    vec.emplace_back(0x00);
    vec.emplace_back(0x01);
}

bool AvcParserImpl::ConvertExtraDataToAnnexb(uint8_t *extraData, int32_t extraDataSize)
{
    annexbExtraDataVec_.clear();
    if (IsAnnexbFrame(extraData, extraDataSize)) {
        annexbExtraDataVec_.insert(annexbExtraDataVec_.end(), extraData, extraData + extraDataSize);
        return true;
    }
    std::vector<DataAddrInfo> addrInfoVec;
    std::shared_ptr<RbspContext> rbspContext = std::make_shared<RbspContext>(extraData, extraDataSize);
    FALSE_RETURN_V_MSG_E(rbspContext != nullptr, false, "rbspContext is nullptr");

    // configurationVersion, AVCProfileIndication, profile_compatibility, AVCLevelIndication, reserverd
    rbspContext->RbspSkipBits(0x26);
    naluLength_ = rbspContext->RbspGetBits(0x02) + 0x01;  // lengthSizeMinusOne
    rbspContext->RbspSkipBits(0x03);                      // reserverd
    uint8_t numOfSPS = rbspContext->RbspGetBits(0x05);    // numOfSequenceParameterSets
    for (int32_t i = 0; i < numOfSPS; ++i) {
        uint16_t naluLength = rbspContext->RbspGetBits<uint16_t, uint32_t>(0x10); // sequenceParameterSetsLength
        uint32_t copySize =
            static_cast<uint32_t>(extraDataSize) > naluLength ? naluLength : static_cast<uint32_t>(extraDataSize);
        dataAddrInfo_ = {rbspContext->GetBytesIndex(), copySize};
        rbspContext->RbspSkipBits(copySize * 0x08); // nalUnit
        addrInfoVec.emplace_back(dataAddrInfo_);
    }
    uint8_t numOfPPS = rbspContext->RbspGetBits(0x08);   // numOfPictureParameterSets
    for (int32_t i = 0; i < numOfPPS; ++i) {
        uint16_t naluLength = rbspContext->RbspGetBits<uint16_t, uint32_t>(0x10);  // pictureParameterSetsLength
        uint32_t copySize =
            static_cast<uint32_t>(extraDataSize) > naluLength ? naluLength : static_cast<uint32_t>(extraDataSize);
        dataAddrInfo_ = {rbspContext->GetBytesIndex(), copySize};
        rbspContext->RbspSkipBits(copySize * 0x08);  // nalUnit
        addrInfoVec.emplace_back(dataAddrInfo_);
    }
    for (auto iter : addrInfoVec) {
        WriteStartCode(annexbExtraDataVec_);
        FALSE_RETURN_V_MSG_E(
            static_cast<uint32_t>(iter.bytesIndex) + iter.bufferSize <= static_cast<uint32_t>(extraDataSize),
            false, "Error size");
        annexbExtraDataVec_.insert(
            annexbExtraDataVec_.end(), extraData + iter.bytesIndex, extraData + iter.bytesIndex + iter.bufferSize);
    }
    return true;
}

void AvcParserImpl::ConvertExtradata(uint8_t **packet, int32_t &packetSize)
{
    *packet = annexbExtraDataVec_.data();
    packetSize = static_cast<int32_t>(annexbExtraDataVec_.size());
}

bool AvcParserImpl::ParsePacket(
    std::vector<DataAddrInfo> &dataInfoVec, bool &gotIrap, const uint8_t *packet, int32_t packetSize)
{
    std::shared_ptr<RbspContext> rbspContext = std::make_shared<RbspContext>(packet, packetSize);
    FALSE_RETURN_V_MSG_E(rbspContext != nullptr, false, "rbspContext is nullptr");
    while (rbspContext->RbspGetLeftBytesNum() > 0) {
        uint32_t naluSize = 0;
        if (rbspContext->RbspGetLeftBytesNum() < naluLength_) {
            return false;
        }
        for (int32_t i = 0; i < naluLength_; ++i) {
            naluSize = (naluSize << 0x08) | rbspContext->RbspGetBits(0x08); // 前4位表示数据大小
        }
        if (naluSize < 0x02 || naluSize > rbspContext->RbspGetLeftBytesNum()) {
            return false;
        }
        int32_t naluType = (rbspContext->RbspGetBits(0x08, false)) & 0x1f; // false 读完后不跳转
        bool isIrap = naluType == AvcNalType::AVC_IDR_W_RADL;
        gotIrap = (gotIrap || isIrap) ? true : false;
        dataAddrInfo_ = {rbspContext->GetBytesIndex(), naluSize};
        rbspContext->RbspSkipBits(naluSize * 0x08);
        dataInfoVec.emplace_back(dataAddrInfo_);
    }
    return true;
}

void AvcParserImpl::ConvertPacketCore(uint8_t **packet, int32_t &packetSize)
{
    annexbFrameVec_.clear();
    std::vector<DataAddrInfo> addrInfoVec;
    bool gotIrap = false;
    bool ret = ParsePacket(addrInfoVec, gotIrap, *packet, packetSize);
    if (!ret) {
        MEDIA_LOG_E("ParsePacket failed");
        return;
    }
    // just add xps to begin of first frame.
    if (!sendXps_ && gotIrap) {
        annexbFrameVec_.insert(
            annexbFrameVec_.end(), annexbExtraDataVec_.data(), annexbExtraDataVec_.data() + annexbExtraDataVec_.size());
        sendXps_ = true;
    }
    for (auto iter : addrInfoVec) {
        if (iter.bufferSize > static_cast<uint32_t>(packetSize)) {
            return;
        }
        WriteStartCode(annexbFrameVec_);
        annexbFrameVec_.insert(
            annexbFrameVec_.end(), *packet + iter.bytesIndex, *packet + iter.bytesIndex + iter.bufferSize);
    }
    *packet = annexbFrameVec_.data();
    packetSize = static_cast<int32_t>(annexbFrameVec_.size());
}

void AvcParserImpl::ConvertPacketToAnnexb(
    uint8_t **hvccPacket, int32_t &hvccPacketSize, uint8_t *sideData, size_t sideDataSize, bool isExtradata)
{
    (void)sideData;
    (void)sideDataSize;
    if (!hvccPacket || !*hvccPacket) {
        return;
    }
    if (isExtradata) {
        ConvertExtradata(hvccPacket, hvccPacketSize);
    } else {
        ConvertPacketCore(hvccPacket, hvccPacketSize);
    }
}

void AvcParserImpl::ResetXPSSendStatus()
{
    sendXps_ = false;
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS