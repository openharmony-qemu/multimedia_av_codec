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
#include "data_sink_fd.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;
using namespace Plugin;
using namespace Ffmpeg;

#ifdef __cplusplus
extern "C" {
#endif
    extern Status register_FFmpegMuxer(const std::shared_ptr<PackageRegister>& pkgReg);
#ifdef __cplusplus
}
#endif


int32_t AVMuxerDemo::getFdByMode(OH_AVOutputFormat format)
{
    if (format == AV_OUTPUT_FORMAT_MPEG_4) {
        filename = "output.mp4";
    } else if (format == AV_OUTPUT_FORMAT_M4A) {
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

int32_t AVMuxerDemo::getErrorFd()
{
    filename = "output.bin";
    int32_t fd = open(filename.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}

int32_t AVMuxerDemo::getFdByName(OH_AVOutputFormat format, string fileName)
{
    if (format == AV_OUTPUT_FORMAT_MPEG_4) {
        filename = fileName + ".mp4";
    } else if (format == AV_OUTPUT_FORMAT_M4A) {
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

int32_t AVMuxerDemo::FFmpeggetFdByMode(OutputFormat format)
{
    if (format == OUTPUT_FORMAT_MPEG_4) {
        filename = "output.mp4";
    } else if (format == OUTPUT_FORMAT_M4A) {
        filename = "output.m4a";
    } else {
        filename = "output.bin";
    }
    int32_t fd = open(filename.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}

int32_t AVMuxerDemo::FFmpeggetFdByName(OutputFormat format, string fileName)
{
    if (format == OUTPUT_FORMAT_MPEG_4) {
        filename = fileName + ".mp4";
    } else if (format == OUTPUT_FORMAT_M4A) {
        filename = fileName + ".m4a";
    } else {
        filename = fileName + ".bin";
    }
    int32_t fd = open(filename.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << filename << std::endl;
        return -1;
    }
    return fd;
}
int32_t AVMuxerDemo::InnergetFdByMode(OutputFormat format)
{
    if (format == OUTPUT_FORMAT_MPEG_4) {
        filename = "output.mp4";
    } else if (format == OUTPUT_FORMAT_M4A) {
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

int32_t AVMuxerDemo::InnergetFdByName(OutputFormat format, string fileName)
{
    if (format == OUTPUT_FORMAT_MPEG_4) {
        filename = fileName + ".mp4";
    } else if (format == OUTPUT_FORMAT_M4A) {
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

OH_AVErrCode AVMuxerDemo::NativeStop(OH_AVMuxer* muxer)
{
    return OH_AVMuxer_Stop(muxer);
}

OH_AVErrCode AVMuxerDemo::NativeDestroy(OH_AVMuxer* muxer)
{
    return OH_AVMuxer_Destroy(muxer);
}


Status AVMuxerDemo::FfmpegRegister::AddPlugin(const PluginDefBase& def)
{
    auto& tempDef = (MuxerPluginDef&)def;
    pluginDef = tempDef;
    tempDef.creator = tempDef.creator;
    tempDef.sniffer = tempDef.sniffer;
    std::cout << "apiVersion:" << pluginDef.apiVersion;
    std::cout << " |pluginType:" << (int32_t)pluginDef.pluginType;
    std::cout << " |name:" << pluginDef.name;
    std::cout << " |description:" << pluginDef.description;
    std::cout << " |rank:" << pluginDef.rank;
    std::cout << " |creator:" << (void*)pluginDef.creator;
    std::cout << std::endl;
    return Status::OK;
}

void AVMuxerDemo::FFmpegCreate(int32_t fd)
{
    cout << "FFmpegCreate" << endl;
    register_ = std::make_shared<FfmpegRegister>();
    register_FFmpegMuxer(register_);
    ffmpegMuxer_ = register_->pluginDef.creator(register_->pluginDef.name);
    if (ffmpegMuxer_ == nullptr) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
        return;
    }
    cout << "ffmpegMuxer_ is: " << ffmpegMuxer_ << endl;
    Status ret = ffmpegMuxer_->SetDataSink(std::make_shared<DataSinkFd>(fd));
    if (ret != Status::NO_ERROR) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
    }
}

Status AVMuxerDemo::FFmpegSetRotation(int32_t rotation)
{
    cout << "FFmpegSetRotation" << endl;
    if (ffmpegMuxer_ == nullptr) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
        return Status::ERROR_NULL_POINTER;
    }
    return ffmpegMuxer_->SetRotation(rotation);
}

Status AVMuxerDemo::FFmpegAddTrack(int32_t& trackIndex, const MediaDescription& trackDesc)
{
    cout << "FFmpegAddTrack" << endl;
    if (ffmpegMuxer_ == nullptr) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
        return Status::ERROR_NULL_POINTER;
    }
    return ffmpegMuxer_->AddTrack(trackIndex, trackDesc);
}

Status AVMuxerDemo::FFmpegStart()
{
    cout << "FFmpegStart" << endl;
    if (ffmpegMuxer_ == nullptr) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
        return Status::ERROR_NULL_POINTER;
    }
    return ffmpegMuxer_->Start();
}

Status AVMuxerDemo::FFmpegWriteSample(uint32_t trackIndex, const uint8_t *sample,
    AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "FFmpegWriteSample" << endl;
    if (ffmpegMuxer_ == nullptr) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
        return Status::ERROR_NULL_POINTER;
    }
    return ffmpegMuxer_->WriteSample(trackIndex, sample, info, flag);
}

Status AVMuxerDemo::FFmpegStop()
{
    cout << "FFmpegStop" << endl;
    if (ffmpegMuxer_ == nullptr) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
        return Status::ERROR_NULL_POINTER;
    }
    return ffmpegMuxer_->Stop();
}


Status AVMuxerDemo::FFmpegDestroy()
{
    cout << "FFmpegDestroy" << endl;
    if (ffmpegMuxer_ == nullptr) {
        std::cout << "ffmpegMuxer create failed!" << std::endl;
        return Status::ERROR_NULL_POINTER;
    }
    ffmpegMuxer_ = nullptr;
    return Status::OK;
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
    return avmuxer_->SetRotation(rotation);
}


int32_t AVMuxerDemo::InnerAddTrack(int32_t& trackIndex, const MediaDescription& trackDesc)
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


int32_t AVMuxerDemo::InnerWriteSample(uint32_t trackIndex,
    std::shared_ptr<AVSharedMemory> sample, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "InnerWriteSample" << endl;
    if (avmuxer_ == nullptr) {
        std::cout << "InnerMuxer create failed!" << std::endl;
        return AVCS_ERR_NO_MEMORY;
    }
    return avmuxer_->WriteSample(trackIndex, sample, info, flag);
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