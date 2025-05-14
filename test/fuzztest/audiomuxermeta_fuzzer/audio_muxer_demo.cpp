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

#include "demo_log.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "native_avformat.h"
#include "native_avbuffer.h"
#include "ffmpeg_converter.h"
#include "media_description.h"
#include "avcodec_codec_name.h"
#include <fuzzer/FuzzedDataProvider.h>

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

constexpr int32_t SAMPLE_RATE_8000 = 8000;
constexpr int32_t SAMPLE_RATE_16000 = 16000;
constexpr int32_t SAMPLE_RATE_44100 = 44100;

constexpr const char* INPUT_AUDIO_FILE_PATH = "/data/test/media/aac_44100_2.bin";

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
    if (avMuxer_) {
        OH_AVMuxer_Destroy(avMuxer_);
        avMuxer_ = nullptr;
    }
    if (outputFd_ > -1) {
        close(outputFd_);
        outputFd_ = -1;
    }
    if (inputFd_ > -1) {
        close(inputFd_);
        inputFd_ = -1;
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
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("h265") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_H265;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("mpeg4") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_MPEG4;
        output_format_ = OH_AVOutputFormat::AV_OUTPUT_FORMAT_MPEG_4;
    } else if (inputFile.find("hdr") != std::string::npos) {
        audioType_ = AudioMuxerFormatType::TYPE_HDR;
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
bool AVMuxerDemo::RunCase(const uint8_t *data, size_t size)
{
    std::string codecdata(reinterpret_cast<const char *>(data), size);
    inputData_ = codecdata;
    inputDataSize_ = size;
    std::string outputFile(output_path_);
    // 1.读取音频文件
    inputFd_ = open(INPUT_AUDIO_FILE_PATH, O_RDONLY);
    if (inputFd_ < 0) {
        std::cout << "open file failed!!!" << std::endl;
        return false;
    }
	// 2.Create
    avMuxer_ = Create();
    DEMO_CHECK_AND_RETURN_RET_LOG(avMuxer_ != nullptr, false, "Fatal: Create fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(SetRotation(avMuxer_, 0) == AVCS_ERR_OK, false, "Fatal: SetRotation fail");
    // 3.获取param
    AudioTrackParam param = InitFormatParam(audioType_);
    int32_t trackIndex = -1;
    // 4.添加 UserMeta
    int32_t retSet = SetFormat(data, size);
    if (retSet == AV_ERR_OK) {
        std::cout << "Add UserMeta Success!!!" << std::endl;
    }
    // 5.添加轨
    int32_t res = 0;
    res = AddTrack(avMuxer_, trackIndex, param);
    DEMO_CHECK_AND_RETURN_RET_LOG(res == AVCS_ERR_OK, false, "Fatal: AddTrack fail");
    // 6.Start
    DEMO_CHECK_AND_RETURN_RET_LOG(Start() == AVCS_ERR_OK, false, "Fatal: Start fail");
    // 7.写数据
    WriteSingleTrackSampleAVBuffer(avMuxer_, trackIndex);
    // 9.Stop
    DEMO_CHECK_AND_RETURN_RET_LOG(Stop() == AVCS_ERR_OK, false, "Fatal: Stop fail");
    // 9.Destroy
    Destroy();
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
    return OH_AVMuxer_Start(avMuxer_);
}

int32_t AVMuxerDemo::Stop()
{
    return OH_AVMuxer_Stop(avMuxer_);
}

int32_t AVMuxerDemo::Destroy()
{
    if (trackFormat_ != nullptr) {
        OH_AVFormat_Destroy(trackFormat_);
        trackFormat_ = nullptr;
    }
    if (metaFormat_ != nullptr) {
        OH_AVFormat_Destroy(metaFormat_);
        metaFormat_ = nullptr;
    }
    return true;
}

int32_t AVMuxerDemo::SetRotation(OH_AVMuxer* muxer, int32_t rotation)
{
    return OH_AVMuxer_SetRotation(muxer, rotation);
}

int32_t AVMuxerDemo::SetFormat(const uint8_t *data, size_t size)
{
    // 只支持  1.(内置) OH_MD_KEY_CREATION_TIME
    //         2.(自定义)string int float
    FuzzedDataProvider fdp(data, size);
    
    metaFormat_ = OH_AVFormat_Create();
    
    std::string metaKeyPrefix("com.openharmony.");
    
    std::string fuzzKey1 = fdp.ConsumeRandomLengthString();
    int fuzzValue1 = fdp.ConsumeIntegral<int>();
    std::string fuzzKey2 = fdp.ConsumeRandomLengthString();
    float fuzzValue2 = fdp.ConsumeFloatingPoint<float>();
    std::string fuzzKey3 = fdp.ConsumeRandomLengthString();
    std::string fuzzValue3 = fdp.ConsumeRandomLengthString();
    std::string fuzzValue4 = fdp.ConsumeRandomLengthString();
    
    std::string metaKey1 = metaKeyPrefix + fuzzKey1;
    std::string metaKey2 = metaKeyPrefix + fuzzKey2;
    std::string metaKey3 = metaKeyPrefix + fuzzKey3;
    
    OH_AVFormat_SetIntValue(metaFormat_, metaKey1.c_str(), fuzzValue1);
    OH_AVFormat_SetFloatValue(metaFormat_, metaKey2.c_str(), fuzzValue2);
    OH_AVFormat_SetStringValue(metaFormat_, metaKey3.c_str(), fuzzValue3.c_str());
    OH_AVFormat_SetStringValue(metaFormat_, OH_MD_KEY_CREATION_TIME, fuzzValue4.c_str());
    
    return OH_AVMuxer_SetFormat(avMuxer_, metaFormat_);
}

int32_t AVMuxerDemo::AddTrack(OH_AVMuxer* muxer, int32_t& trackIndex, AudioTrackParam param)
{
    const int configBufferSize =  0x1FFF;
    trackFormat_ = OH_AVFormat_Create();
    if (trackFormat_ == nullptr) {
        std::cout << "AddTrack: format failed!" << std::endl;
        return AV_ERR_INVALID_VAL;
    }
    // set codec config
    int extraSize = 0;
    unsigned char buffer[configBufferSize] = {0};
    read(inputFd_, reinterpret_cast<void *>(extraSize), sizeof(extraSize));
    if (extraSize <= configBufferSize && extraSize > 0) {
        read(inputFd_, buffer, extraSize);
        OH_AVFormat_SetBuffer(trackFormat_, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }

    OH_AVFormat_SetStringValue(trackFormat_, OH_MD_KEY_CODEC_MIME, param.mimeType);
    // audio
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_AUD_SAMPLE_RATE, param.sampleRate);
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_AUD_CHANNEL_COUNT, param.channels);
    OH_AVFormat_SetIntValue(trackFormat_, "audio_samples_per_frame", param.frameSize);
    //video
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_WIDTH, param.width);
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_HEIGHT, param.height);
    OH_AVFormat_SetDoubleValue(trackFormat_, OH_MD_KEY_FRAME_RATE, param.frameRate);
    OH_AVFormat_SetIntValue(trackFormat_, "video_delay", param.videoDelay);
    // hdr vivid
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_COLOR_PRIMARIES, param.colorPrimaries);
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_TRANSFER_CHARACTERISTICS, param.colorTransfer);
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_MATRIX_COEFFICIENTS, param.colorMatrixCoeff);
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_RANGE_FLAG, param.colorRange);
    OH_AVFormat_SetIntValue(trackFormat_, OH_MD_KEY_VIDEO_IS_HDR_VIVID, param.isHdrVivid);
    int32_t ret = OH_AVMuxer_AddTrack(muxer, &trackIndex, trackFormat_);
    return ret;
}

