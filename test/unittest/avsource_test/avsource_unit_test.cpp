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

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
const int64_t SOURCE_OFFSET = 0;
uint8_t *g_addr = nullptr;
size_t g_buffSize = 0;
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

string g_mp4Uri = TEST_URI_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_mp4Uri3 = TEST_URI_PATH + string("test_mpeg2_B_Gop25_4sec.mp4");
string g_mp4Uri5 = TEST_URI_PATH + string("test_suffix_mismatch.mp4");
string g_mp4Uri6 = TEST_URI_PATH + string("test_empty_file.mp4");
string g_mp4Uri7 = TEST_URI_PATH + string("test_error.mp4");
string g_mp4Uri8 = TEST_URI_PATH + string("zero_track.mp4");
string g_mkvUri2 = TEST_URI_PATH + string("h264_opus_4sec.mkv");
string g_tsUri = TEST_URI_PATH + string("test_mpeg2_Gop25_4sec.ts");
string g_aacUri = TEST_URI_PATH + string("audio/aac_44100_1.aac");
string g_flacUri = TEST_URI_PATH + string("audio/flac_48000_1_cover.flac");
string g_m4aUri = TEST_URI_PATH + string("audio/m4a_48000_1.m4a");
string g_mp3Uri = TEST_URI_PATH + string("audio/mp3_48000_1_cover.mp3");
string g_oggUri = TEST_URI_PATH + string("audio/ogg_48000_1.ogg");
string g_wavUri = TEST_URI_PATH + string("audio/wav_48000_1.wav");
string g_amrUri = TEST_URI_PATH + string("audio/amr_nb_8000_1.amr");
string g_amrUri2 = TEST_URI_PATH + string("audio/amr_wb_16000_1.amr");
string g_audioVividUri = TEST_URI_PATH + string("2obj_44100Hz_16bit_32k.m4a");

struct FormatValue {
    // source format
    string title = "";
    string artist = "";
    string album = "";
    string albumArtist = "";
    string date = "";
    string comment = "";
    string genre = "";
    string copyright = "";
    string description = "";
    string language = "";
    string lyrics = "";
    int64_t duration = 0;
    int32_t trackCount = 0;
    string author = "";
    string composer = "";
    int32_t hasVideo = -1;
    int32_t hasAudio = -1;
    int32_t fileType = 0;
    // track format
    string codecMime = "";
    int32_t trackType = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t aacIsAdts = -1;
    int32_t sampleRate = 0;
    int32_t channelCount = 0;
    int64_t bitRate = 0;
    int32_t audioSampleFormat = 0;
    double frameRate = 0;
    int32_t rotationAngle = 0;
    int64_t channelLayout = 0;
    int32_t hdrType = 0;
    int32_t codecProfile = 0;
    int32_t codecLevel = 0;
    int32_t colorPrimaries = 0;
    int32_t transferCharacteristics = 0;
    int32_t rangeFlag = 0;
    int32_t matrixCoefficients = 0;
    int32_t chromaLocation = 0;
};
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
    g_addr = nullptr;
    g_buffSize = 0;
}

int64_t AVSourceUnitTest::GetFileSize(const string fileName)
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

int32_t AVSourceUnitTest::OpenFile(const string fileName)
{
    int32_t fd = open(fileName.c_str(), O_RDONLY);
    return fd;
}

