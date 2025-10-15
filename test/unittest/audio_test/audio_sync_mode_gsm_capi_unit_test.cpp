/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <vector>
#include <queue>
#include <mutex>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include <fcntl.h>

#include "securec.h"
#include "common/native_mfmagic.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_mime_type.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "native_avbuffer.h"
#include "native_avcodec_audiocodec.h"
#include "native_audio_channel_layout.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

namespace {
constexpr int32_t DEFAULT_CHANNELS = 1;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 8000;
constexpr int32_t CHANNEL_COUNT = 2;
constexpr int32_t SAMPLE_RATE = 44100;
constexpr int SAMPLERATE = 8000;
constexpr int SAMPLES_PER_FRAME = 160;
constexpr string_view GSM_FILE_TODEMUX = "/data/test/media/8000_mono_gsm.mov";
constexpr string_view OUTPUT_GSM_PCM_FILE_PATH = "/data/test/media/test_decoder_gsm.pcm";
}

namespace OHOS {
namespace MediaAVCodec {

class AudioSyncModeCapiUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    bool DecoderFillInputBuffer(OH_AVBuffer *buffer);
    void DecoderRun();
    int32_t InitFile();
    int64_t GetFileSize(const char *fileName);
    int32_t Configure();
    bool ReadBuffer(OH_AVBuffer *buffer, uint32_t index);
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    
protected:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> outThread_;
    OH_AVCodec *codec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    std::unique_ptr<std::ifstream> inputFile_;
    std::unique_ptr<std::ofstream> outputFile_;
    int32_t channels_ = CHANNEL_COUNT;
    int32_t sampleRate_ = SAMPLE_RATE;
    uint32_t inputFrameBytes_ = 8192;
    uint32_t inputSizeCnt_ = 0;
    uint32_t decodeInputFrameCnt_ = 0;
    uint32_t outputFrameCnt_ = 0;
    uint32_t outputFormatChangedTimes = 0;
    int32_t outputSampleRate = 0;
    int32_t outputChannels = 0;
    int64_t inTimeout_ = 20000; // 20000us: 20ms
    int64_t outTimeout_ = 20000; // 20000us: 20ms
    uint32_t frameCount_ = 0;
    OH_AVDemuxer *demuxer = nullptr;
    OH_AVSource *source = nullptr;
};

void AudioSyncModeCapiUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioSyncModeCapiUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioSyncModeCapiUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioSyncModeCapiUnitTest::TearDown(void)
{
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
    if (inputFile_ != nullptr) {
        if (inputFile_->is_open()) {
            inputFile_->close();
        }
    }
    if (outputFile_ != nullptr) {
        if (outputFile_->is_open()) {
            outputFile_->close();
        }
    }
}

void AudioSyncModeCapiUnitTest::SetEOS(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t res = OH_AudioCodec_PushInputBuffer(codec_, index);
    cout << "OH_AudioCodec_PushInputBuffer    EOS   res: " << res << endl;
}

bool AudioSyncModeCapiUnitTest::ReadBuffer(OH_AVBuffer *buffer, uint32_t index)
{
    OH_AVCodecBufferAttr attr;
    OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, buffer);
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        SetEOS(index, buffer);
        return false;
    }
    buffer->buffer_->pts_ = frameCount_ * SAMPLES_PER_FRAME * 1000000LL / SAMPLERATE;
    buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;

    return true;
}

bool AudioSyncModeCapiUnitTest::DecoderFillInputBuffer(OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    memset_s(&attr, sizeof(attr), 0, sizeof(attr));
    bool finish = true;
    uint32_t index = 0;

    if (ReadBuffer(buffer, index)) {
        OH_AVBuffer_GetBufferAttr(buffer, &attr);
        if (attr.size > 0) {
            attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            decodeInputFrameCnt_++;
            finish = false;
        }
    } else {
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    }

    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(buffer, &attr), AV_ERR_OK);
    return finish;
}