void AVMuxerDemo::WriteSingleTrackSampleAVBuffer(OH_AVMuxer *muxer, int32_t trackIndex)
{
    // 1.【安全检测】 Interface Protection(security check)
    if (muxer == nullptr) {
        std::cout << "WriteSingleTrackSampleAVBuffer: muxer is nullptr!!!" << std::endl;
        return;
    }
    if (trackIndex < 0) {
        std::cout << "WriteSingleTrackSampleAVBuffer: trackIndex error!!!" << std::endl;
        return;
    }
    // 2.【创建info】 create info
    OH_AVCodecBufferAttr info;
    memset_s(&info, sizeof(info), 0, sizeof(info));
	// 3.【数据写入】 write data
    OH_AVBuffer *buffer = nullptr;
    bool ret = false;
    do {
        ret = UpdateWriteBufferInfoAVBuffer(&buffer, &info, trackIndex);
    } while (ret);
    // 4.【释放内存】 free buffer
    if (buffer != nullptr) {
        OH_AVBuffer_Destroy(buffer);
        buffer = nullptr;
    }
}

bool AVMuxerDemo::UpdateWriteBufferInfoAVBuffer(OH_AVBuffer **buffer, OH_AVCodecBufferAttr *info, int32_t trackIndex)
{
    // 1.【安全检测】Interface Protection(security check)
    if (*buffer != nullptr) {
        OH_AVBuffer_Destroy(*buffer);
        *buffer = nullptr;
    }
    if (info == nullptr) {
        std::cout << "UpdateWriteBufferInfoAVBuffer: info is nullptr!!! " << std::endl;
        return false;
    }
    if (trackIndex < 0) {
        std::cout << "UpdateWriteBufferInfoAVBuffer: trackIndex error!!!" << std::endl;
        return false;
    }
    // 2.【获取info数据】get info msg
    // info pts
    int ret = read(inputFd_, (void*)&info->pts, sizeof(info->pts));
    if (ret <= 0) {
        return false;
    }
    // info flags
    ret = read(inputFd_, (void*)&info->flags, sizeof(info->flags));
    info->flags |= AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    info->flags |= AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    if (ret <= 0) {
        return false;
    }
    // info size
    ret = read(inputFd_, (void*)&info->size, sizeof(info->size));
    if (ret <= 0 || info->size < 0) {
        return false;
    }
    info->size %= 10485760; // 10485760: 10MB
    // 3.【初始化buffer】initialization buffer
    *buffer = OH_AVBuffer_Create(info->size);
    if (*buffer == nullptr) {
        std::cout << "UpdateWriteBufferInfoAVBuffer: buffer creation failed!!! " << std::endl;
        return false;
    }
    ret = read(inputFd_, (void*)OH_AVBuffer_GetAddr(*buffer), info->size);
    if (ret <= 0) {
        std::cout << "UpdateWriteBufferInfoAVBuffer: buffer acquisition failed!!! " << std::endl;
        return false;
    }
    // 4.【写入数据】write data
    OH_AVErrCode res = OH_AVBuffer_SetBufferAttr(*buffer, info);
    DEMO_CHECK_AND_RETURN_RET_LOG(res == AV_ERR_OK, false, "Fatal: OH_AVBuffer_SetBufferAttr fail!!!");
    res = OH_AVMuxer_WriteSampleBuffer(avMuxer_, trackIndex, *buffer);
    DEMO_CHECK_AND_RETURN_RET_LOG(res == AV_ERR_OK, false, "Fatal: OH_AVMuxer_WriteSampleBuffer fail!!!");
    OH_AVMemory *memBuf = OH_AVMemory_Create(info->size);
    DEMO_CHECK_AND_RETURN_RET_LOG(memBuf != nullptr, false, "create AVMemory fail");
    errno_t secRet = memcpy_s(OH_AVMemory_GetAddr(memBuf), info->size, OH_AVBuffer_GetAddr(*buffer), info->size);
    DEMO_CHECK_AND_RETURN_RET_LOG(secRet == EOK, false, "memcpy_s fail");
    res = OH_AVMuxer_WriteSample(avMuxer_, 0, memBuf, *info);
    DEMO_CHECK_AND_RETURN_RET_LOG(res == AV_ERR_OK, false, "Fatal: OH_AVMuxer_WriteSample fail!!!");
    OH_AVMemory_Destroy(memBuf);
    return true;
}