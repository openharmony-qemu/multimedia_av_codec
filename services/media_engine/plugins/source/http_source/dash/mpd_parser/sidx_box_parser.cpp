/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "DashSidxBoxParser"

#include "sidx_box_parser.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
    constexpr unsigned int SHIFT_NUM_2 = 2;
    constexpr unsigned int SHIFT_NUM_4 = 4;
    constexpr unsigned int SHIFT_NUM_8 = 8;
    constexpr unsigned int SHIFT_NUM_16 = 16;
    constexpr unsigned int SHIFT_NUM_24 = 24;
    constexpr unsigned int SHIFT_NUM_31 = 31;
    constexpr unsigned int SHIFT_NUM_32 = 32;
    constexpr unsigned int BUFF_INDEX_0 = 0;
    constexpr unsigned int BUFF_INDEX_1 = 1;
    constexpr unsigned int BUFF_INDEX_2 = 2;
    constexpr unsigned int BUFF_INDEX_3 = 3;
    constexpr unsigned int SIDX_REFERENCE_ITEM_SIZE = 12; // bytes of one referenced item
}

static inline unsigned short Get2Bytes(char *buffer, uint32_t &currPos);
static inline uint32_t Get4Bytes(char *buffer, uint32_t &currPos);
static inline int64_t Get8Bytes(char *buffer, uint32_t &currPos);
static inline uint32_t GetVersion(uint32_t vflag);
static inline uint32_t GetReferenceType(uint32_t typeAndSize);
static inline uint32_t GetReferenceSize(uint32_t typeAndSize);
static inline void ForwardBytes(uint32_t &currPos, uint32_t skipSize);
static inline uint32_t BemSource(char s, char i, char d, char x);

SidxBoxParser::~SidxBoxParser()
{
    MEDIA_LOG_D("SidxBoxParser deleter was called!!!!");
}

template <typename T> bool GetBytes(char *buffer, uint32_t &currPos, uint32_t streamSize, T &currentReturnValue)
{
    if constexpr (std::is_same_v<T, uint32_t>) {
        if (streamSize - currPos >= SHIFT_NUM_4) {
            currentReturnValue = Get4Bytes(buffer, currPos);
            return true;
        } else {
            return false;
        }
    } else if constexpr (std::is_same_v<T, int64_t>) {
        if (streamSize - currPos >= SHIFT_NUM_8) {
            currentReturnValue = Get8Bytes(buffer, currPos);
            return true;
        } else {
            return false;
        }
    } else if constexpr (std::is_same_v<T, unsigned short>) {
        if (streamSize - currPos >= SHIFT_NUM_2) {
            currentReturnValue = Get2Bytes(buffer, currPos);
            return true;
        } else {
            return false;
        }
    }
    return false;
}

int32_t SidxBoxParser::ParseSidxBox(char *bitStream, uint32_t streamSize, int64_t sidxEndOffset,
                                    DashList<std::shared_ptr<SubSegmentIndex>> &subSegIndexTable)
{
    const uint32_t BEM_SIDX = BemSource('s', 'i', 'd', 'x');
    uint32_t currPos = 0;

    while (streamSize > currPos && BASE_BOX_HEAD_SIZE < streamSize - currPos) {
        uint32_t boxType = 0;
        uint32_t dummyByte4 = 0;

        bool isByteReadSuccess = GetBytes<uint32_t>(bitStream, currPos, streamSize, dummyByte4);
        if (!isByteReadSuccess) return -1;
    
        isByteReadSuccess = GetBytes<uint32_t>(bitStream, currPos, streamSize, boxType);
        if (!isByteReadSuccess) return -1;

        if (boxType == BEM_SIDX) {
            MEDIA_LOG_D("it is a sidx box");
            if (!BuildSubSegmentIndexes(bitStream, streamSize, sidxEndOffset, subSegIndexTable, currPos)) {
                return -1;
            }
        } else {
            MEDIA_LOG_W("sdix box error box=(%c %c %c %c), typeSize="
                PUBLIC_LOG_D32, (boxType >> SHIFT_NUM_24) & 0x000000ff, (boxType >> SHIFT_NUM_16) & 0x000000ff,
                (boxType >> SHIFT_NUM_8) & 0x000000ff, boxType & 0x000000ff, currPos);
            return -1;
        }
    }

    return 0;
}

