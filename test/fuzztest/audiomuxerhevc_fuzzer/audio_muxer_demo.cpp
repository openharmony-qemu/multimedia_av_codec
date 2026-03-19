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

#include "audio_muxer_demo.h"
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "native_avformat.h"
#include "demo_log.h"
#include "avcodec_codec_name.h"
#include "native_avbuffer.h"
#include "ffmpeg_converter.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;

constexpr int32_t WIDTH_720 = 720;
constexpr int32_t HEIGHT_480 = 480;
constexpr int32_t WIDTH_3840 = 3840;
constexpr int32_t HEIGHT_2160 = 2160;
constexpr int32_t COLOR_TRANSFER_2 = 2;
constexpr int32_t COLOR_TRANSFER_18 = 18;
constexpr int32_t FRAME_RATE_30 = 30;
constexpr int32_t FRAME_RATE_60 = 60;
constexpr int32_t FRAME_SIZE_1024 = 1024;
constexpr int32_t COLOR_PRIMARIES_2 = 2;
constexpr int32_t COLOR_PRIMARIES_9 = 9;
constexpr int32_t COLOR_MATRIXCIEFF_2 = 2;
constexpr int32_t COLOR_MATRIXCIEFF_9 = 9;
constexpr int32_t VIDEO_DELAY_2 = 2;

constexpr int32_t CHANNELS_2 = 2;

constexpr int32_t INFO_PTS_100 = 100;
constexpr int32_t INFO_SIZE_1024 = 1024;

constexpr int32_t SAMPLE_RATE_8000 = 8000;
constexpr int32_t SAMPLE_RATE_16000 = 16000;
constexpr int32_t SAMPLE_RATE_44100 = 44100;

constexpr const char* MP4_OUTPUT_FILE_PATH = "/data/test/media/muxer_MP4_outputfile.mp4";
constexpr const char* AMRNB_OUTPUT_FILE_PATH = "/data/test/media/muxer_AMRNB_outputfile.amr";
constexpr const char* AMRWB_OUTPUT_FILE_PATH = "/data/test/media/muxer_AMRWB_outputfile.amr";
constexpr const char* M4A_OUTPUT_FILE_PATH = "/data/test/media/muxer_M4A_outputfile.m4a";
constexpr const char* MP3_OUTPUT_FILE_PATH = "/data/test/media/muxer_MP3_outputfile.mp3";

AVMuxerDemo::AVMuxerDemo()
{
}
AVMuxerDemo::~AVMuxerDemo()
{
    if (avmuxer_) {
        OH_AVMuxer_Destroy(avmuxer_);
        avmuxer_ = nullptr;
    }
    if (avbuffer) {
        OH_AVBuffer_Destroy(avbuffer);
        avbuffer = nullptr;
    }
    if (outputFd_ > -1) {
        close(outputFd_);
        outputFd_ = -1;
    }
}

bool AVMuxerDemo::InitFile(const std::string& inputFile)
{
    if (inputFile.find("aac") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_AAC;
        output_path_ = MP4_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_M4A;
    } else if (inputFile.find("h264") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_H264;
        output_path_ = MP4_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("h265") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_H265;
        output_path_ = MP4_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("mpeg4") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_MPEG4;
        output_path_ = MP4_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("hdr") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_HDR;
        output_path_ = MP4_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("jpg") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_JPG;
        output_path_ = M4A_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("amrwb") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_AMRWB;
        output_path_ = AMRWB_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_AMR;
    } else if (inputFile.find("amrnb") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_AMRNB;
        output_path_ = AMRNB_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_AMR;
    } else if (inputFile.find("mpeg3") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_MPEG3;
        output_path_ = MP3_OUTPUT_FILE_PATH;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MP3;
    }
    return true;
}

