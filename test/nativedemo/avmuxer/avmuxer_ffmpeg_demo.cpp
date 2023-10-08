/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avmuxer_ffmpeg_demo.h"
#include <dlfcn.h>
#include <iostream>
#include <fcntl.h>
#include <fstream>
#include "data_sink_fd.h"

namespace {
    const char *FFMPEG_REGISTER_FUNC_NAME = "register_FFmpegMuxer";
    const char *FFMPEG_UNREGISTER_FUNC_NAME =  "unregister_FFmpegMuxer";
    const char *FFMPEG_LIB_PATH =  "/libav_codec_plugin_FFmpegMuxer.z.so";
}

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
Status AVMuxerFFmpegDemo::FfmpegRegister::AddPlugin(const PluginDefBase& def)
{
    auto& tempDef = (MuxerPluginDef&)def;
    std::cout<<"find plugin apiVersion:"<<tempDef.apiVersion;
    std::cout<<" |pluginType:"<<static_cast<int32_t>(tempDef.pluginType);
    std::cout<<" |name:"<<tempDef.name;
    std::cout<<" |description:"<<tempDef.description;
    std::cout<<" |rank:"<<tempDef.rank;
    std::cout<<" |sniffer:"<<tempDef.sniffer;
    std::cout<<" |creator:"<<tempDef.creator;
    std::cout<<std::endl;
    plugins.push_back(tempDef);
    return Status::NO_ERROR;
}

AVMuxerFFmpegDemo::AVMuxerFFmpegDemo() : register_(std::make_shared<FfmpegRegister>())
{
}

int AVMuxerFFmpegDemo::DoAddTrack(int32_t &trackIndex, MediaDescription &trackDesc)
{
    int32_t tempTrackId = 0;
    ffmpegMuxer_->AddTrack(tempTrackId, trackDesc);
    if (tempTrackId < 0) {
        std::cout<<"AVMuxerFFmpegDemo::DoAddTrack failed! trackId:"<<tempTrackId<<std::endl;
        return -1;
    }
    trackIndex = tempTrackId;
    return 0;
}

void AVMuxerFFmpegDemo::DoRunMuxer()
{
    long long testTimeStart = GetTimestamp();
    GetFfmpegRegister();
    if (register_->plugins.size() == 0) {
        std::cout<<"regist muxers failed!"<<std::endl;
        return;
    }

    int32_t maxProb = 0;
    MuxerPluginDef pluginDef {};
    for (auto& plugin : register_->plugins) {
        if (plugin.pluginType == PluginType::MUXER) {
            auto prob = plugin.sniffer(plugin.name, outputFormat_);
            if (prob > maxProb) {
                maxProb = prob;
                pluginDef = plugin;
            }
        }
    }

    if (pluginDef.creator == nullptr) {
        std::cout<<"no plugins matching output format - "<< outputFormat_ <<std::endl;
        return;
    }

    std::string outFileName = GetOutputFileName("ffmpeg_mux_");
    outFd_ = open(outFileName.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (outFd_ < 0) {
        std::cout<<"create muxer output file failed! fd:"<<outFd_<<std::endl;
        return;
    }
    std::cout<<"==== open success! =====\noutputFileName: "<<outFileName<<"\n============"<<std::endl;

    ffmpegMuxer_ = pluginDef.creator(pluginDef.name);
    if (ffmpegMuxer_ == nullptr) {
        std::cout<<"ffmpegMuxer create failed!"<<std::endl;
        return;
    }
    ffmpegMuxer_->SetDataSink(std::make_shared<DataSinkFd>(outFd_));
    ffmpegMuxer_->SetRotation(0);

    AddAudioTrack(audioParams_);
    AddVideoTrack(videoParams_);
    AddCoverTrack(coverParams_);

    ffmpegMuxer_->Start();
    WriteCoverSample();
    WriteTrackSample();
    ffmpegMuxer_->Stop();
    long long testTimeEnd = GetTimestamp();
    std::cout << "muxer used time: " << testTimeEnd - testTimeStart << "us" << std::endl;
}

void AVMuxerFFmpegDemo::DoRunMultiThreadCase()
{
    std::cout<<"ffmpeg plugin demo is not support multi-thread write!"<<std::endl;
    return;
}

int AVMuxerFFmpegDemo::DoWriteSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    if (ffmpegMuxer_ != nullptr &&
        ffmpegMuxer_->WriteSample(trackIndex, sample->GetBase(), info, flag) == Status::NO_ERROR) {
        return 0;
    }
    return -1;
}

int AVMuxerFFmpegDemo::GetFfmpegRegister()
{
    std::string libPath = AV_CODEC_PLUGIN_PATH;
    libPath += FFMPEG_LIB_PATH;
    dlHandle_ = ::dlopen(libPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (dlHandle_ == nullptr) {
        std::cout<<"AVMuxerFFmpegDemo::GetFfmpegRegister dlHandle_ is nullptr!"<<std::endl;
        return -1;
    }

    registerFunc_ = (RegisterFunc)(::dlsym(dlHandle_, FFMPEG_REGISTER_FUNC_NAME));
    unregisterFunc_ = (UnregisterFunc)(::dlsym(dlHandle_, FFMPEG_UNREGISTER_FUNC_NAME));
    if (registerFunc_ == nullptr || unregisterFunc_ == nullptr) {
        std::cout<<"get dl function failed! registerFunc_:"<<(void*)registerFunc_;
        return -1;
    }

    if (registerFunc_(register_) != Status::NO_ERROR) {
        std::cout<<"ffmpeg register failed!"<<std::endl;
        return -1;
    }
    return 0;
}
}  // Plugin
}  // namespace MediaAVCodec
}  // namespace OHOS