void AudioSyncModeCapiUnitTest::DecoderRun()
{
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);
    bool inputEnd = false;
    OH_AVErrCode ret = AV_ERR_OK;
    for (;;) {
        uint32_t index = 0;
        if (!inputEnd) {
            ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
            if (ret == AV_ERR_TRY_AGAIN_LATER) {
                continue;
            }
            ASSERT_EQ(ret, AV_ERR_OK);
            OH_AVBuffer *inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
            ASSERT_NE(inputBuf, nullptr);

            inputEnd = DecoderFillInputBuffer(inputBuf);
            EXPECT_EQ(OH_AudioCodec_PushInputBuffer(codec_, index), AV_ERR_OK);
        }

        ret = OH_AudioCodec_QueryOutputBuffer(codec_, &index, outTimeout_);
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            cout << "output index get timeout" << endl;
            continue;
        } else if (ret == AV_ERR_STREAM_CHANGED) {
            OH_AVFormat *outFormat = OH_AudioCodec_GetOutputDescription(codec_);
            outputFormatChangedTimes++;
            OH_AVFormat_GetIntValue(outFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &outputChannels);
            OH_AVFormat_GetIntValue(outFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &outputSampleRate);
            continue;
        }
        EXPECT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *outputBuf = OH_AudioCodec_GetOutputBuffer(codec_, index);
        ASSERT_NE(outputBuf, nullptr);
        OH_AVCodecBufferAttr attr;
        memset_s(&attr, sizeof(attr), 0, sizeof(attr));
        ASSERT_EQ(OH_AVBuffer_GetBufferAttr(outputBuf, &attr), AV_ERR_OK);

        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "output eos" << endl;
            ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
            break;
        }

        outputFrameCnt_++;
        outputFile_->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(outputBuf)), attr.size);
        ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
    }
}

int64_t AudioSyncModeCapiUnitTest::GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

int32_t AudioSyncModeCapiUnitTest::InitFile()
{
    inputFile_ = std::make_unique<std::ifstream>(GSM_FILE_TODEMUX.data(), std::ios::binary);
    if (!inputFile_->is_open()) {
        cout << "Fatal: open input file failed:" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_GSM_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    if (!outputFile_->is_open()) {
        cout << "Fatal: open output file failed" << endl;
        inputFile_->close();
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    int fd = open(GSM_FILE_TODEMUX.data(), O_RDONLY);
    int64_t size = GetFileSize(GSM_FILE_TODEMUX.data());
    cout << GSM_FILE_TODEMUX.data() << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        cout << "Fatal: source is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (demuxer == nullptr) {
        cout << "Fatal: demuxer is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    OH_AVErrCode ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: OH_AVDemuxer_SelectTrackByID is fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioSyncModeCapiUnitTest::Configure()
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    uint32_t bitRate = 13000;
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNELS);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    return OH_AudioCodec_Configure(codec_, format_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, gsm_decode_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile());

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_GSM, false);
    ASSERT_NE(codec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, gsm_InvalidSizeTest, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile());

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_GSM, false);
    ASSERT_NE(codec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);

    OH_AVBuffer *inputBuf = nullptr;
    uint32_t index = 0;
    OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
    ASSERT_EQ(ret, AV_ERR_OK);
    inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
    ASSERT_NE(inputBuf, nullptr);

    OH_AVCodecBufferAttr attr;
    memset_s(&attr, sizeof(attr), 0, sizeof(attr));

    attr.size = -1;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, &attr), AV_ERR_INVALID_VAL);

    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, gsm_InvalidOffsetTest, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile());

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_GSM, false);
    ASSERT_NE(codec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);
    
    OH_AVBuffer *inputBuf = nullptr;
    uint32_t index = 0;
    OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
    ASSERT_EQ(ret, AV_ERR_OK);
    inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
    ASSERT_NE(inputBuf, nullptr);

    OH_AVCodecBufferAttr attr;
    memset_s(&attr, sizeof(attr), 0, sizeof(attr));

    attr.offset = -1;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, &attr), AV_ERR_INVALID_VAL);

    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, gsm_InvalidAttrTest, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile());

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_GSM, false);
    ASSERT_NE(codec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);

    OH_AVBuffer *inputBuf = nullptr;
    uint32_t index = 0;
    OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
    ASSERT_EQ(ret, AV_ERR_OK);
    inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
    ASSERT_NE(inputBuf, nullptr);

    OH_AVCodecBufferAttr *attr = nullptr;
    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, attr), AV_ERR_INVALID_VAL);

    OH_AudioCodec_Destroy(codec_);
}
}
}
