/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "media_cached_buffer.h"
#include "gtest/gtest.h"
#include <cstring>
#include <memory>

using namespace OHOS::Media;
using namespace testing::ext;

class MediaCachedBufferTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    std::unique_ptr<CacheMediaChunkBufferImpl> buffer_;
    const uint32_t DEFAULT_CHUNK_SIZE = 16 * 1024;
    const uint64_t DEFAULT_BUFFER_SIZE = 5 * 1024 * 1024;
};

void MediaCachedBufferTest::SetUpTestCase(void)
{
}

void MediaCachedBufferTest::TearDownTestCase(void)
{
}

void MediaCachedBufferTest::SetUp(void)
{
    buffer_ = std::make_unique<CacheMediaChunkBufferImpl>();
}

void MediaCachedBufferTest::TearDown(void)
{
    buffer_.reset();
}

HWTEST_F(MediaCachedBufferTest, Init_InvalidParameters_TotalBuffSizeZero, TestSize.Level1)
{
    bool result = buffer_->Init(0, DEFAULT_CHUNK_SIZE);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, Init_InvalidParameters_ChunkSizeZero, TestSize.Level1)
{
    bool result = buffer_->Init(DEFAULT_BUFFER_SIZE, 0);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, Init_InvalidParameters_TotalBuffSizeLessThanChunkSize, TestSize.Level1)
{
    bool result = buffer_->Init(1024, 2048);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, Init_ValidParameters, TestSize.Level1)
{
    bool result = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaCachedBufferTest, Init_DoubleInit, TestSize.Level1)
{
    bool result1 = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    EXPECT_TRUE(result1);
    bool result2 = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    EXPECT_FALSE(result2);
}