void AVMuxerDemo::SetParameter(const uint8_t *data, size_t size)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetFloatValue(format, "latitude", 10.0);
    OH_AVFormat_SetFloatValue(format, "longitude", 20.0);
    OH_AVFormat_SetFloatValue(format, "altitude", 30.0);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_COMMENT, "fuzz_test_comment");
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_TITLE, "fuzz_test_title");
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_ARTIST, "fuzz_test_artist");
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_LYRICS, "fuzz_test_artist_lyrics");
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_GENRE, "fuzz_test_artist_genre");
    OH_AVFormat_SetIntValue(format, "com.openharmony.version", 5); // 5 test version
    OH_AVFormat_SetStringValue(format, "com.openharmony.model", "LNA-AL00");
    OH_AVFormat_SetDoubleValue(format, "com.openharmony.capture.fps", 30.00f); // 30.00f test capture fps
    OH_AVFormat_SetBuffer(format, "com.openharmony.capture.test.buffer", data, size);
    OH_AVMuxer_SetFormat(avmuxer_, format);
}

bool AVMuxerDemo::RunCase(const uint8_t *data, size_t size)
{
    std::string codecdata(reinterpret_cast<const char *>(data), size);
    inputdata = codecdata;
    inputdatasize = size;
    std::string outputFile(output_path_);
	//Create
    avmuxer_ = Create();
    DEMO_CHECK_AND_RETURN_RET_LOG(avmuxer_ != nullptr, false, "Fatal: Create fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(SetRotation(avmuxer_, 0) == AVCS_ERR_OK, false, "Fatal: SetRotation fail");
    SetParameter(data, size);
    //获取param
    AudioTrackParam param = InitFormatParam(audioType_);
    int32_t trackIndex = -1;
    //添加轨
    int32_t res = 0;
    if (param.isNeedCover) {
        res = AddCoverTrack(avmuxer_, trackIndex, param);
        DEMO_CHECK_AND_RETURN_RET_LOG(res == AVCS_ERR_OK, false, "Fatal: AddTrack fail");
    } else {
        res = AddTrack(avmuxer_, trackIndex, param);
        DEMO_CHECK_AND_RETURN_RET_LOG(res == AVCS_ERR_OK, false, "Fatal: AddTrack fail");
    }
    //Start
    DEMO_CHECK_AND_RETURN_RET_LOG(Start() == AVCS_ERR_OK, false, "Fatal: Start fail");
    //写数据
    if (param.isNeedCover) {
        WriteTrackCover(avmuxer_, trackIndex);
    } else {
        WriteSingleTrackSampleAVBuffer(avmuxer_, trackIndex);
    }
    //Stop
    DEMO_CHECK_AND_RETURN_RET_LOG(Stop() == AVCS_ERR_OK, false, "Fatal: Stop fail");
    //Destroy
    DEMO_CHECK_AND_RETURN_RET_LOG(Destroy() == AVCS_ERR_OK, false, "Fatal: Destroy fail");
    return true;
}

AudioTrackParam AVMuxerDemo::InitFormatParam(AudioMuxerFormatType type)
{
    AudioTrackParam param;
    param.sampleRate = SAMPLE_RATE_44100;
    param.channels = CHANNELS_2;
    param.frameSize = FRAME_SIZE_1024;
    param.width = WIDTH_720;
    param.height = HEIGHT_480;
    param.isNeedCover = false;
    param.frameRate = FRAME_RATE_60;
    param.videoDelay = 0;
    param.colorPrimaries = COLOR_PRIMARIES_2;
    param.colorTransfer = COLOR_TRANSFER_2;
    param.colorMatrixCoeff = COLOR_MATRIXCIEFF_2;
    param.colorRange = 0;
    param.isHdrVivid = 0;
    param.isNeedVideo = false;
    if (audioType_ == AudioMuxerFormatType::TYPE_AAC) {
        param.mimeType = OH_AVCODEC_MIMETYPE_AUDIO_AAC;
    } else if (audioType_ == AudioMuxerFormatType::TYPE_H264) {
        param.mimeType = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    } else if (audioType_ == AudioMuxerFormatType::TYPE_H265) {
        param.mimeType = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        param.videoDelay = VIDEO_DELAY_2;
    } else if (audioType_ == AudioMuxerFormatType::TYPE_MPEG3 || audioType_ == AudioMuxerFormatType::TYPE_MPEG4) {
        param.mimeType = OH_AVCODEC_MIMETYPE_AUDIO_MPEG;
    } else if (audioType_ == AudioMuxerFormatType::TYPE_HDR) {
        param.mimeType = OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
        param.width = WIDTH_3840;
        param.height = HEIGHT_2160;
        param.frameRate = FRAME_RATE_30;
        param.colorPrimaries = COLOR_PRIMARIES_9;
        param.colorTransfer = COLOR_TRANSFER_18;
        param.colorMatrixCoeff = COLOR_MATRIXCIEFF_9;
        param.isHdrVivid = 1;
    } else if (audioType_ == AudioMuxerFormatType::TYPE_JPG) {
        param.mimeType = OH_AVCODEC_MIMETYPE_IMAGE_JPG;
        param.isNeedCover = true;
    } else if (audioType_ == AudioMuxerFormatType::TYPE_AMRNB) {
        param.mimeType = OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB;
        param.sampleRate = SAMPLE_RATE_8000;
        param.channels = 1;
    } else if (audioType_ == AudioMuxerFormatType::TYPE_AMRWB) {
        param.mimeType = OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB;
        param.sampleRate = SAMPLE_RATE_16000;
        param.channels = 1;
    }
    return param;
}

OH_AVMuxer* AVMuxerDemo::Create()
{
    std::string outputFile(output_path_);
    outputFd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (outputFd_ < 0) {
        std::cout << "open file failed! outputFd_ is:" << outputFd_ << std::endl;
        return nullptr;
    }
    return OH_AVMuxer_Create(outputFd_, output_format_);
}

int32_t AVMuxerDemo::Start()
{
    return OH_AVMuxer_Start(avmuxer_);
}

int32_t AVMuxerDemo::Stop()
{
    return OH_AVMuxer_Stop(avmuxer_);
}

int32_t AVMuxerDemo::Destroy()
{
    if (avbuffer != nullptr) {
        OH_AVBuffer_Destroy(avbuffer);
        avbuffer = nullptr;
    }
    int32_t res = 0;
    if (avmuxer_ != nullptr) {
        res = OH_AVMuxer_Destroy(avmuxer_);
        avmuxer_ = nullptr;
    }
    return res;
}

int32_t AVMuxerDemo::SetRotation(OH_AVMuxer* muxer, int32_t rotation)
{
    return OH_AVMuxer_SetRotation(muxer, rotation);
}

int32_t AVMuxerDemo::AddTrack(OH_AVMuxer* muxer, int32_t& trackIndex, AudioTrackParam param)
{
    const int configBufferSize =  0x1FFF;
    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    if (trackFormat == NULL) {
        std::cout << "AddTrack: format failed!" << std::endl;
        return AV_ERR_INVALID_VAL;
    }
    // set codec config
    // 此处可以不设CONFIG
    int extraSize = inputdatasize > configBufferSize ? configBufferSize : inputdatasize;
    unsigned char buffer[configBufferSize] = {0};
    errno_t res = 0;
    res = strncpy_s(reinterpret_cast<char*>(buffer), configBufferSize, inputdata.c_str(), extraSize);
    if (res != 0) {
        return res;
    }
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, param.mimeType);
    // audio
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, param.sampleRate);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, param.channels);
    OH_AVFormat_SetIntValue(trackFormat, "audio_samples_per_frame", param.frameSize);
    //video
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, param.width);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, param.height);
    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, param.frameRate);
    OH_AVFormat_SetIntValue(trackFormat, "video_delay", param.videoDelay);
    // hdr vivid
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_COLOR_PRIMARIES, param.colorPrimaries);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS, param.colorTransfer);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_MATRIX_COEFFICIENTS, param.colorMatrixCoeff);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_RANGE_FLAG, param.colorRange);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, param.isHdrVivid);
    int32_t ret = OH_AVMuxer_AddTrack(muxer, &trackIndex, trackFormat);
    return ret;
}


