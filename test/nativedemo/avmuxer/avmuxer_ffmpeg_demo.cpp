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
    std::string outFileName = "ffmpeg_mux_" + audioType_ + "_" + videoType_ + "_" + coverType_ + "." + format_;
    outFd_ = open(outFileName.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (outFd_ < 0) {
        std::cout<<"create muxer output file failed! fd:"<<outFd_<<std::endl;
        return;
    }
    std::cout<<"==== open success! =====\noutputFileName: "<<outFileName<<"\n============"<<std::endl;

    ffmpegMuxer_ = CreatePlugin(Plugin::OutputFormat::MPEG_4);
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

std::shared_ptr<Plugin::MuxerPlugin> AVMuxerFFmpegDemo::CreatePlugin(Plugin::OutputFormat format)
{
    static const std::unordered_map<Plugin::OutputFormat, std::string> table = {
        {Plugin::OutputFormat::DEFAULT, Plugin::MimeType::MEDIA_MP4},
        {Plugin::OutputFormat::MPEG_4, Plugin::MimeType::MEDIA_MP4},
        {Plugin::OutputFormat::M4A, Plugin::MimeType::MEDIA_M4A},
    };

    auto names = Plugin::PluginManager::Instance().ListPlugins(Plugin::PluginType::MUXER);
    std::string pluginName = "";
    uint32_t maxProb = 0;
    for (auto& name : names) {
        auto info = Plugin::PluginManager::Instance().GetPluginInfo(Plugin::PluginType::MUXER, name);
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
        auto plugin = Plugin::PluginManager::Instance().CreatePlugin(pluginName, Plugin::PluginType::MUXER);
        return std::reinterpret_pointer_cast<Plugin::MuxerPlugin>(plugin);
    }
    return nullptr;
}
}  // namespace MediaAVCodec
}  // namespace OHOS