bool SidxBoxParser::BuildSubSegmentIndexes(char *bitStream, uint32_t streamSize, int64_t sidxEndOffset,
                                           DashList<std::shared_ptr<SubSegmentIndex>> &subSegIndexTable,
                                           uint32_t &currPos)
{
    uint32_t vFlag = 0;
    if (!GetBytes<uint32_t>(bitStream, currPos, streamSize, vFlag)) return false;

    uint32_t version = GetVersion(vFlag);

    uint32_t dummyByte4 = 0;
    if (!GetBytes<uint32_t>(bitStream, currPos, streamSize, dummyByte4)) return false;

    uint32_t timescale = 0;
    if (!GetBytes(bitStream, currPos, streamSize, timescale)) return false;

    int64_t firstOffset;
    int64_t dummyByte8 = 0;
    if (version == 0) {
        // skip earliestPresentationTime
        if (!GetBytes<uint32_t>(bitStream, currPos, streamSize, dummyByte4)) return false;
        
        uint32_t firstOffset32 = 0;
        if (!GetBytes<uint32_t>(bitStream, currPos, streamSize, firstOffset32)) return false;
        firstOffset = firstOffset32;
    } else {
        // skip earliestPresentationTime
        if (!GetBytes<int64_t>(bitStream, currPos, streamSize, dummyByte8)) return false;

        if (!GetBytes<int64_t>(bitStream, currPos, streamSize, firstOffset)) return false;
    }

    // In the file containing the Segment Index box, the anchor point for a Segment Index box is the first byte
    // after that box
    int64_t mediaSegOffset = firstOffset + sidxEndOffset + 1;

    // skip reserved
    ForwardBytes(currPos, SHIFT_NUM_2);

    uint32_t referenceCount = 0;
    unsigned short referenceCountShort = 0;
    if (!GetBytes<unsigned short>(bitStream, currPos, streamSize, referenceCountShort)) return false;
    referenceCount = referenceCountShort;

    if ((referenceCount * SIDX_REFERENCE_ITEM_SIZE + currPos) > streamSize) {
        MEDIA_LOG_W("sidx box reference count error: " PUBLIC_LOG_U32 ", currPos:" PUBLIC_LOG_U32 ", streamSize:"
            PUBLIC_LOG_U32, referenceCount, currPos, streamSize);
        return false;
    }

    MEDIA_LOG_D("sidx box reference count " PUBLIC_LOG_U32, referenceCount);
    for (uint32_t i = 0; i < referenceCount; i++) {
        std::shared_ptr<SubSegmentIndex> subSegIndex = std::make_shared<SubSegmentIndex>();
        uint32_t typeAndSize = 0;
        if (!GetBytes<uint32_t>(bitStream, currPos, streamSize, typeAndSize)) return false;
        subSegIndex->referenceType_ = static_cast<int32_t>(GetReferenceType(typeAndSize));
        subSegIndex->referencedSize_ = static_cast<int32_t>(GetReferenceSize(typeAndSize));
        subSegIndex->startPos_ = mediaSegOffset;
        subSegIndex->endPos_ = mediaSegOffset + subSegIndex->referencedSize_ - 1;
        mediaSegOffset = subSegIndex->endPos_ + 1;
        subSegIndex->duration_ = 0;
        if (!GetBytes<uint32_t>(bitStream, currPos, streamSize, subSegIndex->duration_)) return false;
        subSegIndex->timeScale_ = timescale;
        if (!GetBytes<uint32_t>(bitStream, currPos, streamSize, dummyByte4)) return false; // uint32_t sapInfo
        subSegIndexTable.push_back(subSegIndex);
    }
    return true;
}

unsigned short Get2Bytes(char *buffer, uint32_t &currPos)
{
    unsigned char *tmpBuff = reinterpret_cast<unsigned char *>(buffer + currPos);
    currPos += SHIFT_NUM_2;
    return (tmpBuff[BUFF_INDEX_0] << SHIFT_NUM_8) | tmpBuff[BUFF_INDEX_1];
}

uint32_t Get4Bytes(char *buffer, uint32_t &currPos)
{
    unsigned char *tmpBuff = reinterpret_cast<unsigned char *>(buffer + currPos);
    currPos += SHIFT_NUM_4;
    return (tmpBuff[BUFF_INDEX_0] << SHIFT_NUM_24) | (tmpBuff[BUFF_INDEX_1] << SHIFT_NUM_16) |
           (tmpBuff[BUFF_INDEX_2] << SHIFT_NUM_8) | tmpBuff[BUFF_INDEX_3];
}

int64_t Get8Bytes(char *buffer, uint32_t &currPos)
{
    uint64_t tmp = Get4Bytes(buffer, currPos);
    tmp = (tmp << SHIFT_NUM_32) | Get4Bytes(buffer, currPos);
    return static_cast<int64_t>(tmp);
}

uint32_t GetVersion(uint32_t vflag)
{
    return (vflag >> SHIFT_NUM_24) & 0xff;
}

uint32_t GetReferenceType(uint32_t typeAndSize)
{
    return (typeAndSize >> SHIFT_NUM_31) & 0xff;
}

uint32_t GetReferenceSize(uint32_t typeAndSize)
{
    return typeAndSize & 0x7fffffff;
}

void ForwardBytes(uint32_t &currPos, uint32_t skipSize)
{
    currPos += skipSize;
}

uint32_t BemSource(char s, char i, char d, char x)
{
    return (x) | ((d) << SHIFT_NUM_8) | ((i) << SHIFT_NUM_16) | ((s) << SHIFT_NUM_24);
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS