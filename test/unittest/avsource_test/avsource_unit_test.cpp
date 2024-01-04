/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_audio_common.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "avsource_unit_test.h"

#define LOCAL true

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
const int64_t SOURCE_OFFSET = 0;

string g_mp4Path = TEST_FILE_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_mp4Path3 = TEST_FILE_PATH + string("test_mpeg2_B_Gop25_4sec.mp4");
string g_mp4Path5 = TEST_FILE_PATH + string("test_suffix_mismatch.mp4");
string g_mp4Path6 = TEST_FILE_PATH + string("test_empty_file.mp4");
string g_mp4Path7 = TEST_FILE_PATH + string("test_error.mp4");
string g_mp4Path8 = TEST_FILE_PATH + string("zero_track.mp4");
string g_mkvPath2 = TEST_FILE_PATH + string("h264_opus_4sec.mkv");
string g_tsPath = TEST_FILE_PATH + string("test_mpeg2_Gop25_4sec.ts");
string g_aacPath = TEST_FILE_PATH + string("audio/aac_44100_1.aac");
string g_flacPath = TEST_FILE_PATH + string("audio/flac_48000_1_cover.flac");
string g_m4aPath = TEST_FILE_PATH + string("audio/m4a_48000_1.m4a");
string g_mp3Path = TEST_FILE_PATH + string("audio/mp3_48000_1_cover.mp3");
string g_oggPath = TEST_FILE_PATH + string("audio/ogg_48000_1.ogg");
string g_wavPath = TEST_FILE_PATH + string("audio/wav_48000_1.wav");
string g_amrPath = TEST_FILE_PATH + string("audio/amr_nb_8000_1.amr");
string g_amrPath2 = TEST_FILE_PATH + string("audio/amr_wb_16000_1.amr");
string g_audioVividPath = TEST_FILE_PATH + string("2obj_44100Hz_16bit_32k.mp4");
string g_audioVividPath2 = TEST_FILE_PATH + string("2obj_44100Hz_16bit_32k.ts");
} // namespace

void AVSourceUnitTest::SetUpTestCase(void)
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}

void AVSourceUnitTest::TearDownTestCase(void)
{
    server->StopServer();
}

void AVSourceUnitTest::SetUp(void) {}

void AVSourceUnitTest::TearDown(void)
{
    if (source_ != nullptr) {
        source_->Destroy();
        source_ = nullptr;
    }
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    if (format_ != nullptr) {
        format_->Destroy();
        format_ = nullptr;
    }
    trackIndex_ = 0;
    size_ = 0;
    addr_ = nullptr;
    buffSize_ = 0;
    ResetFormatValue();
}