/**********************************source FD**************************************/
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
    FormatValue formatValue;
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.duration, 4120000);
    ASSERT_EQ(formatValue.trackCount, 3);
    ASSERT_EQ(formatValue.hasVideo, 1);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.fileType, 101);
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
    FormatValue formatValue;
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.bitRate, 7782407);
    ASSERT_DOUBLE_EQ(formatValue.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatValue.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 128563);
    ASSERT_EQ(formatValue.aacIsAdts, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.channelLayout, 3);
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
    FormatValue formatValue;
    trackIndex_ = 2;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/test_264_B_Gop25_4sec_cover.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &g_addr, g_buffSize));
    fwrite(g_addr, sizeof(uint8_t), g_buffSize, saveFile);
    fclose(saveFile);
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
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatValue.codecMime, "video/mpeg2");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_DOUBLE_EQ(formatValue.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 127103);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.channelLayout, 3);
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
    printf("---- %s ----\n", g_mp4Path3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_EQ(formatValue.hasVideo, 1);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.fileType, 101);
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.author, "author");
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatValue.codecMime, "video/mpeg2");
    ASSERT_EQ(formatValue.bitRate, 3889231);
    ASSERT_DOUBLE_EQ(formatValue.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 128563);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.channelLayout, 3);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.albumArtist, "album_artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "copyRight");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.duration, 4001000);
    ASSERT_EQ(formatValue.trackCount, 2);
    ASSERT_EQ(formatValue.hasVideo, 1);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.fileType, 103);
    ASSERT_EQ(formatValue.language, "language");
    ASSERT_EQ(formatValue.author, "author");
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
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.frameRate, 60.000000);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.codecMime, "audio/opus");
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
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
    FormatValue formatValue;
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.duration, 30023469);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 202);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatValue.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.channelLayout, 3);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 126800);
    ASSERT_EQ(formatValue.aacIsAdts, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.author, "author");
    ASSERT_EQ(formatValue.duration, 30000000);
    ASSERT_EQ(formatValue.trackCount, 2);
    ASSERT_EQ(formatValue.fileType, 204);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
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
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/flac");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    const char* outFile = "/data/test/flac_48000_1_cover.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &g_addr, g_buffSize));
    fwrite(g_addr, sizeof(uint8_t), g_buffSize, saveFile);
    fclose(saveFile);
}

