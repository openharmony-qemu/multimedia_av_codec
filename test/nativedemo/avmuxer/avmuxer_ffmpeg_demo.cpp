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
#include "plugin/plugin_manager.h"

namespace OHOS {
namespace MediaAVCodec {
int AVMuxerFFmpegDemo::DoAddTrack(int32_t &trackIndex, std::shared_ptr<Meta> trackDesc)
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
    std::string outFileName = GetOutputFileName("ffmpeg_mux");
    outFd_ = open(outFileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (outFd_ < 0) {
        std::cout<<"create muxer output file failed! fd:"<<outFd_<<std::endl;
        return;
    }
    std::cout<<"==== open success! =====\noutputFileName: "<<outFileName<<"\n============"<<std::endl;

    ffmpegMuxer_ = CreatePlugin(Plugins::OutputFormat::MPEG_4);
    if (ffmpegMuxer_ == nullptr) {
        std::cout<<"ffmpegMuxer create failed!"<<std::endl;
        return;
    }

    ffmpegMuxer_->SetDataSink(std::make_shared<DataSinkFd>(outFd_));

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

int AVMuxerFFmpegDemo::DoWriteSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    if (ffmpegMuxer_ != nullptr &&
        ffmpegMuxer_->WriteSample(trackIndex, sample) == Status::OK) {
        return 0;
    }
    return -1;
}

std::shared_ptr<Plugins::MuxerPlugin> AVMuxerFFmpegDemo::CreatePlugin(Plugins::OutputFormat format)
{
    static const std::unordered_map<Plugins::OutputFormat, std::string> table = {
        {Plugins::OutputFormat::DEFAULT, Plugins::MimeType::MEDIA_MP4},
        {Plugins::OutputFormat::MPEG_4, Plugins::MimeType::MEDIA_MP4},
        {Plugins::OutputFormat::M4A, Plugins::MimeType::MEDIA_M4A},
        {Plugins::OutputFormat::MP3, Plugins::MimeType::MEDIA_MP3},
    };

    auto names = Plugins::PluginManager::Instance().ListPlugins(Plugins::PluginType::MUXER);
    std::string pluginName = "";
    uint32_t maxProb = 0;
    for (auto& name : names) {
        auto info = Plugins::PluginManager::Instance().GetPluginInfo(Plugins::PluginType::MUXER, name);
        if (info == nullptr) {
            continue;
        }
        for (auto& cap : info->outCaps) {
            if (cap.mime == table.at(format) && info->rank > maxProb) {
                maxProb = info->rank;
                pluginName = name;
                break;
            }
        }
    }
    std::cout<<"The maxProb is "<<maxProb<<" and pluginName is "<<pluginName<<std::endl;
    if (!pluginName.empty()) {
        auto plugin = Plugins::PluginManager::Instance().CreatePlugin(pluginName, Plugins::PluginType::MUXER);
        return std::reinterpret_pointer_cast<Plugins::MuxerPlugin>(plugin);
    }
    return nullptr;
}
}  // namespace MediaAVCodec
}  // namespace OHOS