int64_t AVSourceUnitTest::GetFileSize(const string &fileName)
{
    int64_t fileSize = 0;
    if (!fileName.empty()) {
        struct stat fileStatus {};
        if (stat(fileName.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

int32_t AVSourceUnitTest::OpenFile(const string &fileName)
{
    int32_t fd = open(fileName.c_str(), O_RDONLY);
    return fd;
}

void AVSourceUnitTest::ResetFormatValue()
{
    formatVal_.title = "";
    formatVal_.artist = "";
    formatVal_.album = "";
    formatVal_.albumArtist = "";
    formatVal_.date = "";
    formatVal_.comment = "";
    formatVal_.genre = "";
    formatVal_.copyright = "";
    formatVal_.description = "";
    formatVal_.language = "";
    formatVal_.lyrics = "";
    formatVal_.duration = 0;
    formatVal_.trackCount = 0;
    formatVal_.author = "";
    formatVal_.composer = "";
    formatVal_.hasVideo = -1;
    formatVal_.hasAudio = -1;
    formatVal_.fileType = 0;
    formatVal_.codecMime = "";
    formatVal_.trackType = 0;
    formatVal_.width = 0;
    formatVal_.height = 0;
    formatVal_.aacIsAdts = -1;
    formatVal_.sampleRate = 0;
    formatVal_.channelCount = 0;
    formatVal_.bitRate = 0;
    formatVal_.audioSampleFormat = 0;
    formatVal_.frameRate = 0;
    formatVal_.rotationAngle = 0;
    formatVal_.channelLayout = 0;
    formatVal_.hdrType = 0;
    formatVal_.codecProfile = 0;
    formatVal_.codecLevel = 0;
    formatVal_.colorPrimaries = 0;
    formatVal_.transferCharacteristics = 0;
    formatVal_.rangeFlag = 0;
    formatVal_.matrixCoefficients = 0;
    formatVal_.chromaLocation = 0;
    formatVal_.profile = 0;
    formatVal_.level = 0;
    formatVal_.colorPri = 0;
    formatVal_.colorTrans = 0;
    formatVal_.colorMatrix = 0;
    formatVal_.colorRange = 0;
    formatVal_.chromaLoc = 0;
    formatVal_.isHdrVivid = 0;
}

/**********************************source FD**************************************/
namespace {
/**
 * @tc.name: AVSource_CreateSourceWithFD_1000
 * @tc.desc: create source with fd, mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1000, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path.c_str());
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    size_ += 1000;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    size_ = 1000;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
    size_ = 0;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
    size_ = -1;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1010
 * @tc.desc: create source with fd, ts
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1010, TestSize.Level1)
{
    printf("---- %s ----\n", g_tsPath.c_str());
    fd_ = OpenFile(g_tsPath);
    size_ = GetFileSize(g_tsPath);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1020
 * @tc.desc: create source with fd, but file is abnormal
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1020, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path5.c_str());
    fd_ = OpenFile(g_mp4Path5);
    size_ = GetFileSize(g_mp4Path5);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1030
 * @tc.desc: create source with fd, but file is empty
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1030, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path6.c_str());
    fd_ = OpenFile(g_mp4Path6);
    size_ = GetFileSize(g_mp4Path6);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1040
 * @tc.desc: create source with fd, but file is error
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1040, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path7.c_str());
    fd_ = OpenFile(g_mp4Path7);
    size_ = GetFileSize(g_mp4Path7);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1050
 * @tc.desc: create source with fd, but track is zero
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1050, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path8.c_str());
    fd_ = OpenFile(g_mp4Path8);
    size_ = GetFileSize(g_mp4Path8);
    cout << "---fd: " << fd_ << "---size: " << size_ << endl;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1060
 * @tc.desc: create source with fd, the values of fd is abnormal;
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1060, TestSize.Level1)
{
    size_ = 1000;
    fd_ = 0;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
    fd_ = 1;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
    fd_ = 2;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
    fd_ = -1;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1070
 * @tc.desc: create source with fd, offset is exception value;
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1070, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path.c_str());
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    int64_t offset = 5000;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, offset, size_);
    ASSERT_EQ(source_, nullptr);
    offset = -10;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, offset, size_);
    ASSERT_EQ(source_, nullptr);
    offset = size_;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, offset, size_);
    ASSERT_EQ(source_, nullptr);
    offset = size_ + 1000;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, offset, size_);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1080
 * @tc.desc: Create source repeatedly
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1080, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path.c_str());
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_1090
 * @tc.desc: destroy source
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_1090, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4Path.c_str());
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    int32_t fd2 = OpenFile(g_mp3Path);
    int64_t size2 = GetFileSize(g_mp3Path);
    printf("---- %s ----\n", g_mp3Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    shared_ptr<AVSourceMock> source2 = AVSourceMockFactory::CreateSourceWithFD(fd2, SOURCE_OFFSET, size2);
    ASSERT_NE(source2, nullptr);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
    ASSERT_EQ(source2->Destroy(), AV_ERR_OK);
    source_ = nullptr;
    source2 = nullptr;
    close(fd2);
}

/**
 * @tc.name: AVSource_GetFormat_1000
 * @tc.desc: get source format(mp4)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1000, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    printf("---- %s ----\n", g_mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.duration, 4120000);
    ASSERT_EQ(formatVal_.trackCount, 3);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 101);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1010
 * @tc.desc: get track format (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1010, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    printf("---- %s ------\n", g_mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.bitRate, 7782407);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatVal_.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128563);
    ASSERT_EQ(formatVal_.aacIsAdts, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_1011
 * @tc.desc: get track format(mp4, cover)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1011, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    printf("---- %s ------\n", g_mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 2;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
#ifdef AVSOURCE_INNER_UNIT_TEST
    const char* outFile = "/data/test/test_264_B_Gop25_4sec_cover.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &addr_, buffSize_));
    fwrite(addr_, sizeof(uint8_t), buffSize_, saveFile);
    fclose(saveFile);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1020
 * @tc.desc: get source format when the file is ts(mpeg2, aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1020, TestSize.Level1)
{
    fd_ = OpenFile(g_tsPath);
    size_ = GetFileSize(g_tsPath);
    printf("---- %s ----\n", g_tsPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 127103);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_1030
 * @tc.desc: get source format when the file is mp4(mpeg2 aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1030, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4Path3);
    size_ = GetFileSize(g_mp4Path3);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
#ifdef AVSOURCE_INNER_UNIT_TEST
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 101);
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.author, "author");
#endif
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.bitRate, 3889231);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128563);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_1050
 * @tc.desc: get format when the file is mkv (video: h264, audio: opus)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1050, TestSize.Level1)
{
    fd_ = OpenFile(g_mkvPath2);
    size_ = GetFileSize(g_mkvPath2);
    printf("---- %s ----\n", g_mkvPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.albumArtist, "album_artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "copyRight");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.duration, 4001000);
    ASSERT_EQ(formatVal_.trackCount, 2);
    ASSERT_EQ(formatVal_.language, "language");
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 103);
    ASSERT_EQ(formatVal_.author, "author");
    ASSERT_EQ(formatVal_.composer, "composer");
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1060
 * @tc.desc: get format when the file is mkv (video: h264, audio: opus)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1060, TestSize.Level1)
{
    fd_ = OpenFile(g_mkvPath2);
    size_ = GetFileSize(g_mkvPath2);
    printf("---- %s ----\n", g_mkvPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/opus");
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_1100
 * @tc.desc: get format when the file is aac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1100, TestSize.Level1)
{
    fd_ = OpenFile(g_aacPath);
    size_ = GetFileSize(g_aacPath);
    printf("---- %s ----\n", g_aacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30023469);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 202);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatVal_.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 126800);
    ASSERT_EQ(formatVal_.aacIsAdts, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_1110
 * @tc.desc: get format when the file is flac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1110, TestSize.Level1)
{
    fd_ = OpenFile(g_flacPath);
    size_ = GetFileSize(g_flacPath);
    printf("---- %s ----\n", g_flacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.duration, 30000000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.author, "author");
    ASSERT_EQ(formatVal_.fileType, 204);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1111
 * @tc.desc: get format when the file is flac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1111, TestSize.Level1)
{
    fd_ = OpenFile(g_flacPath);
    size_ = GetFileSize(g_flacPath);
    printf("---- %s ----\n", g_flacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/flac");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
#ifdef AVSOURCE_INNER_UNIT_TEST
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/flac_48000_1_cover.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &addr_, buffSize_));
    fwrite(addr_, sizeof(uint8_t), buffSize_, saveFile);
    fclose(saveFile);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_11201
 * @tc.desc: get format when the file is m4a
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_11201, TestSize.Level1)
{
    fd_ = OpenFile(g_m4aPath);
    size_ = GetFileSize(g_m4aPath);
    printf("---- %s ----\n", g_m4aPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.duration, 30016000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.fileType, 206);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1121
 * @tc.desc: get format when the file is m4a
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1121, TestSize.Level1)
{
    fd_ = OpenFile(g_m4aPath);
    size_ = GetFileSize(g_m4aPath);
    printf("---- %s ----\n", g_m4aPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 69594);
}

/**
 * @tc.name: AVSource_GetFormat_1130
 * @tc.desc: get format when the file is mp3
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1130, TestSize.Level1)
{
    fd_ = OpenFile(g_mp3Path);
    size_ = GetFileSize(g_mp3Path);
    printf("---- %s ----\n", g_mp3Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "SLT");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.language, "language");
    ASSERT_EQ(formatVal_.duration, 30024000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.author, "author");
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.fileType, 203);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1131
 * @tc.desc: get format when the file is mp3
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1131, TestSize.Level1)
{
    fd_ = OpenFile(g_mp3Path);
    size_ = GetFileSize(g_mp3Path);
    printf("---- %s ----\n", g_mp3Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 64000);
#ifdef AVSOURCE_INNER_UNIT_TEST
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/mp3_48000_1_cover.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &addr_, buffSize_));
    fwrite(addr_, sizeof(uint8_t), buffSize_, saveFile);
    fclose(saveFile);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1140
 * @tc.desc: get format when the file is ogg
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1140, TestSize.Level1)
{
    fd_ = OpenFile(g_oggPath);
    size_ = GetFileSize(g_oggPath);
    printf("---- %s ----\n", g_oggPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/vorbis");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 80000);
}

/**
 * @tc.name: AVSource_GetFormat_1150
 * @tc.desc: get format when the file is wav
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1150, TestSize.Level1)
{
    fd_ = OpenFile(g_wavPath);
    size_ = GetFileSize(g_wavPath);
    printf("---- %s ----\n", g_wavPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.language, "language");
    ASSERT_EQ(formatVal_.duration, 30037333);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 207);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_1151
 * @tc.desc: get format when the file is wav
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1151, TestSize.Level1)
{
    fd_ = OpenFile(g_wavPath);
    size_ = GetFileSize(g_wavPath);
    printf("---- %s ----\n", g_wavPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.bitRate, 768000);
}

/**
 * @tc.name: AVSource_GetFormat_1160
 * @tc.desc: get format when the file is amr (amr_nb)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1160, TestSize.Level1)
{
    fd_ = OpenFile(g_amrPath);
    size_ = GetFileSize(g_amrPath);
    printf("---- %s ----\n", g_amrPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30020000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 201);
#endif
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 8000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/3gpp");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
}

/**
 * @tc.name: AVSource_GetFormat_1170
 * @tc.desc: get format when the file is amr (amr_wb)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1170, TestSize.Level1)
{
    fd_ = OpenFile(g_amrPath2);
    size_ = GetFileSize(g_amrPath2);
    printf("---- %s ----\n", g_amrPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_EQ(formatVal_.fileType, 201);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 16000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/amr-wb");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
}

/**
 * @tc.name: AVSource_GetFormat_1180
 * @tc.desc: get format when the file is audio vivid (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1180, TestSize.Level1)
{
    fd_ = OpenFile(g_audioVividPath);
    size_ = GetFileSize(g_audioVividPath);
    printf("---- %s ----\n", g_audioVividPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 32044000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_EQ(formatVal_.fileType, 101);
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
#endif
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.codecMime, CodecMimeType::AUDIO_AVS3DA);
    ASSERT_EQ(formatVal_.bitRate, 64082);
}

/**
 * @tc.name: AVSource_GetFormat_11901
 * @tc.desc: get format when the file is audio vivid (ts)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_11901, TestSize.Level1)
{
    fd_ = OpenFile(g_audioVividPath2);
    size_ = GetFileSize(g_audioVividPath2);
    printf("---- %s ----\n", g_audioVividPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 31718456);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_EQ(formatVal_.fileType, 102);
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
#endif
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.codecMime, CodecMimeType::AUDIO_AVS3DA);
    ASSERT_EQ(formatVal_.bitRate, 64000);
}
} // namespace