/**
 * @tc.name: AVSource_GetFormat_1120
 * @tc.desc: get format when the file is m4a
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1120, TestSize.Level1)
{
    fd_ = OpenFile(g_m4aPath);
    size_ = GetFileSize(g_m4aPath);
    printf("---- %s ----\n", g_m4aPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.duration, 30016000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 206);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
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
    FormatValue formatValue;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.bitRate, 69594);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.author, "author");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "SLT");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.language, "language");
    ASSERT_EQ(formatValue.duration, 30024000);
    ASSERT_EQ(formatValue.trackCount, 2);
    ASSERT_EQ(formatValue.fileType, 203);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
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
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mpeg");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.bitRate, 64000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/mp3_48000_1_cover.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &g_addr, g_buffSize));
    fwrite(g_addr, sizeof(uint8_t), g_buffSize, saveFile);
    fclose(saveFile);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_EQ(formatValue.duration, 30000000);
    ASSERT_EQ(formatValue.trackCount, 1);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/vorbis");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.bitRate, 80000);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.language, "language");
    ASSERT_EQ(formatValue.duration, 30037333);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 207);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
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
    FormatValue formatValue;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/raw");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatValue.bitRate, 768000);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.duration, 30020000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 201);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 8000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/3gpp");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_EQ(formatValue.duration, 30000000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 201);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 16000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/amr-wb");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
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
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_EQ(formatValue.duration, 32044000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 101);
    ASSERT_EQ(formatValue.hasVideo, 0);
    ASSERT_EQ(formatValue.hasAudio, 1);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 3);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.codecMime, "audio/avs-3da");
    ASSERT_EQ(formatValue.bitRate, 64082);
}

/**
 * @tc.name: AVSource_GetFormat_1190
 * @tc.desc: get format when the file is audio vivid (ts)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1190, TestSize.Level1)
{
    fd_ = OpenFile(g_audioVividPath2);
    size_ = GetFileSize(g_audioVividPath2);
    printf("---- %s ----\n", g_audioVividPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_EQ(formatValue.duration, 31718456);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 102);
    ASSERT_EQ(formatValue.hasVideo, 0);
    ASSERT_EQ(formatValue.hasAudio, 1);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 3);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.codecMime, "audio/avs-3da");
    ASSERT_EQ(formatValue.bitRate, 64000);
}

/**********************************source URI**************************************/
/**
 * @tc.name: AVSource_CreateSourceWithURI_1000
 * @tc.desc: create source with uri, mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1000, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1020
 * @tc.desc: create source with uri, but file is abnormal
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1020, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri5.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri5.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1030
 * @tc.desc: create source with uri, but file is empty
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1030, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri6.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri6.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1040
 * @tc.desc: create source with uri, but file is error
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1040, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri7.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri7.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1050
 * @tc.desc: create source with uri, but track is zero
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1050, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri8.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri8.data()));
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1060
 * @tc.desc: create source with invalid uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1060, TestSize.Level1)
{
    string uri = "http://127.0.0.1:46666/asdffafafaf";
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(uri.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1070
 * @tc.desc: Create source repeatedly
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1070, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1080
 * @tc.desc: destroy source
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1080, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    printf("---- %s ------\n", g_mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    shared_ptr<AVSourceMock> source2 = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp3Uri.data()));
    ASSERT_NE(source2, nullptr);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
    ASSERT_EQ(source2->Destroy(), AV_ERR_OK);
    source2 = nullptr;
}

/**
 * @tc.name: AVSource_GetFormat_2000
 * @tc.desc: get source format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2000, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.duration, 4120000);
    ASSERT_EQ(formatValue.trackCount, 3);
    ASSERT_EQ(formatValue.hasVideo, 1);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.fileType, 101);
}

/**
 * @tc.name: AVSource_GetFormat_2010
 * @tc.desc: get track format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2010, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    FormatValue formatValue;
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.bitRate, 7782407);
    ASSERT_DOUBLE_EQ(formatValue.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatValue.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 128563);
    ASSERT_EQ(formatValue.aacIsAdts, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2011
 * @tc.desc: get track format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2011, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 2;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    FormatValue formatValue;
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    const char* outFile = "/data/test/test_264_B_Gop25_4sec_cover_uri.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &g_addr, g_buffSize));
    fwrite(g_addr, sizeof(uint8_t), g_buffSize, saveFile);
    fclose(saveFile);
}

/**
 * @tc.name: AVSource_GetFormat_2020
 * @tc.desc: get source format when the file is ts
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2020, TestSize.Level1)
{
    printf("---- %s ------\n", g_tsUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_tsUri.data()));
    ASSERT_NE(source_, nullptr);
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatValue.codecMime, "video/mpeg2");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_DOUBLE_EQ(formatValue.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 127103);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2030
 * @tc.desc: get source format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2030, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri3.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri3.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_EQ(formatValue.hasVideo, 1);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.fileType, 101);
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.author, "author");
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatValue.codecMime, "video/mpeg2");
    ASSERT_EQ(formatValue.bitRate, 3889231);
    ASSERT_DOUBLE_EQ(formatValue.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 128563);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2050
 * @tc.desc: get format when the file is mkv (video: h264, audio: opus)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2050, TestSize.Level1)
{
    printf("---- %s ------\n", g_mkvUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mkvUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.albumArtist, "album_artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "copyRight");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.duration, 4001000);
    ASSERT_EQ(formatValue.trackCount, 2);
    ASSERT_EQ(formatValue.hasVideo, 1);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.fileType, 103);
    ASSERT_EQ(formatValue.language, "language");
    ASSERT_EQ(formatValue.author, "author");
}

/**
 * @tc.name: AVSource_GetFormat_2060
 * @tc.desc: get format when the file is mkv (video: h264, audio: opus)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2060, TestSize.Level1)
{
    printf("---- %s ------\n", g_mkvUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mkvUri2.data()));
    ASSERT_NE(source_, nullptr);
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.frameRate, 60.000000);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.codecMime, "audio/opus");
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_2100
 * @tc.desc: get format when the file is aac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2100, TestSize.Level1)
{
    printf("---- %s ------\n", g_aacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_aacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    FormatValue formatValue;
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.duration, 30023469);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 202);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatValue.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.channelLayout, 3);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.bitRate, 126800);
    ASSERT_EQ(formatValue.aacIsAdts, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}


/**
 * @tc.name: AVSource_GetFormat_2110
 * @tc.desc: get format when the file is flac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2110, TestSize.Level1)
{
    printf("---- %s ------\n", g_flacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_flacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.author, "author");
    ASSERT_EQ(formatValue.duration, 30000000);
    ASSERT_EQ(formatValue.trackCount, 2);
    ASSERT_EQ(formatValue.fileType, 204);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
}

/**
 * @tc.name: AVSource_GetFormat_2111
 * @tc.desc: get format when the file is flac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2111, TestSize.Level1)
{
    printf("---- %s ------\n", g_flacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_flacUri.data()));
    ASSERT_NE(source_, nullptr);
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/flac");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    const char* outFile = "/data/test/flac_48000_1_uri.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &g_addr, g_buffSize));
    fwrite(g_addr, sizeof(uint8_t), g_buffSize, saveFile);
    fclose(saveFile);
}

/**
 * @tc.name: AVSource_GetFormat_2120
 * @tc.desc: get format when the file is m4a
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2120, TestSize.Level1)
{
    printf("---- %s ------\n", g_m4aUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "lyrics");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.duration, 30016000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 206);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
}

/**
 * @tc.name: AVSource_GetFormat_2121
 * @tc.desc: get format when the file is m4a
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2121, TestSize.Level1)
{
    printf("---- %s ------\n", g_m4aUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    FormatValue formatValue;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.bitRate, 69594);
}

/**
 * @tc.name: AVSource_GetFormat_2130
 * @tc.desc: get format when the file is mp3
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2130, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp3Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatValue.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatValue.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatValue.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatValue.author));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatValue.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.albumArtist, "album artist");
    ASSERT_EQ(formatValue.author, "author");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.lyrics, "SLT");
    ASSERT_EQ(formatValue.composer, "composer");
    ASSERT_EQ(formatValue.description, "description");
    ASSERT_EQ(formatValue.language, "language");
    ASSERT_EQ(formatValue.duration, 30024000);
    ASSERT_EQ(formatValue.trackCount, 2);
    ASSERT_EQ(formatValue.fileType, 203);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
}

/**
 * @tc.name: AVSource_GetFormat_2131
 * @tc.desc: get format when the file is mp3
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2131, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp3Uri.data()));
    ASSERT_NE(source_, nullptr);
    FormatValue formatValue;
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/mpeg");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.bitRate, 64000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/mp3_48000_1_cover_uri.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &g_addr, g_buffSize));
    fwrite(g_addr, sizeof(uint8_t), g_buffSize, saveFile);
    fclose(saveFile);
}

/**
 * @tc.name: AVSource_GetFormat_2140
 * @tc.desc: get format when the file is ogg
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2140, TestSize.Level1)
{
    printf("---- %s ------\n", g_oggUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_oggUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_EQ(formatValue.duration, 30000000);
    ASSERT_EQ(formatValue.trackCount, 1);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/vorbis");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.bitRate, 80000);
}

/**
 * @tc.name: AVSource_GetFormat_2150
 * @tc.desc: get format when the file is wav
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2150, TestSize.Level1)
{
    printf("---- %s ------\n", g_wavUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_wavUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatValue.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatValue.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatValue.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatValue.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatValue.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatValue.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatValue.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatValue.language));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.title, "title");
    ASSERT_EQ(formatValue.artist, "artist");
    ASSERT_EQ(formatValue.album, "album");
    ASSERT_EQ(formatValue.date, "2023");
    ASSERT_EQ(formatValue.comment, "comment");
    ASSERT_EQ(formatValue.genre, "genre");
    ASSERT_EQ(formatValue.copyright, "Copyright");
    ASSERT_EQ(formatValue.language, "language");
    ASSERT_EQ(formatValue.duration, 30037333);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 207);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
}

/**
 * @tc.name: AVSource_GetFormat_2151
 * @tc.desc: get format when the file is wav
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2151, TestSize.Level1)
{
    printf("---- %s ------\n", g_wavUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_wavUri.data()));
    ASSERT_NE(source_, nullptr);
    FormatValue formatValue;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/raw");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatValue.bitRate, 768000);
}

/**
 * @tc.name: AVSource_GetFormat_2160
 * @tc.desc: get format when the file is amr
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2160, TestSize.Level1)
{
    printf("---- %s ------\n", g_amrUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_amrUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_EQ(formatValue.duration, 30020000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 201);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 8000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/3gpp");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
}

/**
 * @tc.name: AVSource_GetFormat_2170
 * @tc.desc: get format when the file is amr
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2170, TestSize.Level1)
{
    printf("---- %s ------\n", g_amrUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_amrUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_EQ(formatValue.duration, 30000000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 201);
    ASSERT_EQ(formatValue.hasAudio, 1);
    ASSERT_EQ(formatValue.hasVideo, 0);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 4);
    ASSERT_EQ(formatValue.sampleRate, 16000);
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.codecMime, "audio/amr-wb");
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
}

/**
 * @tc.name: AVSource_GetFormat_2180
 * @tc.desc: get format when the file is audio vivid (m4a)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2180, TestSize.Level1)
{
    printf("---- %s ------\n", g_audioVividUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_audioVividUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    FormatValue formatValue;
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatValue.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatValue.trackCount));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatValue.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatValue.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatValue.hasAudio));
    ASSERT_EQ(formatValue.duration, 32044000);
    ASSERT_EQ(formatValue.trackCount, 1);
    ASSERT_EQ(formatValue.fileType, 206);
    ASSERT_EQ(formatValue.hasVideo, 0);
    ASSERT_EQ(formatValue.hasAudio, 1);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatValue.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatValue.channelLayout, 3);
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.codecMime, "audio/avs-3da");
    ASSERT_EQ(formatValue.bitRate, 64082);
}