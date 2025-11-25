/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <string>
#include <malloc.h>
#include <sys/stat.h>
#include <cinttypes>
#include <fcntl.h>
#include <list>
#include <cmath>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "demuxer_unit_test.h"

#define LOCAL true
#define URI false

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";

string g_skipOggPath = TEST_FILE_PATH + string("audio/skip_ogg.ogg");
string g_skipOggUri = TEST_URI_PATH + string("audio/skip_ogg.ogg");
string g_skipMp3Path = TEST_FILE_PATH + string("audio/mp3_48000_1_cover.mp3");
string g_skipMp3Uri = TEST_URI_PATH + string("audio/mp3_48000_1_cover.mp3");
} // namespace

namespace {
/**
 * @tc.name: Demuxer_ReadSample_CheckSkipInfo_0011
 * @tc.desc: Check skip info from buffer, ogg, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_CheckSkipInfo_0011, TestSize.Level1)
{
    InitResource(g_skipOggPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    uint32_t idx = 0;
    std::map<int32_t, std::vector<uint8_t>> skipInfoMap = {
        {49, {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00}},
    };
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_, true), AV_ERR_OK);
        CountFrames(idx);
        if (!isEOS(eosFlag_) && skipInfoMap.count(frames_[0]) > 0) {
            BufferInfo currentBufferInfo = GetCurrentBufferInfo();
            printf("check frame %d skip info size %zu\n", frames_[0], currentBufferInfo.skipSize);
            ASSERT_EQ(currentBufferInfo.skipSize, skipInfoMap[frames_[0]].size());
            printf("check frame %d skip info data", frames_[0]);
            for (auto i = 0; i < currentBufferInfo.skipSize; i++) {
                printf(" %02x", currentBufferInfo.skipData[i]);
                ASSERT_EQ(currentBufferInfo.skipData[i], skipInfoMap[frames_[0]][i]);
            }
            printf("\n");
        }
    }
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_CheckSkipInfo_0012
 * @tc.desc: Check skip info from buffer, ogg, url
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_CheckSkipInfo_0012, TestSize.Level1)
{
    InitResource(g_skipOggUri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    uint32_t idx = 0;
    std::map<int32_t, std::vector<uint8_t>> skipInfoMap = {
        {49, {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00}},
    };
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_, true), AV_ERR_OK);
        CountFrames(idx);
        if (!isEOS(eosFlag_) && skipInfoMap.count(frames_[0]) > 0) {
            BufferInfo currentBufferInfo = GetCurrentBufferInfo();
            printf("check frame %d skip info size %zu\n", frames_[0], currentBufferInfo.skipSize);
            ASSERT_EQ(currentBufferInfo.skipSize, skipInfoMap[frames_[0]].size());
            printf("check frame %d skip info data", frames_[0]);
            for (auto i = 0; i < currentBufferInfo.skipSize; i++) {
                printf(" %02x", currentBufferInfo.skipData[i]);
                ASSERT_EQ(currentBufferInfo.skipData[i], skipInfoMap[frames_[0]][i]);
            }
            printf("\n");
        }
    }
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_CheckSkipInfo_0021
 * @tc.desc: Check skip info from buffer, mp3, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_CheckSkipInfo_0021, TestSize.Level1)
{
    InitResource(g_skipMp3Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    uint32_t idx = 0;
    std::map<int32_t, std::vector<uint8_t>> skipInfoMap = {
        {1, {0x51, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {1251, {0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00}},
    };
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_, true), AV_ERR_OK);
        CountFrames(idx);
        if (!isEOS(eosFlag_) && skipInfoMap.count(frames_[0]) > 0) {
            BufferInfo currentBufferInfo = GetCurrentBufferInfo();
            printf("check frame %d skip info size %zu\n", frames_[0], currentBufferInfo.skipSize);
            ASSERT_EQ(currentBufferInfo.skipSize, skipInfoMap[frames_[0]].size());
            printf("check frame %d skip info data", frames_[0]);
            for (auto i = 0; i < currentBufferInfo.skipSize; i++) {
                printf(" %02x", currentBufferInfo.skipData[i]);
                ASSERT_EQ(currentBufferInfo.skipData[i], skipInfoMap[frames_[0]][i]);
            }
            printf("\n");
        }
    }
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_CheckSkipInfo_0022
 * @tc.desc: Check skip info from buffer, mp3, url
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_CheckSkipInfo_0022, TestSize.Level1)
{
    InitResource(g_skipMp3Uri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    uint32_t idx = 0;
    std::map<int32_t, std::vector<uint8_t>> skipInfoMap = {
        {1, {0x51, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {1251, {0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00}},
    };
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_, true), AV_ERR_OK);
        CountFrames(idx);
        if (!isEOS(eosFlag_) && skipInfoMap.count(frames_[0]) > 0) {
            BufferInfo currentBufferInfo = GetCurrentBufferInfo();
            printf("check frame %d skip info size %zu\n", frames_[0], currentBufferInfo.skipSize);
            ASSERT_EQ(currentBufferInfo.skipSize, skipInfoMap[frames_[0]].size());
            printf("check frame %d skip info data", frames_[0]);
            for (auto i = 0; i < currentBufferInfo.skipSize; i++) {
                printf(" %02x", currentBufferInfo.skipData[i]);
                ASSERT_EQ(currentBufferInfo.skipData[i], skipInfoMap[frames_[0]][i]);
            }
            printf("\n");
        }
    }
    RemoveValue();
}
} // namespace