int32_t AVMuxerDemo::AddCoverTrack(OH_AVMuxer* muxer, int32_t& trackId, AudioTrackParam param)
{
    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    if (trackFormat == NULL) {
        std::cout << "format failed!" << std::endl;
        return AV_ERR_INVALID_VAL;
    }
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, param.mimeType);
    // audio
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, param.sampleRate);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, param.channels);
    OH_AVFormat_SetIntValue(trackFormat, "audio_samples_per_frame", param.frameSize);
    //video
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, param.width);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, param.height);
    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, param.frameRate);
    OH_AVFormat_SetIntValue(trackFormat, "video_delay", param.videoDelay);
    // hdr vivid
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_COLOR_PRIMARIES, param.colorPrimaries);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS, param.colorTransfer);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_MATRIX_COEFFICIENTS, param.colorMatrixCoeff);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_RANGE_FLAG, param.colorRange);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, param.isHdrVivid);
    int32_t ret = OH_AVMuxer_AddTrack(muxer, &trackId, trackFormat);
    return ret;
}

void AVMuxerDemo::WriteTrackCover(OH_AVMuxer *muxer, int32_t trackIndex)
{
    OH_AVCodecBufferAttr info;
    memset_s(&info, sizeof(info), 0, sizeof(info));
    info.size = inputdatasize;
    OH_AVBuffer *av_buffer = OH_AVBuffer_Create(info.size);
    if (av_buffer == NULL) {
        std::cout << "WriteTrackCover: create OH_AVMemory error!" << std::endl;
        return;
    }
    size_t frameBytes = 1024;
    size_t cSize = inputdatasize < frameBytes ? inputdatasize : frameBytes;
    errno_t res = 0;
    res = strncpy_s(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(av_buffer)), cSize, inputdata.c_str(), cSize);
    if (res != 0) {
        return;
    }
    if (OH_AVMuxer_WriteSampleBuffer(muxer, trackIndex, av_buffer) != AV_ERR_OK) {
        OH_AVBuffer_Destroy(av_buffer);
        av_buffer = nullptr;
        std::cout << "OH_AVMuxer_WriteSample error!" << std::endl;
        return;
    }
    if (av_buffer != nullptr) {
        OH_AVBuffer_Destroy(av_buffer);
        av_buffer = nullptr;
    }
}

