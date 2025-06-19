
/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "ffmpeg_demuxer_plugin.h"

// namespace OHOS {
// namespace Media {

// class MockDemuxerPlugin : OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin {
// public:
    
//     explicit MockDemuxerPlugin(std::string name):FFmpegDemuxerPlugin(std::move(name)) {};
//     MOCK_METHOD(bool, EnsurePacketAllocated, (AVPacket*&), ());

    
//     using FFmpegDemuxerPlugin::ioContext_;
//     using FFmpegDemuxerPlugin::SetDataSource;
//     using FFmpegDemuxerPlugin::GetMediaInfo;
//     using FFmpegDemuxerPlugin::ReadSample;
//     using FFmpegDemuxerPlugin::SelectTrack;
//     using FFmpegDemuxerPlugin::Reset;
//     using FFmpegDemuxerPlugin::SeekTo;

//     using FFmpegDemuxerPlugin::ResetParam;
//     using FFmpegDemuxerPlugin::ReleaseFFmpegReadLoop;
//     using FFmpegDemuxerPlugin::GetUserMeta;

// };
// } // namespace Ffmpeg
// } // namespace OHOS


namespace OHOS {
namespace Media {
class MockDemuxerPlugin {
    public : 
    virtual ~MockDemuxerPlugin() = default;
    virtual bool EnsurePacketAllocated(AVPacket*& pkt) = 0;
};

template <typename... T>
class MockDemuxerPluginAdapter : public T... {
public:
    using T::T...;
    ~MockDemuxerPluginAdapter() override = default;
    MOCK_METHOD(bool, EnsurePacketAllocated, (AVPacket*&), (override));
};
} // namespace Media
} // namespace OHOS



// namespace OHOS {
// namespace Media {
// class MockDemuxerPlugin {
//     public : 
//     virtual ~MockDemuxerPlugin() = default;
//     virtual bool EnsurePacketAllocated(AVPacket*& pkt) = 0;
// };

// class MockDemuxerPluginAdapter : public MockDemuxerPlugin, public OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin {
// public:
//     using MockDemuxerPlugin::MockDemuxerPlugin;
//     using OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin::FFmpegDemuxerPlugin;
//     // MockDemuxerPluginAdapter(std::string name) : OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin(name) {};
//     ~MockDemuxerPluginAdapter() override = default;
//     MOCK_METHOD(bool, EnsurePacketAllocated, (AVPacket*&), (override));
// };
// } // namespace Media
// } // namespace OHOS