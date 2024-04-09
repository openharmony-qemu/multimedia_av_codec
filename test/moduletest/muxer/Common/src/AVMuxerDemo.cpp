/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "AVMuxerDemo.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace std;
using namespace Plugins;

int32_t AVMuxerDemo::GetFdByMode(OH_AVOutputFormat format)
{
    if (format == AV_OUTPUT_FORMAT_MPEG_4) {
        filename = "output.mp4";
    } else if (format == AV_OUTPUT_FORMAT_M4A) {
        filename = "output.m4a";
    } else if (format == AV_OUTPUT_FORMAT_AMR) {
        filename = "output.amr";
    } else if (format == AV_OUTPUT_FORMAT_MP3) {
        filename = "output.mp3";
    } else {
        filename = "output.bin";
    }
    int32_t fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}

int32_t AVMuxerDemo::GetErrorFd()
{
    filename = "output.bin";
    int32_t fd = open(filename.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}

int32_t AVMuxerDemo::GetFdByName(OH_AVOutputFormat format, string fileName)
{
    if (format == AV_OUTPUT_FORMAT_MPEG_4) {
        filename = fileName + ".mp4";
    } else if (format == AV_OUTPUT_FORMAT_M4A) {
        filename = fileName + ".m4a";
    } else if (format == AV_OUTPUT_FORMAT_AMR) {
        filename = fileName + ".amr";
    } else if (format == AV_OUTPUT_FORMAT_MP3) {
        filename = fileName + ".mp3";
    } else {
        filename = fileName + ".bin";
    }
    int32_t fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}

int32_t AVMuxerDemo::InnerGetFdByMode(OutputFormat format)
{
    if (format == OutputFormat::MPEG_4) {
        filename = "output.mp4";
    } else if (format == OutputFormat::M4A) {
        filename = "output.m4a";
    } else {
        filename = "output.bin";
    }
    int32_t fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}

int32_t AVMuxerDemo::InnerGetFdByName(OutputFormat format, string fileName)
{
    if (format == OutputFormat::MPEG_4) {
        filename = fileName + ".mp4";
    } else if (format == OutputFormat::M4A) {
        filename = fileName + ".m4a";
    } else {
        filename = fileName + ".bin";
    }
    int32_t fd = open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}
OH_AVMuxer* AVMuxerDemo::NativeCreate(int32_t fd, OH_AVOutputFormat format)
{
    return OH_AVMuxer_Create(fd, format);
}

OH_AVErrCode AVMuxerDemo::NativeSetRotation(OH_AVMuxer* muxer, int32_t rotation)
{
    return OH_AVMuxer_SetRotation(muxer, rotation);
}

OH_AVErrCode AVMuxerDemo::NativeAddTrack(OH_AVMuxer* muxer, int32_t* trackIndex, OH_AVFormat* trackFormat)
{
    return OH_AVMuxer_AddTrack(muxer, trackIndex, trackFormat);
}

OH_AVErrCode AVMuxerDemo::NativeStart(OH_AVMuxer* muxer)
{
    return OH_AVMuxer_Start(muxer);
}

OH_AVErrCode AVMuxerDemo::NativeWriteSampleBuffer(OH_AVMuxer* muxer, uint32_t trackIndex,
    OH_AVMemory* sampleBuffer, OH_AVCodecBufferAttr info)
{
    return OH_AVMuxer_WriteSample(muxer, trackIndex, sampleBuffer, info);
}

OH_AVErrCode AVMuxerDemo::NativeWriteSampleBuffer(OH_AVMuxer* muxer, uint32_t trackIndex,
    OH_AVBuffer* sampleBuffer)
{
    return OH_AVMuxer_WriteSampleBuffer(muxer, trackIndex, sampleBuffer);
}

OH_AVErrCode AVMuxerDemo::NativeStop(OH_AVMuxer* muxer)
{
    return OH_AVMuxer_Stop(muxer);
}

OH_AVErrCode AVMuxerDemo::NativeDestroy(OH_AVMuxer* muxer)
{
    return OH_AVMuxer_Destroy(muxer);
}

int32_t AVMuxerDemo::InnerCreate(int32_t fd, OutputFormat format)
{
    cout << "InnerCreate" << endl;
    avmuxer_ = AVMuxerFactory::CreateAVMuxer(fd, format);
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    return AV_ERR_OK;
}

int32_t AVMuxerDemo::InnerSetRotation(int32_t rotation)
{
    cout << "InnerSetRotation" << endl;
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::VIDEO_ROTATION>(static_cast<Plugins::VideoRotation>(rotation));
    return avmuxer_->SetParameter(param);
}

int32_t AVMuxerDemo::InnerAddTrack(int32_t& trackIndex, std::shared_ptr<Meta> trackDesc)
{
    cout << "InnerAddTrack" << endl;
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    return avmuxer_->AddTrack(trackIndex, trackDesc);
}

int32_t AVMuxerDemo::InnerStart()
{
    cout << "InnerStart" << endl;
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    return avmuxer_->Start();
}

int32_t AVMuxerDemo::InnerWriteSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    cout << "InnerWriteSample" << endl;
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    return avmuxer_->WriteSample(trackIndex, sample);
}

int32_t AVMuxerDemo::InnerStop()
{
    cout << "InnerStop" << endl;
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    return avmuxer_->Stop();
}

int32_t AVMuxerDemo::InnerDestroy()
{
    cout << "InnerDestroy" << endl;
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    avmuxer_->~AVMuxer();
    avmuxer_ = nullptr;
    return AV_ERR_OK;
}