HWTEST_F(MediaCachedBufferTest, Write_BeforeInit, TestSize.Level1)
{
    uint8_t data[1024] = {0};
    size_t result = buffer_->Write(data, 0, sizeof(data));
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Write_NullPointer, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    size_t result = buffer_->Write(nullptr, 0, 1024);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Write_ZeroSize, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {0};
    size_t result = buffer_->Write(data, 0, 0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Write_SimpleData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {0};
    for (int i = 0; i < 1024; i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    size_t result = buffer_->Write(data, 0, sizeof(data));
    EXPECT_EQ(result, sizeof(data));
}

HWTEST_F(MediaCachedBufferTest, Write_MultipleChunks, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    std::vector<uint8_t> data(DEFAULT_CHUNK_SIZE * 2, 0xAA);
    size_t result = buffer_->Write(data.data(), 0, data.size());
    EXPECT_EQ(result, data.size());
}

HWTEST_F(MediaCachedBufferTest, Write_WithOffset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {0};
    size_t result = buffer_->Write(data, 1024, sizeof(data));
    EXPECT_EQ(result, sizeof(data));
}

HWTEST_F(MediaCachedBufferTest, Write_AppendData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data1[1024] = {1};
    uint8_t data2[1024] = {2};
    
    size_t result1 = buffer_->Write(data1, 0, sizeof(data1));
    EXPECT_EQ(result1, sizeof(data1));
    
    size_t result2 = buffer_->Write(data2, sizeof(data1), sizeof(data2));
    EXPECT_EQ(result2, sizeof(data2));
}

HWTEST_F(MediaCachedBufferTest, Write_LargeData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    std::vector<uint8_t> data(DEFAULT_BUFFER_SIZE / 2, 0xAA);
    size_t result = buffer_->Write(data.data(), 0, data.size());
    EXPECT_EQ(result, data.size());
}

HWTEST_F(MediaCachedBufferTest, Read_BeforeInit, TestSize.Level1)
{
    uint8_t buffer[1024] = {0};
    size_t result = buffer_->Read(buffer, 0, sizeof(buffer));
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Read_NullPointer, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    size_t result = buffer_->Read(nullptr, 0, 1024);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Read_ZeroSize, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    size_t result = buffer_->Read(nullptr, 0, 0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Read_NoDataWritten, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t buffer[1024] = {0};
    size_t result = buffer_->Read(buffer, 0, sizeof(buffer));
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Read_SimpleData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t writeData[1024] = {0};
    for (int i = 0; i < 1024; i++) {
        writeData[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(writeData, 0, sizeof(writeData));
    
    uint8_t readData[1024] = {0};
    size_t result = buffer_->Read(readData, 0, sizeof(readData));
    EXPECT_EQ(result, sizeof(readData));
    EXPECT_EQ(memcmp(writeData, readData, sizeof(writeData)), 0);
}

HWTEST_F(MediaCachedBufferTest, Read_PartialData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t writeData[1024] = {0};
    for (int i = 0; i < 1024; i++) {
        writeData[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(writeData, 0, sizeof(writeData));
    
    uint8_t readData[512] = {0};
    size_t result = buffer_->Read(readData, 0, sizeof(readData));
    EXPECT_EQ(result, sizeof(readData));
    EXPECT_EQ(memcmp(writeData, readData, sizeof(readData)), 0);
}

HWTEST_F(MediaCachedBufferTest, Read_WithOffset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t writeData[1024] = {0};
    for (int i = 0; i < 1024; i++) {
        writeData[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(writeData, 0, sizeof(writeData));
    
    uint8_t readData[512] = {0};
    size_t result = buffer_->Read(readData, 512, sizeof(readData));
    EXPECT_EQ(result, sizeof(readData));
    EXPECT_EQ(memcmp(writeData + 512, readData, sizeof(readData)), 0);
}

HWTEST_F(MediaCachedBufferTest, Read_MultipleChunks, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    std::vector<uint8_t> writeData(DEFAULT_CHUNK_SIZE * 2, 0);
    for (size_t i = 0; i < writeData.size(); i++) {
        writeData[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(writeData.data(), 0, writeData.size());
    
    std::vector<uint8_t> readData(writeData.size(), 0);
    size_t result = buffer_->Read(readData.data(), 0, readData.size());
    EXPECT_EQ(result, readData.size());
    EXPECT_EQ(memcmp(writeData.data(), readData.data(), writeData.size()), 0);
}

HWTEST_F(MediaCachedBufferTest, Seek_BeforeInit, TestSize.Level1)
{
    bool result = buffer_->Seek(0);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, Seek_NoData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    bool result = buffer_->Seek(0);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, Seek_ValidOffset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    bool result = buffer_->Seek(512);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaCachedBufferTest, Seek_InvalidOffset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    bool result = buffer_->Seek(2048);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, GetBufferSize_BeforeInit, TestSize.Level1)
{
    uint64_t result = buffer_->GetBufferSize(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, GetBufferSize_NoData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint64_t result = buffer_->GetBufferSize(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, GetBufferSize_WithData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t result = buffer_->GetBufferSize(0);
    EXPECT_EQ(result, sizeof(data));
}

HWTEST_F(MediaCachedBufferTest, GetBufferSize_WithOffset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[2048] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t result = buffer_->GetBufferSize(512);
    EXPECT_EQ(result, sizeof(data) - 512);
}

HWTEST_F(MediaCachedBufferTest, GetNextBufferOffset_BeforeInit, TestSize.Level1)
{
    uint64_t result = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, GetNextBufferOffset_NoData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint64_t result = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, GetNextBufferOffset_WithOneFragment, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t result = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, GetNextBufferOffset_WithTwoFragments, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data1[1024] = {1};
    uint8_t data2[1024] = {2};
    
    buffer_->Write(data1, 0, sizeof(data1));
    buffer_->Write(data2, 4096, sizeof(data2));
    
    uint64_t result = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(result, 4096);
}

HWTEST_F(MediaCachedBufferTest, GetFreeSize_BeforeInit, TestSize.Level1)
{
    uint64_t result = buffer_->GetFreeSize();
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, GetFreeSize_AfterInit, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint64_t result = buffer_->GetFreeSize();
    EXPECT_EQ(result, DEFAULT_BUFFER_SIZE);
}

HWTEST_F(MediaCachedBufferTest, GetFreeSize_AfterWrite, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t result = buffer_->GetFreeSize();
    EXPECT_EQ(result, DEFAULT_BUFFER_SIZE - sizeof(data));
}

HWTEST_F(MediaCachedBufferTest, Clear_BeforeInit, TestSize.Level1)
{
    buffer_->Clear();
    uint64_t result = buffer_->GetBufferSize(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Clear_AfterWrite, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Clear();
    uint64_t result = buffer_->GetBufferSize(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Clear_MultipleTimes, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Clear();
    buffer_->Clear();
    
    uint64_t result = buffer_->GetBufferSize(0);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaCachedBufferTest, Reset_BeforeInit, TestSize.Level1)
{
    buffer_->Reset();
    bool result = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaCachedBufferTest, Reset_AfterInit, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Reset();
    
    uint64_t bufferSize = buffer_->GetBufferSize(0);
    EXPECT_EQ(bufferSize, 0);
}

HWTEST_F(MediaCachedBufferTest, Reset_ReinitAfterReset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Reset();
    bool result = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearFragmentBeforeOffset_BeforeInit, TestSize.Level1)
{
    bool result = buffer_->ClearFragmentBeforeOffset(512);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearFragmentBeforeOffset_NoData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    bool result = buffer_->ClearFragmentBeforeOffset(512);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearFragmentBeforeOffset_ValidOffset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[2048] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    bool result = buffer_->ClearFragmentBeforeOffset(1024);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearChunksOfFragment_BeforeInit, TestSize.Level1)
{
    bool result = buffer_->ClearChunksOfFragment(512);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearChunksOfFragment_NoData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    bool result = buffer_->ClearChunksOfFragment(512);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearChunksOfFragment_ValidOffset, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    uint8_t data[2048] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    bool result = buffer_->ClearChunksOfFragment(1024);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearMiddleReadFragment_BeforeInit, TestSize.Level1)
{
    bool result = buffer_->ClearMiddleReadFragment(512, 1024);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, ClearMiddleReadFragment_NoData, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    bool result = buffer_->ClearMiddleReadFragment(512, 1024);
    EXPECT_FALSE(result);
}

HWTEST_F(MediaCachedBufferTest, SetIsLargeOffsetSpan_True, TestSize.Level1)
{
    buffer_->SetIsLargeOffsetSpan(true);
    bool result = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaCachedBufferTest, SetIsLargeOffsetSpan_False, TestSize.Level1)
{
    buffer_->SetIsLargeOffsetSpan(false);
    bool result = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaCachedBufferTest, Write_Read_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint8_t readData[1024] = {0};
    buffer_->Read(readData, 0, sizeof(readData));
    EXPECT_EQ(readData[0], 1);
}

HWTEST_F(MediaCachedBufferTest, Write_Seek_Read_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[2048] = {0};
    for (int i = 0; i < 2048; i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Seek(0);
    uint8_t readData[1024] = {0};
    buffer_->Read(readData, 0, sizeof(readData));
    EXPECT_EQ(readData[0], 0);
}

HWTEST_F(MediaCachedBufferTest, Write_Clear_Write_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Clear();
    
    uint8_t data2[1024] = {2};
    buffer_->Write(data2, 0, sizeof(data2));
    
    uint8_t readData[1024] = {0};
    buffer_->Read(readData, 0, sizeof(readData));
    EXPECT_EQ(readData[0], 2);
}

HWTEST_F(MediaCachedBufferTest, Write_Reset_Write_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Reset();
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data2[1024] = {2};
    buffer_->Write(data2, 0, sizeof(data2));
    
    uint8_t readData[1024] = {0};
    buffer_->Read(readData, 0, sizeof(readData));
    EXPECT_EQ(readData[0], 2);
}

HWTEST_F(MediaCachedBufferTest, Write_GetBufferSize_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t size1 = buffer_->GetBufferSize(0);
    EXPECT_EQ(size1, sizeof(data));
    
    buffer_->Clear();
    
    uint64_t size2 = buffer_->GetBufferSize(0);
    EXPECT_EQ(size2, 0);
}

HWTEST_F(MediaCachedBufferTest, Write_GetFreeSize_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t free1 = buffer_->GetFreeSize();
    EXPECT_EQ(free1, DEFAULT_BUFFER_SIZE - sizeof(data));
    
    buffer_->Clear();
    
    uint64_t free2 = buffer_->GetFreeSize();
    EXPECT_EQ(free2, DEFAULT_BUFFER_SIZE);
}

HWTEST_F(MediaCachedBufferTest, Write_Read_Clear_Write_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data1[1024] = {1};
    buffer_->Write(data1, 0, sizeof(data1));
    
    uint8_t readData1[1024] = {0};
    buffer_->Read(readData1, 0, sizeof(readData1));
    EXPECT_EQ(readData1[0], 1);
    
    buffer_->Clear();
    
    uint8_t data2[1024] = {2};
    buffer_->Write(data2, 0, sizeof(data2));
    
    uint8_t readData2[1024] = {0};
    size_t result = buffer_->Read(readData2, 0, sizeof(readData2));
    EXPECT_EQ(result, 1024);
}

HWTEST_F(MediaCachedBufferTest, Write_Seek_Read_Seek_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[2048] = {0};
    for (int i = 0; i < 2048; i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Seek(0);
    uint8_t readData1[1024] = {0};
    buffer_->Read(readData1, 0, sizeof(readData1));
    EXPECT_EQ(readData1[0], 0);
    
    buffer_->Seek(1024);
    uint8_t readData2[1024] = {0};
    buffer_->Read(readData2, 1024, sizeof(readData2));
    EXPECT_EQ(readData2[0], data[1024]);
}

HWTEST_F(MediaCachedBufferTest, Write_GetNextBufferOffset_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data1[1024] = {1};
    buffer_->Write(data1, 0, sizeof(data1));
    buffer_->Write(data1, 8192, sizeof(data1));
    
    uint64_t offset1 = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(offset1, 8192);
    
    buffer_->Clear();
    
    uint64_t offset2 = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(offset2, 0);
}

HWTEST_F(MediaCachedBufferTest, Write_ClearFragmentBeforeOffset_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[2048] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    bool clearResult1 = buffer_->ClearFragmentBeforeOffset(1024);
    EXPECT_FALSE(clearResult1);
    
    buffer_->Clear();
    
    bool clearResult2 = buffer_->ClearFragmentBeforeOffset(1024);
    EXPECT_FALSE(clearResult2);
}

HWTEST_F(MediaCachedBufferTest, Write_ClearChunksOfFragment_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[2048] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    bool clearResult1 = buffer_->ClearChunksOfFragment(1024);
    EXPECT_FALSE(clearResult1);
    
    buffer_->Clear();
    
    bool clearResult2 = buffer_->ClearChunksOfFragment(1024);
    EXPECT_FALSE(clearResult2);
}

HWTEST_F(MediaCachedBufferTest, Write_ClearMiddleReadFragment_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[4096] = {1};
    buffer_->Write(data, 0, sizeof(data));
    buffer_->Seek(0);
    
    bool clearResult1 = buffer_->ClearMiddleReadFragment(512, 2048);
    EXPECT_FALSE(clearResult1);
    
    buffer_->Clear();
    
    bool clearResult2 = buffer_->ClearMiddleReadFragment(512, 2048);
    EXPECT_FALSE(clearResult2);
}

HWTEST_F(MediaCachedBufferTest, Write_MultipleOperations, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    for (int i = 0; i < 10; i++) {
        uint8_t data[1024] = {static_cast<uint8_t>(i)};
        size_t result = buffer_->Write(data, i * 1024, sizeof(data));
        EXPECT_EQ(result, sizeof(data));
    }
    
    uint64_t bufferSize = buffer_->GetBufferSize(0);
    EXPECT_EQ(bufferSize, 10 * 1024);
}

HWTEST_F(MediaCachedBufferTest, GetBufferSize_MultipleOperations, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    for (int i = 0; i < 5; i++) {
        uint64_t size = buffer_->GetBufferSize(i * 256);
        EXPECT_EQ(size, sizeof(data) - i * 256);
    }
}

HWTEST_F(MediaCachedBufferTest, GetFreeSize_MultipleOperations, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    for (int i = 0; i < 5; i++) {
        uint64_t free = buffer_->GetFreeSize();
        EXPECT_EQ(free, 5241856);
    }
}

HWTEST_F(MediaCachedBufferTest, Clear_MultipleOperations, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    for (int i = 0; i < 3; i++) {
        buffer_->Clear();
        uint64_t size = buffer_->GetBufferSize(0);
        EXPECT_EQ(size, 0);
    }
}

HWTEST_F(MediaCachedBufferTest, Reset_MultipleOperations, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    for (int i = 0; i < 3; i++) {
        buffer_->Reset();
        bool result = buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
        EXPECT_TRUE(result);
    }
}

HWTEST_F(MediaCachedBufferTest, Write_Read_Clear_Reset_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data1[1024] = {1};
    buffer_->Write(data1, 0, sizeof(data1));
    
    uint8_t readData1[1024] = {0};
    buffer_->Read(readData1, 0, sizeof(readData1));
    EXPECT_EQ(readData1[0], 1);
    
    buffer_->Clear();
    buffer_->Reset();
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data2[1024] = {2};
    buffer_->Write(data2, 0, sizeof(data2));
    
    uint8_t readData2[1024] = {0};
    size_t result = buffer_->Read(readData2, 0, sizeof(readData2));
    EXPECT_EQ(result, sizeof(readData2));
    EXPECT_EQ(readData2[0], 2);
}

HWTEST_F(MediaCachedBufferTest, Write_Seek_Read_Clear_Seek_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[2048] = {0};
    for (int i = 0; i < 2048; i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Seek(0);
    uint8_t readData1[1024] = {0};
    buffer_->Read(readData1, 0, sizeof(readData1));
    EXPECT_EQ(readData1[0], 0);
    
    buffer_->Seek(1024);
    uint8_t readData2[1024] = {0};
    buffer_->Read(readData2, 1024, sizeof(readData2));
    EXPECT_EQ(readData2[0], data[1024]);
    
    buffer_->Clear();
    buffer_->Reset();
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    buffer_->Seek(0);
    uint8_t readData3[1024] = {0};
    buffer_->Read(readData3, 0, sizeof(readData3));
    EXPECT_EQ(readData3[0], 0);
}

HWTEST_F(MediaCachedBufferTest, Write_GetBufferSize_Clear_GetBufferSize_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t size1 = buffer_->GetBufferSize(0);
    EXPECT_EQ(size1, sizeof(data));
    
    buffer_->Clear();
    uint64_t size2 = buffer_->GetBufferSize(0);
    EXPECT_EQ(size2, 0);
    
    buffer_->Reset();
    uint64_t size3 = buffer_->GetBufferSize(0);
    EXPECT_EQ(size3, 0);
}

HWTEST_F(MediaCachedBufferTest, Write_GetFreeSize_Clear_GetFreeSize_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[1024] = {1};
    buffer_->Write(data, 0, sizeof(data));
    
    uint64_t free1 = buffer_->GetFreeSize();
    EXPECT_EQ(free1, DEFAULT_BUFFER_SIZE - sizeof(data));
    
    buffer_->Clear();
    uint64_t free2 = buffer_->GetFreeSize();
    EXPECT_EQ(free2, DEFAULT_BUFFER_SIZE);
    
    buffer_->Reset();
    uint64_t free3 = buffer_->GetFreeSize();
    EXPECT_EQ(free3, 0);
}

HWTEST_F(MediaCachedBufferTest, Write_GetNextBufferOffset_Clear_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data1[1024] = {1};
    uint8_t data2[1024] = {2};
    
    buffer_->Write(data1, 0, sizeof(data1));
    buffer_->Write(data2, 4096, sizeof(data2));
    
    uint64_t offset1 = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(offset1, 4096);
    
    buffer_->Clear();
    uint64_t offset2 = buffer_->GetNextBufferOffset(0);
    EXPECT_EQ(offset2, 0);
}

HWTEST_F(MediaCachedBufferTest, Complex_Write_Read_Seek_Clear_Sequence, TestSize.Level1)
{
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    uint8_t data[2048] = {0};
    for (int i = 0; i < 2048; i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    buffer_->Write(data, 0, sizeof(data));
    
    buffer_->Seek(0);
    uint8_t readData[1024] = {0};
    buffer_->Read(readData, 0, sizeof(readData));
    EXPECT_EQ(readData[0], 0);
    
    buffer_->Seek(1024);
    uint8_t readData2[1024] = {0};
    buffer_->Read(readData2, 1024, sizeof(readData2));
    EXPECT_EQ(readData2[0], data[1024]);
    
    buffer_->Clear();
    buffer_->Reset();
    buffer_->Init(DEFAULT_BUFFER_SIZE, DEFAULT_CHUNK_SIZE);
    
    buffer_->Seek(0);
    uint8_t readData3[1024] = {0};
    buffer_->Read(readData3, 0, sizeof(readData3));
    EXPECT_EQ(readData3[0], 0);
}
