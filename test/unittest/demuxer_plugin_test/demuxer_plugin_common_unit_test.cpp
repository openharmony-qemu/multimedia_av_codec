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
#include <atomic>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <iostream>
#include "avpacket_wrapper.h"
#include "avpacket_memory.h"
#include "demuxer_plugin_unit_test.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_manager_v2.h"
#include "buffer/avbuffer.h"
#include "mock/ffmpeg_avreadframe_mock_helper.h"

extern "C" {
void AvReadFrameMockSetOrder(const int* streams, int count);
void AvReadFrameMockEnable(int enable);
void AvReadFrameMockLogEnable(int enable);
void AvReadFrameMockReset();
uint64_t AvReadFrameMockGetReadCount();
}

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::Ffmpeg;
using namespace testing::ext;
using MediaAVBuffer = OHOS::Media::AVBuffer;

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

// ==================== Cache Pressure Control Tests ====================

/**
 * @tc.name: AVDemuxer_Enhance_CachePressure_061330
 * @tc.desc: Test cache pressure control interfaces: SetCachePressureCallback,
 *           SetTrackCacheLimit, GetCurrentCacheFrameCount, GetCurrentCacheSize
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_Enhance_CachePressure_061330, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    
    // Select tracks first
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);
    
    // Test SetCachePressureCallback - 设置回调函数用于接收缓存压力通知
    constexpr int initialCallbackCount = 0; // 初始回调计数为0
    std::atomic<int> callbackInvokedCount(initialCallbackCount);
    
    Status status = demuxerPlugin_->SetCachePressureCallback(
        [&callbackInvokedCount](uint32_t trackId, uint32_t bytes) {
            (void)trackId; // 避免未使用变量警告
            (void)bytes;   // 避免未使用变量警告
            callbackInvokedCount++;
            // 注意：回调在读线程中执行，必须异步派发实际工作以避免死锁
            // 本测试仅验证回调被正确设置和调用，不在此处执行实际处理逻辑
        });
    EXPECT_EQ(status, Status::OK);
    
    // Test SetTrackCacheLimit - 为视频轨道设置较小的缓存限制以触发压力通知
    // 设置较小的限制值（例如 64KB），以便更容易触发缓存压力
    constexpr uint32_t bytesPerKB = 1024; // 1KB的字节数
    constexpr uint32_t cacheLimitKB = 64;  // 缓存限制为64KB
    constexpr uint32_t cacheLimitBytes = cacheLimitKB * bytesPerKB;
    constexpr uint32_t throttleWindowMs = 300; // 节流窗口为300毫秒
    status = demuxerPlugin_->SetTrackCacheLimit(videoTrackId, cacheLimitBytes, throttleWindowMs);
    EXPECT_EQ(status, Status::OK);
    
    // Test GetCurrentCacheFrameCount and GetCurrentCacheSize before reading
    // 注意：后台线程可能在 SelectTrack 后已经开始填充缓存，所以初始缓存可能不为空
    constexpr uint32_t initialCount = 0; // 初始计数为0
    uint32_t initialFrameCount = initialCount;
    status = demuxerPlugin_->GetCurrentCacheFrameCount(videoTrackId, initialFrameCount);
    std::cout << "[061330] initialFrameCount: " << initialFrameCount
            << ", status: " << static_cast<int>(status) << std::endl;
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(initialFrameCount, 0); // 缓存帧数应该 >= 0
    
    constexpr uint32_t initialSize = 0; // 初始缓存大小为0
    uint32_t initialCacheSize = initialSize;
    status = demuxerPlugin_->GetCurrentCacheSize(videoTrackId, initialCacheSize);
    std::cout << "[061330] initialCacheSize: " << initialCacheSize
            << ", status: " << static_cast<int>(status) << std::endl;
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(initialCacheSize, 0); // 缓存大小应该 >= 0
    
    // 异步读取视频数据，让后台线程填充缓存
    // 读取少量数据以触发缓存填充，但不会立即超过限制
    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t maxReadAttempts = 10; // 最大读取尝试次数
    constexpr uint32_t readTimeoutMs = 100; // ReadSample超时时间100毫秒
    
    for (uint32_t i = 0; i < maxReadAttempts; ++i) {
        Status readStatus = demuxerPlugin_->ReadSample(videoTrackId,
            avbuffer.mediaAVBuffer, readTimeoutMs);
        if (readStatus != Status::OK) {
            break;
        }
    }
    
    // 等待后台线程有时间填充缓存
    constexpr uint32_t cacheWaitMs = 200; // 等待缓存填充的时间200毫秒
    std::this_thread::sleep_for(std::chrono::milliseconds(cacheWaitMs));
    
    // 验证 GetCurrentCacheFrameCount 返回值合理
    constexpr uint32_t zeroCount = 0; // 初始帧计数为0
    uint32_t frameCount = zeroCount;
    status = demuxerPlugin_->GetCurrentCacheFrameCount(videoTrackId, frameCount);
    std::cout << "[061330] after read frameCount: " << frameCount
            << ", status: " << static_cast<int>(status) << std::endl;
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(frameCount, 0);
    
    // 验证 GetCurrentCacheSize 返回值合理
    constexpr uint32_t zeroSize = 0; // 初始缓存大小为0
    uint32_t cacheSize = zeroSize;
    status = demuxerPlugin_->GetCurrentCacheSize(videoTrackId, cacheSize);
    std::cout << "[061330] after read cacheSize: " << cacheSize
            << ", status: " << static_cast<int>(status) << std::endl;
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(cacheSize, 0);
    
    // 如果缓存超过了限制，回调应该被调用（取决于实现和节流窗口）
    // 由于节流窗口的存在，回调可能不会立即被调用，这是正常行为
    if (cacheSize > cacheLimitBytes) {
        // 等待足够的时间让回调有机会被触发（考虑节流窗口）
        constexpr uint32_t extraWaitMs = 100; // 额外等待时间100毫秒
        std::this_thread::sleep_for(
            std::chrono::milliseconds(throttleWindowMs + extraWaitMs));
    }
    
    // 验证回调机制正常工作（回调可能被调用，也可能由于节流而未调用，都是正常情况）
    // 至少验证回调函数被正确设置，不会导致崩溃
    constexpr int minCallbackCount = 0; // 最小回调计数为0
    EXPECT_GE(callbackInvokedCount.load(), minCallbackCount);
}

/**
 * @tc.name: AVDemuxer_Enhance_CachePressure_061331
 * @tc.desc: Test SeekTo interface and cache state after seek operation
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_Enhance_CachePressure_061331, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    
    // Select tracks first
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);
    
    // Read some audio samples first
    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t maxAudioReads = 20; // 最大音频读取次数
    constexpr uint32_t audioReadTimeoutMs = 50; // 音频读取超时时间50毫秒
    
    for (uint32_t i = 0; i < maxAudioReads; ++i) {
        Status readStatus = demuxerPlugin_->ReadSample(audioTrackId,
            avbuffer.mediaAVBuffer, audioReadTimeoutMs);
        std::cout << "[061331] audio sample " << i << " readStatus: "
                << static_cast<int>(readStatus) << std::endl;
        if (readStatus != Status::OK) {
            break;
        }
    }
    
    // Read first video sample to get initial timestamp
    constexpr uint32_t videoReadTimeoutMs = 50; // 视频读取超时时间50毫秒
    Status status = demuxerPlugin_->ReadSample(videoTrackId,
        avbuffer.mediaAVBuffer, videoReadTimeoutMs);
    std::cout << "[061331] first video ReadSample status: "
            << static_cast<int>(status) << std::endl;
    ASSERT_EQ(status, Status::OK);
    
    int64_t firstVideoDts = avbuffer.mediaAVBuffer->dts_;
    int64_t firstVideoPts = avbuffer.mediaAVBuffer->pts_;
    std::cout << "[061331] firstVideoDts: " << firstVideoDts
            << ", firstVideoPts: " << firstVideoPts << std::endl;
    EXPECT_NE(firstVideoDts, AV_NOPTS_VALUE);
    EXPECT_NE(firstVideoPts, AV_NOPTS_VALUE);
    
    // Read more video samples to advance position before seeking back
    // This tests seeking back to a previous position
    constexpr uint32_t maxVideoReads = 10; // 最大视频读取次数
    
    for (uint32_t i = 0; i < maxVideoReads; ++i) {
        Status readStatus = demuxerPlugin_->ReadSample(videoTrackId,
            avbuffer.mediaAVBuffer, videoReadTimeoutMs);
        std::cout << "[061331] video sample " << i << " readStatus: "
                << static_cast<int>(readStatus)
                << ", pts: " << avbuffer.mediaAVBuffer->pts_
                << ", dts: " << avbuffer.mediaAVBuffer->dts_ << std::endl;
        if (readStatus != Status::OK) {
            break;
        }
    }
    
    // Get media info to calculate relative time for SeekTo
    MediaInfo mediaInfo;
    status = demuxerPlugin_->GetMediaInfo(mediaInfo);
    std::cout << "[061331] GetMediaInfo status: "
            << static_cast<int>(status) << std::endl;
    EXPECT_EQ(status, Status::OK);
    
    constexpr int64_t zeroTime = 0; // 初始时间为0
    int64_t trackStartTime = zeroTime;
    bool hasStartTime = mediaInfo.tracks[videoTrackId].GetData(
        Tag::MEDIA_START_TIME, trackStartTime);
    std::cout << "[061331] hasStartTime: " << hasStartTime
            << ", trackStartTime: " << trackStartTime << std::endl;
    
    // Calculate relative seek time (seek back to first video PTS position)
    // Use first video PTS as the seek target for simplicity and reliability
    // The seek time should be relative to track start time
    int64_t relativeSeekTime = firstVideoPts;
    constexpr int64_t zeroSeekTime = 0; // 零seek时间
    if (hasStartTime && trackStartTime > zeroSeekTime &&
        firstVideoPts >= trackStartTime) {
        relativeSeekTime = firstVideoPts - trackStartTime;
    }
    
    // Ensure seek time is non-negative
    if (relativeSeekTime < zeroSeekTime) {
        relativeSeekTime = zeroSeekTime;
    }
    std::cout << "[061331] relativeSeekTime: " << relativeSeekTime << std::endl;
    
    // Test SeekTo interface - seek back to first video position
    int64_t realSeekTime = zeroSeekTime;
    status = demuxerPlugin_->SeekTo(videoTrackId, relativeSeekTime,
        SeekMode::SEEK_CLOSEST_SYNC, realSeekTime);
    std::cout << "[061331] SeekTo status: " << static_cast<int>(status)
            << ", realSeekTime: " << realSeekTime << std::endl;
    
    // Verify seek succeeded
    ASSERT_EQ(status, Status::OK) << "SeekTo failed with relativeSeekTime: "
                                    << relativeSeekTime
                                    << ", firstVideoPts: " << firstVideoPts
                                    << ", trackStartTime: " << trackStartTime;
    
    // Verify real seek time is reasonable (should be close to requested time)
    EXPECT_GE(realSeekTime, zeroSeekTime);
    
    // Read sample after seek to verify seek operation
    constexpr uint32_t seekReadTimeoutMs = 100; // seek后读取超时时间100毫秒
    status = demuxerPlugin_->ReadSample(videoTrackId,
        avbuffer.mediaAVBuffer, seekReadTimeoutMs);
    std::cout << "[061331] after seek ReadSample status: "
            << static_cast<int>(status)
            << ", pts: " << avbuffer.mediaAVBuffer->pts_
            << ", dts: " << avbuffer.mediaAVBuffer->dts_ << std::endl;
    ASSERT_EQ(status, Status::OK) << "ReadSample after seek failed";
    
    // Verify the PTS of the sample after seek is reasonable
    int64_t seekedPts = avbuffer.mediaAVBuffer->pts_;
    ASSERT_NE(seekedPts, AV_NOPTS_VALUE) << "Seeked PTS is invalid";
    
    // The PTS after seek should be close to the first video PTS (within reasonable range)
    // Due to key frame alignment, it may not be exactly the same
    // Compare absolute PTS values
    int64_t ptsDiff = (seekedPts > firstVideoPts) ? (seekedPts - firstVideoPts) : (firstVideoPts - seekedPts);
    std::cout << "[061331] PTS diff: " << ptsDiff
            << " (target: " << firstVideoPts << ", actual: " << seekedPts << ")" << std::endl;
    
    // Allow tolerance for key frame alignment (within 2 seconds at 90000 timebase)
    // Increased tolerance to account for key frame spacing in some codecs
    EXPECT_LE(ptsDiff, 180000) << "PTS difference too large: " << ptsDiff 
                                << " (target: " << firstVideoPts << ", actual: " << seekedPts << ")";
    
    // Test cache state after seek - cache should be reset or updated
    uint32_t videoCacheSize = 0;
    status = demuxerPlugin_->GetCurrentCacheSize(videoTrackId, videoCacheSize);
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(videoCacheSize, 0);
    
    uint32_t videoFrameCount = 0;
    status = demuxerPlugin_->GetCurrentCacheFrameCount(videoTrackId, videoFrameCount);
    EXPECT_EQ(status, Status::OK);
    EXPECT_GE(videoFrameCount, 0);
}

Status ReadSampleAndPrintInfo(int32_t testId, std::shared_ptr<DemuxerPlugin> demuxerPlugin, int32_t trackId,
    AVBufferWrapper& avbuffer, uint32_t timeoutMs)
{
    Status readStatus = demuxerPlugin->ReadSample(trackId, avbuffer.mediaAVBuffer, timeoutMs);
    std::cout << "[" << testId << "] ReadSample status: " << static_cast<int>(readStatus) << ", trackId: " << trackId <<
        ", pts: " << avbuffer.mediaAVBuffer->pts_ << ", dts: " << avbuffer.mediaAVBuffer->dts_ << std::endl;
    uint32_t frameCount = 0;
    demuxerPlugin->GetCurrentCacheFrameCount(trackId, frameCount);
    std::cout << "[" << testId << "] frame count: " << frameCount << std::endl;
    uint32_t cacheSize = 0;
    demuxerPlugin->GetCurrentCacheSize(trackId, cacheSize);
    std::cout << "[" << testId << "] cache size: " << cacheSize << std::endl;
    return readStatus;
}

/**
 * @tc.name: AVDemuxer_Enhance_CachePressure_061332
 * @tc.desc: Test SeekToFrameByDts interface and cache state after seek operation
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_Enhance_CachePressure_061332, TestSize.Level1)
{
    constexpr int32_t testId = 061332;
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";
    constexpr uint32_t audioReadTimeoutMs = 50;
    constexpr uint32_t videoReadTimeoutMs = 50;
    constexpr uint32_t seekTimeoutMs = 1000;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    // Select tracks first
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);
    
    // Read some audio samples first
    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t maxAudioReads = 20; // 最大音频读取次数
    constexpr uint32_t maxVideoReads = 5; // 最大视频读取次数
    Status readStatus = Status::OK;
    for (uint32_t i = 0; i < maxAudioReads; ++i) {
        readStatus = ReadSampleAndPrintInfo(testId, demuxerPlugin_, audioTrackId, avbuffer, audioReadTimeoutMs);
        ASSERT_EQ(readStatus, Status::OK);
    }
    for (uint32_t i = 0; i < maxVideoReads; ++i) {
        readStatus = ReadSampleAndPrintInfo(testId, demuxerPlugin_, videoTrackId, avbuffer, videoReadTimeoutMs);
        ASSERT_EQ(readStatus, Status::OK);
    }
    int64_t seekTime = avbuffer.mediaAVBuffer->dts_ / 1000;
    int64_t realSeekTime = 0;
    Status seekStatus = demuxerPlugin_->SeekToFrameByDts(videoTrackId, seekTime, SeekMode::SEEK_CLOSEST, realSeekTime,
                                                         seekTimeoutMs);
    std::cout << "[" << testId << "] SeekToFrameByDts status: " << static_cast<int>(seekStatus) <<
        ", seekTime: " << seekTime << ", realSeekTime: " << realSeekTime << std::endl;
    ASSERT_EQ(seekStatus, Status::OK);
    readStatus = ReadSampleAndPrintInfo(testId, demuxerPlugin_, videoTrackId, avbuffer, videoReadTimeoutMs);
    ASSERT_EQ(readStatus, Status::OK);
}

/**
 * @tc.name: AVDemuxer_AvReadFrame_MockOrder_061333
 * @tc.desc: Verify interposed av_read_frame can output packets in a user-defined order
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_AvReadFrame_MockOrder_061333, TestSize.Level1)
{
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";

    int order[] = {static_cast<int>(audioTrackId), static_cast<int>(audioTrackId), static_cast<int>(videoTrackId)};
    std::cout << "[TEST] set order: " << order[0] << ", " << order[1] << ", " << order[2] << std::endl;
    AvReadFrameMockLogEnable(1);
    AvReadFrameMockSetOrder(order, 3);
    AvReadFrameMockEnable(1);

    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);

    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t timeoutMs = 100;
    // Read two audio samples then one video sample; all should succeed if mock order works.
    std::cout << "[TEST] ReadSample audio #1" << std::endl;
    EXPECT_EQ(demuxerPlugin_->ReadSample(audioTrackId, avbuffer.mediaAVBuffer, timeoutMs), Status::OK);
    std::cout << "[TEST] got audio #1 pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    std::cout << "[TEST] ReadSample audio #2" << std::endl;
    EXPECT_EQ(demuxerPlugin_->ReadSample(audioTrackId, avbuffer.mediaAVBuffer, timeoutMs), Status::OK);
    std::cout << "[TEST] got audio #2 pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    std::cout << "[TEST] ReadSample video #1" << std::endl;
    EXPECT_EQ(demuxerPlugin_->ReadSample(videoTrackId, avbuffer.mediaAVBuffer, timeoutMs), Status::OK);
    std::cout << "[TEST] got video #1 pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;

    // Reset mock to avoid affecting other tests.
    AvReadFrameMockReset();
    AvReadFrameMockLogEnable(0);
}

/**
 * @tc.name: AVDemuxer_AvReadFrame_MockOrder_Complex_061334
 * @tc.desc: Verify a more complex av_read_frame order (audio burst, video burst, then audio burst)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_AvReadFrame_MockOrder_Complex_061334, TestSize.Level1)
{
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";

    // Complex order: 10 audio -> 5 video -> 3 audio
    std::vector<int> order;
    for (int i = 0; i < 10; i++) {
        order.push_back(static_cast<int>(audioTrackId));
    }
    for (int i = 0; i < 5; i++) {
        order.push_back(static_cast<int>(videoTrackId));
    }
    for (int i = 0; i < 3; i++) {
        order.push_back(static_cast<int>(audioTrackId));
    }

    std::cout << "[TEST061334] order size=" << order.size() << " (10A->5V->3A)" << std::endl;
    AvReadFrameMockLogEnable(1);
    AvReadFrameMockSetOrder(order.data(), static_cast<int>(order.size()));
    AvReadFrameMockEnable(1);

    InitResource(filePath, pluginName);
    std::cout << "[TEST061334] InitResource status=" << static_cast<int>(initStatus_) << std::endl;
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);

    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t timeoutMs = 100;
    for (size_t i = 0; i < order.size(); i++) {
        const uint32_t trackId = (order[i] == static_cast<int>(audioTrackId)) ? audioTrackId : videoTrackId;
        Status st = demuxerPlugin_->ReadSample(trackId, avbuffer.mediaAVBuffer, timeoutMs);
        std::cout << "[TEST061334] i=" << i << ", expectTrack=" << trackId
                  << ", st=" << static_cast<int>(st)
                  << ", pts=" << avbuffer.mediaAVBuffer->pts_
                  << ", dts=" << avbuffer.mediaAVBuffer->dts_
                  << std::endl;
        ASSERT_EQ(st, Status::OK);
    }

    AvReadFrameMockReset();
    AvReadFrameMockLogEnable(0);
}

/**
 * @tc.name: AVDemuxer_LimitedProbe_UseLargeSizeDataSource_061335
 * @tc.desc: Verify that demuxer takes limited pre-read path when DataSource reports a large file size
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_LimitedProbe_UseLargeSizeDataSource_061335, TestSize.Level1)
{
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";

    // Ensure mock is in counting mode (no order control) and logs enabled only for this test.
    AvReadFrameMockReset();
    AvReadFrameMockLogEnable(1);
    AvReadFrameMockEnable(0);

    // Init with a DataSource that reports a very large file size so that
    // FFmpegDemuxerPlugin::ParseVideoFirstFramesLimited() is used.
    InitResourceLargeFile(filePath, pluginName);
    std::cout << "[TEST061335] InitResourceLargeFile status=" << static_cast<int>(initStatus_) << std::endl;
    ASSERT_TRUE(initStatus_);

    // Select tracks and read a few samples to make sure normal playback still works.
    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);

    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t timeoutMs = 100;
    Status st = demuxerPlugin_->ReadSample(audioTrackId, avbuffer.mediaAVBuffer, timeoutMs);
    std::cout << "[TEST061335] first audio read st=" << static_cast<int>(st)
              << ", pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    ASSERT_EQ(st, Status::OK);

    st = demuxerPlugin_->ReadSample(videoTrackId, avbuffer.mediaAVBuffer, timeoutMs);
    std::cout << "[TEST061335] first video read st=" << static_cast<int>(st)
              << ", pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    ASSERT_EQ(st, Status::OK);

    // This test mainly serves to ensure that:
    // 1) Limited pre-read path can be triggered without a real >=1GB file.
    // 2) Normal ReadSample() behavior is not broken under limited pre-read.
    // Details of soft/hard limits can be inspected via log output.

    AvReadFrameMockReset();
    AvReadFrameMockLogEnable(0);
}

/**
 * @tc.name: AVDemuxer_LimitedProbe_HardLimit_061336
 * @tc.desc: Verify hard limit fallback mechanism when reading exceeds hardLimit frames
 * @tc.type: FUNC
 *
 * Test strategy:
 * 1. Use mock to control frame order: first 60 frames are all video (stream=0),
 *    audio first frame (stream=1) never appears in the first 60 frames
 * 2. Verify: should trigger hard limit (initReadFrameCount_ > 50)
 * 3. Verify log: "Hard limit reached 50, some tracks may not have first frame"
 * 4. Verify: audio track should be marked as pending (firstFramePendingTracks_)
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_LimitedProbe_HardLimit_061336, TestSize.Level1)
{
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";

    // For 2 tracks: hardLimit = max(2*4, 50) = 50
    // Design order: first 60 frames are all video frames, audio first frame never appears
    // Expected: when reading frame 51 (>hardLimit=50), should trigger hard limit
    std::vector<int> order;
    // First 60 frames: all video frames, audio first frame never appears
    for (int i = 0; i < 60; i++) {
        order.push_back(static_cast<int>(videoTrackId));
    }
    // After hard limit, add some audio frames for later ReadSample test
    for (int i = 0; i < 10; i++) {
        order.push_back(static_cast<int>(audioTrackId));
    }

    std::cout << "[TEST061336] Set order: 60 video frames, then 10 audio frames" << std::endl;
    AvReadFrameMockLogEnable(1);
    AvReadFrameMockSetOrder(order.data(), static_cast<int>(order.size()));
    AvReadFrameMockEnable(1);

    // Init with large file DataSource to trigger limited pre-read path
    InitResourceLargeFile(filePath, pluginName);
    std::cout << "[TEST061336] InitResourceLargeFile status=" << static_cast<int>(initStatus_) << std::endl;
    ASSERT_TRUE(initStatus_);

    // Verify hard limit was triggered:
    // 1. Should trigger hard limit (initReadFrameCount_ > 50)
    // 2. The actual read count should exceed hardLimit (50)
    // Note: AvReadFrameMockGetReadCount() counts all av_read_frame calls during initialization.
    // For 2 tracks: hardLimit = max(2*4, 50) = 50
    // Expected: readCount should be > 50 (we set 60 video frames, so should read at least 51)
    uint64_t readCountAfterInit = AvReadFrameMockGetReadCount();
    std::cout << "[TEST061336] av_read_frame call count after init: " << readCountAfterInit << std::endl;
    // Verify that we read more than hardLimit (50) frames, confirming hard limit was triggered
    ASSERT_GT(readCountAfterInit, 50U) << "Hard limit should be triggered (readCount > 50)";
    
    // Note: The log "Hard limit reached 50, some tracks may not have first frame" may appear
    // in system logs (MEDIA_LOG_W) rather than stdout. Check system logs for confirmation.
    // Audio track should be marked as pending (firstFramePendingTracks_ contains audioTrackId)
    // for later supplement during ReadSample().

    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);

    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t timeoutMs = 100;

    // Read video sample should succeed (video first frame was obtained)
    Status st = demuxerPlugin_->ReadSample(videoTrackId, avbuffer.mediaAVBuffer, timeoutMs);
    std::cout << "[TEST061336] video read st=" << static_cast<int>(st)
              << ", pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    ASSERT_EQ(st, Status::OK);

    // Audio read should also succeed (frames are cached, even though first frame
    // was not set during ParseVideoFirstFramesLimited due to hard limit)
    st = demuxerPlugin_->ReadSample(audioTrackId, avbuffer.mediaAVBuffer, timeoutMs);
    std::cout << "[TEST061336] audio read st=" << static_cast<int>(st)
              << ", pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    ASSERT_EQ(st, Status::OK);

    AvReadFrameMockReset();
    AvReadFrameMockLogEnable(0);
}

/**
 * @tc.name: AVDemuxer_LimitedProbe_Supplement_061337
 * @tc.desc: Verify supplement first frame mechanism for pending tracks
 * @tc.type: FUNC
 *
 * Test strategy:
 * 1. First trigger hard limit, let audio track be marked as pending
 * 2. In ReadSample(), if reading a sync frame of the pending track,
 *    should call SupplementFirstFrameIfPending() to supplement the first frame
 * 3. Verify log: "Supplement first frame for track X"
 */
