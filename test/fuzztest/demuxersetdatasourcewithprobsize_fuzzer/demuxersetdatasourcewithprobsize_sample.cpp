/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "demuxersetdatasourcewithprobsize_sample.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

const int BUFFER_PADDING_SIZE = 1024;
const std::string prefix = "avdemux";

bool DemuxerPluginTest::CreateDataSource(const std::string& filePath)
{
    mediaSource_ = std::make_shared<MediaSource>(filePath);
    realSource_ = std::make_shared<Source>();
    realSource_->SetSource(mediaSource_);

    realStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    realStreamDemuxer_->SetSourceType(Plugins::SourceType::SOURCE_TYPE_URI);
    realStreamDemuxer_->SetSource(realSource_);
    realStreamDemuxer_->Init(filePath);

    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    dataSourceImpl_ = std::make_shared<DataSourceImpl>(realStreamDemuxer_, streamId_);
    realSource_->NotifyInitSuccess();

    return true;
}

bool DemuxerPluginTest::CreateDemuxerPluginByName(
    const std::string& typeName, const std::string& filePath, int probSize)
{
    if (typeName.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }

    if (!CreateDataSource(filePath)) {
        return false;
    }
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(typeName);
    if (pluginBase_ == nullptr) {
        return false;
    }

    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, probSize) != Status::OK) {
        return false;
    }

    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);

    return true;
}

bool DemuxerPluginTest::PluginSelectTracks()
{
    MediaInfo mediaInfo;
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (demuxerPlugin->GetMediaInfo(mediaInfo) != Status::OK) {
        return false;
    }

    for (size_t i = 0; i < mediaInfo.tracks.size(); i++) {
        demuxerPlugin->SelectTrack(static_cast<uint32_t>(i));
        selectedTrackIds_.push_back(static_cast<uint32_t>(i));
    }

    return true;
}

bool DemuxerPluginTest::PluginReadSample(uint32_t idx, uint32_t& flag)
{
    int bufSize = 0;
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    demuxerPlugin->GetNextSampleSize(idx, bufSize);
    if (static_cast<size_t>(bufSize) > buffer_.size()) {
        buffer_.resize(bufSize + BUFFER_PADDING_SIZE);
    }

    auto avBuf = AVBuffer::CreateAVBuffer(buffer_.data(), bufSize, bufSize);
    if (avBuf == nullptr) {
        return false;
    }
    
    demuxerPlugin->ReadSample(idx, avBuf);
    flag = avBuf->flag_;

    return true;
}

bool DemuxerPluginTest::PluginReadAllSample()
{
    bool end = false;
    while (!end) {
        for (auto idx : selectedTrackIds_) {
            uint32_t flag = 0;
            if (!PluginReadSample(idx, flag)) {
                return false;
            }
            if (flag & static_cast<uint32_t>(AVBufferFlag::EOS)) {
                end = true;
                break;
            }
        }
    }

    return true;
}

bool DemuxerPluginTest::Run(const std::string& typeName, const std::string& filePath, int probSize)
{
    if (!CreateDemuxerPluginByName(typeName, filePath, probSize)) {
        return false;
    }
    if (!PluginSelectTracks()) {
        return false;
    }
    int64_t realSeekTime = 0;
    if (!PluginSeekToKeyFrame(-1, seekTime_, SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeOutMs_)) {
        return false;
    }
    if (!PluginReadAllSample()) {
        return false;
    }
    if (!PluginReplay()) {
        return false;
    }
    return true;
}

bool DemuxerPluginTest::PluginReplay()
{
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    auto ret = demuxerPlugin->SeekToStart();
    if (ret != Status::OK) {
        return false;
    }
    if (!PluginReadAllSample()) {
        return false;
    }
    return true;
}

bool DemuxerPluginTest::PluginSeekToKeyFrame(int32_t trackId, int64_t seekTime, SeekMode mode,
        int64_t& realSeekTime, uint32_t timeoutMs)
{
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    auto ret = demuxerPlugin->SeekToKeyFrame(trackId, seekTime, mode, realSeekTime, timeoutMs);
    if (ret != Status::OK) {
        return false;
    }
    return true;
}