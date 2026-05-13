/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "gtest/gtest.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "mpeg4_source_unit_test.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
const std::string TEST_FILE_PATH = "/data/test/media/";
const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
const int64_t SOURCE_OFFSET = 0;
const std::string M4V_PATH = TEST_FILE_PATH + "h264_fmp4.m4v";
const std::string M4V_URI = TEST_URI_PATH + "h264_fmp4.m4v";

}

/**
 * @tc.name: mp4Source_GetFormat_2004
 * @tc.desc: get format when the file is m4v, local
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2004, TestSize.Level1)
{
    fd_ = OpenFile(M4V_PATH);
    size_ = GetFileSize(M4V_PATH);
    printf("---- %s ----\n", M4V_PATH.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10033333);
    EXPECT_EQ(formatVal_.trackCount, 1);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    EXPECT_EQ(formatVal_.bitRate, 1267567);
}

/**
 * @tc.name: mp4Source_GetFormat_3004
 * @tc.desc: get format when the file is m4v, uri
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3004, TestSize.Level1)
{
    printf("---- %s ------\n", M4V_URI.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(M4V_URI.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10033333);
    EXPECT_EQ(formatVal_.trackCount, 1);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    EXPECT_EQ(formatVal_.bitRate, 1267567);
}