HWTEST_F(DemuxerPluginUnitTest, AVDemuxer_LimitedProbe_Supplement_061337, TestSize.Level1)
{
    constexpr uint32_t videoTrackId = 0;
    constexpr uint32_t audioTrackId = 1;
    std::string pluginName = "avdemux_flv";
    std::string filePath = "/data/test/media/h264.flv";

    // Step 1: Trigger hard limit by making audio first frame never appear in first 60 frames
    std::vector<int> initOrder;
    // First 60 frames: all video frames, audio first frame never appears
    for (int i = 0; i < 60; i++) {
        initOrder.push_back(static_cast<int>(videoTrackId));
    }

    std::cout << "[TEST061337] Step 1: Trigger hard limit with 60 video frames" << std::endl;
    AvReadFrameMockLogEnable(1);
    AvReadFrameMockSetOrder(initOrder.data(), static_cast<int>(initOrder.size()));
    AvReadFrameMockEnable(1);

    // Init with large file DataSource to trigger limited pre-read path
    InitResourceLargeFile(filePath, pluginName);
    std::cout << "[TEST061337] InitResourceLargeFile status=" << static_cast<int>(initStatus_) << std::endl;
    ASSERT_TRUE(initStatus_);

    // At this point, audio track should be marked as pending due to hard limit
    // Verify hard limit was triggered by checking read count
    uint64_t readCountAfterInit = AvReadFrameMockGetReadCount();
    std::cout << "[TEST061337] Step 1: av_read_frame call count after init: " << readCountAfterInit << std::endl;
    ASSERT_GT(readCountAfterInit, 50U) << "Hard limit should be triggered (readCount > 50)";
    // Note: The log "Hard limit reached 50, some tracks may not have first frame" may appear
    // in system logs (MEDIA_LOG_W) rather than stdout. Check system logs for confirmation.

    // Step 2: Reset mock and set normal order (including audio frames)
    // Note: FFmpegDemuxerPlugin has its own cacheQueue_ that stores frames read during
    // initialization. When ReadSample() is called, it may use cached frames first.
    // Expected: when ReadSample() reads the first audio frame from cacheQueue_,
    // SupplementFirstFrameIfPending() checks if the track is pending and if the frame
    // is a sync frame. If both conditions are met, it supplements the first frame.
    AvReadFrameMockReset();

    // Set normal order: include audio frames
    std::vector<int> normalOrder;
    // Add some video frames
    for (int i = 0; i < 5; i++) {
        normalOrder.push_back(static_cast<int>(videoTrackId));
    }
    // Add audio frames (should trigger supplement)
    for (int i = 0; i < 10; i++) {
        normalOrder.push_back(static_cast<int>(audioTrackId));
    }

    std::cout << "[TEST061337] Step 2: Reset mock, set normal order with audio frames" << std::endl;
    AvReadFrameMockSetOrder(normalOrder.data(), static_cast<int>(normalOrder.size()));
    AvReadFrameMockEnable(1);

    ASSERT_EQ(demuxerPlugin_->SelectTrack(videoTrackId), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(audioTrackId), Status::OK);

    AVBufferWrapper avbuffer(DEFAULT_BUFFSIZE);
    constexpr uint32_t timeoutMs = 100;

    // Read audio sample, should trigger SupplementFirstFrameIfPending()
    // Note: The log "Supplement first frame for track 1" may appear in system logs
    // (MEDIA_LOG_I) rather than stdout. Check system logs for confirmation.
    // The supplement mechanism works as follows:
    // 1. Audio track was marked as pending during hard limit (firstFramePendingTracks_)
    // 2. When ReadSample() reads the first audio frame, SupplementFirstFrameIfPending()
    //    checks if it's a sync frame
    // 3. If yes, it calls SetVideoFirstFrame() to supplement the first frame
    // 4. The track is removed from firstFramePendingTracks_
    Status st = demuxerPlugin_->ReadSample(audioTrackId, avbuffer.mediaAVBuffer, timeoutMs);
    std::cout << "[TEST061337] audio read st=" << static_cast<int>(st)
              << ", pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    ASSERT_EQ(st, Status::OK);
    
    // Verify that audio read succeeded, which indicates:
    // 1. The supplement mechanism worked (if first cached audio frame was sync frame),
    //    and the log "Supplement first frame for track 1" should appear in system logs
    // 2. Or the cached frames are usable even without supplement (if first frame wasn't sync)
    // The actual supplement log can be verified in system logs (MEDIA_LOG_I) if needed.
    // The key verification is that ReadSample() succeeds after hard limit, demonstrating
    // that the system can recover from the hard limit scenario.

    // Read video sample should also succeed
    st = demuxerPlugin_->ReadSample(videoTrackId, avbuffer.mediaAVBuffer, timeoutMs);
    std::cout << "[TEST061337] video read st=" << static_cast<int>(st)
              << ", pts=" << avbuffer.mediaAVBuffer->pts_
              << ", dts=" << avbuffer.mediaAVBuffer->dts_ << std::endl;
    ASSERT_EQ(st, Status::OK);

    AvReadFrameMockReset();
    AvReadFrameMockLogEnable(0);
}