void AVMuxerDemo::WriteSingleTrackSampleAVBuffer(OH_AVMuxer *muxer, int32_t trackIndex)
{
    if (muxer == NULL || trackIndex  < 0) {
        std::cout << "WriteSingleTrackSample muxer is null " << trackIndex << std::endl;
        return;
    }
    OH_AVCodecBufferAttr info;
    OH_AVBuffer *buffer = NULL;
    memset_s(&info, sizeof(info), 0, sizeof(info));
    bool ret = UpdateWriteBufferInfoAVBuffer(&buffer, &info);
	//此处只执行一次，正常要根据内容进行while循环将buffer所有数据写入，因为fuzz数据是假数据，故仅调用一次；
    if (ret) {
        OH_AVMuxer_WriteSampleBuffer(muxer, trackIndex, buffer);
    }
    if (buffer != NULL) {
        OH_AVBuffer_Destroy(buffer);
        buffer = nullptr;
    }
}

bool AVMuxerDemo::UpdateWriteBufferInfoAVBuffer(OH_AVBuffer **buffer, OH_AVCodecBufferAttr *info)
{
    //此处不区分音频类型，所有pts、size、flags均取一样的值
    info->pts = INFO_PTS_100;
    info->size = INFO_SIZE_1024;
    info->flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (buffer == NULL || info == NULL) {
        return false;
    }
    if (*buffer != NULL) {
        std::cout << "UpdateWriteBufferInfoAVBuffer: buffer is NULL!" << std::endl;
        OH_AVBuffer_Destroy(*buffer);
        *buffer = NULL;
    }
    if (*buffer == NULL) {
        *buffer = OH_AVBuffer_Create(info->size);
    }
    if (*buffer == NULL) {
        std::cout << "UpdateWriteBufferInfoAVBuffer: error create OH_AVMemory! " << std::endl;
        return false;
    }
    errno_t res = 0;
    res = strncpy_s(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(*buffer)), info->size, inputdata.c_str(), info->size);
    if (res != 0) {
        return false;
    }
    OH_AVBuffer_SetBufferAttr(*buffer, info);
    return true;
}