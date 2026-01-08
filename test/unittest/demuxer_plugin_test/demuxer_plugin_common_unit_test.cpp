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

#include <gtest/gtest.h>
#include <memory>
#include "avpacket_wrapper.h"
#include "avpacket_memory.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace testing::ext;

namespace {
constexpr int32_t TEST_DATA_SIZE = 1024;
constexpr int64_t TEST_PTS = 1000;
constexpr int64_t TEST_DTS = 900;
constexpr int TEST_STREAM_INDEX = 0;
constexpr int TEST_FLAGS = AV_PKT_FLAG_KEY;
constexpr int64_t TEST_DURATION = 40;
constexpr int64_t TEST_POS = 1024;
constexpr AVRational TEST_TIME_BASE = {1, 90000};
} // namespace

// ==================== AVPacketWrapper Tests ====================

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061301
 * @tc.desc: Test default constructor, verify AVPacket is allocated correctly
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061301, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    EXPECT_NE(pkt, nullptr);
    EXPECT_EQ(wrapper.GetSize(), 0);
    EXPECT_EQ(wrapper.GetData(), nullptr);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061302
 * @tc.desc: Test constructor from AVPacket*, verify ownership transfer
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061302, TestSize.Level1)
{
    AVPacket* pkt = av_packet_alloc();
    ASSERT_NE(pkt, nullptr);
    
    // Set some test data
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    pkt->pts = TEST_PTS;
    pkt->dts = TEST_DTS;
    pkt->stream_index = TEST_STREAM_INDEX;
    pkt->flags = TEST_FLAGS;
    pkt->duration = TEST_DURATION;
    pkt->pos = TEST_POS;
    pkt->time_base = TEST_TIME_BASE;
    
    AVPacketWrapper wrapper(pkt);
    EXPECT_EQ(wrapper.GetAVPacket(), pkt);
    EXPECT_EQ(wrapper.GetSize(), TEST_DATA_SIZE);
    EXPECT_EQ(wrapper.GetPts(), TEST_PTS);
    EXPECT_EQ(wrapper.GetDts(), TEST_DTS);
    
    // Verify ownership: pkt should be freed when wrapper is destroyed
    // We can't directly verify this, but we can verify the wrapper works correctly
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061303
 * @tc.desc: Test constructor with nullptr, verify null pointer handling
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061303, TestSize.Level1)
{
    AVPacketWrapper wrapper(nullptr);
    EXPECT_EQ(wrapper.GetAVPacket(), nullptr);
    EXPECT_EQ(wrapper.GetSize(), 0);
    EXPECT_EQ(wrapper.GetData(), nullptr);
    EXPECT_EQ(wrapper.GetPts(), AV_NOPTS_VALUE);
    EXPECT_EQ(wrapper.GetDts(), AV_NOPTS_VALUE);
    EXPECT_EQ(wrapper.GetStreamIndex(), -1);
    EXPECT_EQ(wrapper.GetFlags(), 0);
    EXPECT_EQ(wrapper.GetDuration(), 0);
    EXPECT_EQ(wrapper.GetPos(), -1);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061304
 * @tc.desc: Test GetPts() method with valid value and AV_NOPTS_VALUE
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061304, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test with AV_NOPTS_VALUE (default)
    EXPECT_EQ(wrapper.GetPts(), AV_NOPTS_VALUE);
    
    // Test with valid value
    pkt->pts = TEST_PTS;
    EXPECT_EQ(wrapper.GetPts(), TEST_PTS);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061305
 * @tc.desc: Test GetDts() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061305, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test with AV_NOPTS_VALUE (default)
    EXPECT_EQ(wrapper.GetDts(), AV_NOPTS_VALUE);
    
    // Test with valid value
    pkt->dts = TEST_DTS;
    EXPECT_EQ(wrapper.GetDts(), TEST_DTS);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061306
 * @tc.desc: Test GetSize() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061306, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test default size
    EXPECT_EQ(wrapper.GetSize(), 0);
    
    // Test with data
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    EXPECT_EQ(wrapper.GetSize(), TEST_DATA_SIZE);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061307
 * @tc.desc: Test GetData() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061307, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test default (no data)
    EXPECT_EQ(wrapper.GetData(), nullptr);
    
    // Test with data
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    uint8_t* data = pkt->data;
    EXPECT_EQ(wrapper.GetData(), data);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061308
 * @tc.desc: Test GetStreamIndex() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061308, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test default
    EXPECT_EQ(wrapper.GetStreamIndex(), 0);
    
    // Test with valid index
    pkt->stream_index = TEST_STREAM_INDEX;
    EXPECT_EQ(wrapper.GetStreamIndex(), TEST_STREAM_INDEX);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061309
 * @tc.desc: Test GetFlags() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061309, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test default
    EXPECT_EQ(wrapper.GetFlags(), 0);
    
    // Test with flags
    pkt->flags = TEST_FLAGS;
    EXPECT_EQ(wrapper.GetFlags(), TEST_FLAGS);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061310
 * @tc.desc: Test GetDuration() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061310, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test default
    EXPECT_EQ(wrapper.GetDuration(), 0);
    
    // Test with duration
    pkt->duration = TEST_DURATION;
    EXPECT_EQ(wrapper.GetDuration(), TEST_DURATION);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061311
 * @tc.desc: Test GetPos() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061311, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test default
    EXPECT_EQ(wrapper.GetPos(), -1);
    
    // Test with position
    pkt->pos = TEST_POS;
    EXPECT_EQ(wrapper.GetPos(), TEST_POS);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061312
 * @tc.desc: Test GetTimeBase() method
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061312, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    // Test default
    AVRational defaultTimeBase = wrapper.GetTimeBase();
    EXPECT_EQ(defaultTimeBase.num, 0);
    EXPECT_EQ(defaultTimeBase.den, 1);
    
    // Test with time base
    pkt->time_base = TEST_TIME_BASE;
    AVRational timeBase = wrapper.GetTimeBase();
    EXPECT_EQ(timeBase.num, TEST_TIME_BASE.num);
    EXPECT_EQ(timeBase.den, TEST_TIME_BASE.den);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061313
 * @tc.desc: Test accessing interfaces of empty packet, verify default values
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061313, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    // Empty packet should return default values
    EXPECT_EQ(wrapper.GetSize(), 0);
    EXPECT_EQ(wrapper.GetData(), nullptr);
    EXPECT_EQ(wrapper.GetPts(), AV_NOPTS_VALUE);
    EXPECT_EQ(wrapper.GetDts(), AV_NOPTS_VALUE);
    EXPECT_EQ(wrapper.GetStreamIndex(), 0);
    EXPECT_EQ(wrapper.GetFlags(), 0);
    EXPECT_EQ(wrapper.GetDuration(), 0);
    EXPECT_EQ(wrapper.GetPos(), -1);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061314
 * @tc.desc: Test move constructor, verify ownership transfer
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061314, TestSize.Level1)
{
    AVPacketWrapper wrapper1;
    AVPacket* pkt1 = wrapper1.GetAVPacket();
    ASSERT_NE(pkt1, nullptr);
    
    // Set some data
    int ret = av_grow_packet(pkt1, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    pkt1->pts = TEST_PTS;
    
    // Move construct
    AVPacketWrapper wrapper2(std::move(wrapper1));
    
    // Verify ownership transferred
    EXPECT_EQ(wrapper2.GetAVPacket(), pkt1);
    EXPECT_EQ(wrapper2.GetSize(), TEST_DATA_SIZE);
    EXPECT_EQ(wrapper2.GetPts(), TEST_PTS);
    
    // Original wrapper should have nullptr
    EXPECT_EQ(wrapper1.GetAVPacket(), nullptr);
    EXPECT_EQ(wrapper1.GetSize(), 0);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061315
 * @tc.desc: Test move assignment, verify ownership transfer and original object cleared
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061315, TestSize.Level1)
{
    AVPacketWrapper wrapper1;
    AVPacket* pkt1 = wrapper1.GetAVPacket();
    ASSERT_NE(pkt1, nullptr);
    
    int ret1 = av_grow_packet(pkt1, TEST_DATA_SIZE);
    ASSERT_GE(ret1, 0);
    pkt1->pts = TEST_PTS;
    
    AVPacketWrapper wrapper2;
    AVPacket* pkt2 = wrapper2.GetAVPacket();
    ASSERT_NE(pkt2, nullptr);
    
    // Move assign
    wrapper2 = std::move(wrapper1);
    
    // Verify ownership transferred
    EXPECT_EQ(wrapper2.GetAVPacket(), pkt1);
    EXPECT_EQ(wrapper2.GetSize(), TEST_DATA_SIZE);
    EXPECT_EQ(wrapper2.GetPts(), TEST_PTS);
    
    // Original wrapper should have nullptr
    EXPECT_EQ(wrapper1.GetAVPacket(), nullptr);
    EXPECT_EQ(wrapper1.GetSize(), 0);
    
    // pkt2 should have been freed
    EXPECT_NE(wrapper2.GetAVPacket(), pkt2);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketWrapper_061316
 * @tc.desc: Test self assignment move, verify no crash
 * @tc.type: FUNC
 */
HWTEST(AVPacketWrapperTest, AVDemuxer_Enhance_PacketWrapper_061316, TestSize.Level1)
{
    AVPacketWrapper wrapper;
    AVPacket* pkt = wrapper.GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    pkt->pts = TEST_PTS;
    
    // Self move assignment (use reference to avoid -Wself-move warning)
    AVPacketWrapper& ref = wrapper;
    wrapper = std::move(ref);
    
    // Should still be valid
    EXPECT_EQ(wrapper.GetAVPacket(), pkt);
    EXPECT_EQ(wrapper.GetSize(), TEST_DATA_SIZE);
    EXPECT_EQ(wrapper.GetPts(), TEST_PTS);
}

// ==================== AVPacketMemory Tests ====================

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061317
 * @tc.desc: Test constructor with valid AVPacketWrapper
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061317, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacket* pkt = wrapper->GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    uint8_t* data = pkt->data;
    
    AVPacketMemory memory(wrapper);
    
    EXPECT_EQ(memory.GetAddr(), data);
    EXPECT_EQ(memory.GetCapacity(), TEST_DATA_SIZE);
    EXPECT_EQ(memory.GetSize(), TEST_DATA_SIZE);
    EXPECT_EQ(memory.GetOffset(), 0);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061318
 * @tc.desc: Test constructor with nullptr wrapper
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061318, TestSize.Level1)
{
    std::shared_ptr<AVPacketWrapper> wrapper = nullptr;
    AVPacketMemory memory(wrapper);
    
    EXPECT_EQ(memory.GetAddr(), nullptr);
    EXPECT_EQ(memory.GetCapacity(), 0);
    EXPECT_EQ(memory.GetSize(), 0);
    EXPECT_EQ(memory.GetOffset(), 0);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061319
 * @tc.desc: Test constructor with empty packet wrapper
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061319, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacketMemory memory(wrapper);
    
    EXPECT_EQ(memory.GetAddr(), nullptr);
    EXPECT_EQ(memory.GetCapacity(), 0);
    EXPECT_EQ(memory.GetSize(), 0);
    EXPECT_EQ(memory.GetOffset(), 0);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061320
 * @tc.desc: Test GetMemoryType() returns CUSTOM_MEMORY
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061320, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacketMemory memory(wrapper);
    
    EXPECT_EQ(memory.GetMemoryType(), MemoryType::CUSTOM_MEMORY);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061321
 * @tc.desc: Test GetAddr() returns correct data address
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061321, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacket* pkt = wrapper->GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    uint8_t* data = pkt->data;
    
    AVPacketMemory memory(wrapper);
    EXPECT_EQ(memory.GetAddr(), data);
    
    // Test with null wrapper
    std::shared_ptr<AVPacketWrapper> nullWrapper = nullptr;
    AVPacketMemory nullMemory(nullWrapper);
    EXPECT_EQ(nullMemory.GetAddr(), nullptr);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061322
 * @tc.desc: Test GetCapacity() returns correct capacity
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061322, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacket* pkt = wrapper->GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    
    AVPacketMemory memory(wrapper);
    EXPECT_EQ(memory.GetCapacity(), TEST_DATA_SIZE);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061323
 * @tc.desc: Test GetSize() returns correct size
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061323, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacket* pkt = wrapper->GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    
    AVPacketMemory memory(wrapper);
    EXPECT_EQ(memory.GetSize(), TEST_DATA_SIZE);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061324
 * @tc.desc: Test GetOffset() returns correct offset
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061324, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacketMemory memory(wrapper);
    
    // Offset should always be 0 for AVPacketMemory
    EXPECT_EQ(memory.GetOffset(), 0);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061325
 * @tc.desc: Test Write() method returns 0 (write not supported)
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061325, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacketMemory memory(wrapper);
    
    uint8_t testData[10] = {0};
    int32_t result = memory.Write(testData, 10, 0);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061326
 * @tc.desc: Test destructor, verify no memory leak
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061326, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacket* pkt = wrapper->GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    
    {
        AVPacketMemory memory(wrapper);
        EXPECT_NE(memory.GetAddr(), nullptr);
    }
    
    // After memory destruction, wrapper should still be valid
    EXPECT_NE(wrapper->GetAVPacket(), nullptr);
    EXPECT_EQ(wrapper->GetSize(), TEST_DATA_SIZE);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061327
 * @tc.desc: Test multiple AVPacketMemory sharing the same wrapper
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061327, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacket* pkt = wrapper->GetAVPacket();
    ASSERT_NE(pkt, nullptr);
    
    int ret = av_grow_packet(pkt, TEST_DATA_SIZE);
    ASSERT_GE(ret, 0);
    uint8_t* data = pkt->data;
    
    AVPacketMemory memory1(wrapper);
    AVPacketMemory memory2(wrapper);
    
    // Both should point to the same data
    EXPECT_EQ(memory1.GetAddr(), data);
    EXPECT_EQ(memory2.GetAddr(), data);
    EXPECT_EQ(memory1.GetCapacity(), TEST_DATA_SIZE);
    EXPECT_EQ(memory2.GetCapacity(), TEST_DATA_SIZE);
    
    // Destroy one, the other should still work
    {
        AVPacketMemory memory3(wrapper);
        EXPECT_EQ(memory3.GetAddr(), data);
    }
    
    EXPECT_EQ(memory1.GetAddr(), data);
    EXPECT_EQ(memory2.GetAddr(), data);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061328
 * @tc.desc: Test all interface calls with nullptr wrapper
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061328, TestSize.Level1)
{
    std::shared_ptr<AVPacketWrapper> wrapper = nullptr;
    AVPacketMemory memory(wrapper);
    
    EXPECT_EQ(memory.GetMemoryType(), MemoryType::CUSTOM_MEMORY);
    EXPECT_EQ(memory.GetAddr(), nullptr);
    EXPECT_EQ(memory.GetCapacity(), 0);
    EXPECT_EQ(memory.GetSize(), 0);
    EXPECT_EQ(memory.GetOffset(), 0);
    
    uint8_t testData[10] = {0};
    EXPECT_EQ(memory.Write(testData, 10, 0), 0);
}

/**
 * @tc.name: AVDemuxer_Enhance_PacketMemory_061329
 * @tc.desc: Test all interface calls with empty packet
 * @tc.type: FUNC
 */
HWTEST(AVPacketMemoryTest, AVDemuxer_Enhance_PacketMemory_061329, TestSize.Level1)
{
    auto wrapper = std::make_shared<AVPacketWrapper>();
    AVPacketMemory memory(wrapper);
    
    EXPECT_EQ(memory.GetMemoryType(), MemoryType::CUSTOM_MEMORY);
    EXPECT_EQ(memory.GetAddr(), nullptr);
    EXPECT_EQ(memory.GetCapacity(), 0);
    EXPECT_EQ(memory.GetSize(), 0);
    EXPECT_EQ(memory.GetOffset(), 0);
    
    uint8_t testData[10] = {0};
    EXPECT_EQ(memory.Write(testData, 10, 0), 0);
}
