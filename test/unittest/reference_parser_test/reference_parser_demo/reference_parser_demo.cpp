/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "reference_parser_demo.h"
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include "media_description.h"

using namespace std;

using json = nlohmann::json;

namespace {
constexpr const char *SOURCE_DIR = "/data/test/media/";
constexpr uint32_t MAX_SCENE_NUM = static_cast<uint32_t>(OHOS::MediaAVCodec::MP4Scene::SCENE_MAX);
constexpr const char *VIDEO_FILE_NAME[MAX_SCENE_NUM] = {
    "ipb_0", "ipb_1", "ippp_0", "ippp_1", "ippp_scala_0", "ippp_scala_1", "sdtp", "sdtp_ext"};
constexpr int32_t MAX_BUFFER_SIZE = 8294400;
constexpr int32_t MILL_TO_MICRO = 1000;
constexpr uint32_t MIN_REMAIN_LAYERS = 2;
} // namespace

void from_json(const nlohmann::json &j, JsonGopInfo &gop)
{
    j.at("gopId").get_to(gop.gopId);
    j.at("gopSize").get_to(gop.gopSize);
    j.at("startFrameId").get_to(gop.startFrameId);
}

void from_json(const nlohmann::json &j, JsonFrameLayerInfo &frame)
{
    j.at("frameId").get_to(frame.frameId);
    j.at("dts").get_to(frame.dts);
    j.at("layer").get_to(frame.layer);
    j.at("discardable").get_to(frame.discardable);
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;

ReferenceParserDemo::~ReferenceParserDemo()
{
    close(fd_);
    fd_ = 0;
    source_ = nullptr;
    demuxer_ = nullptr;
}

int32_t ReferenceParserDemo::InitScene(MP4Scene scene)
{
    string path = string(SOURCE_DIR) + string(VIDEO_FILE_NAME[static_cast<uint32_t>(scene)]);
    std::ifstream(path + "_gop.json") >> gopJson_;
    std::ifstream(path + "_frame.json") >> frameLayerJson_;
    string filePath = path + ".mp4";
    fd_ = open(filePath.c_str(), O_RDONLY);
    struct stat fileStatus {};
    if (stat(filePath.c_str(), &fileStatus) != 0) {
        return -1;
    }
    int64_t size = static_cast<int64_t>(fileStatus.st_size);
    cout << filePath << ", fd is " << fd_ << ", size is " << size << endl;
    int32_t ret = InitDemuxer(size);
    if (ret < 0) {
        cout << "InitDemuxer failed!" << endl;
        return ret;
    }
    LoadJson();
    shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    buffer_ = AVBuffer::CreateAVBuffer(allocator, MAX_BUFFER_SIZE);
    return 0;
}

int32_t ReferenceParserDemo::InitDemuxer(int64_t size)
{
    source_ = AVSourceFactory::CreateWithFD(fd_, 0, size);
    if (source_ == nullptr) {
        cout << "CreatWithFD failed!" << endl;
        return -1;
    }
    Format format;
    source_->GetSourceFormat(format);
    int32_t trackCount = 0;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, trackCount);
    for (auto i = 0; i < trackCount; i++) {
        source_->GetTrackFormat(format, i);
        int32_t trackType = 0;
        format.GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, trackType);
        if (trackType == 1) {
            videoTrackId_ = i;
        }
    }
    demuxer_ = AVDemuxerFactory::CreateWithSource(source_);
    if (demuxer_ == nullptr) {
        cout << "CreateWithSource failed!" << endl;
        return -1;
    }
    return 0;
}

void ReferenceParserDemo::LoadJson()
{
    gopVec_ = gopJson_.get<vector<JsonGopInfo>>();
    frameVec_ = frameLayerJson_.get<vector<JsonFrameLayerInfo>>();
    for (auto gop : gopVec_) {
        cout << "GopID " << gop.gopId << ", GopSize " << gop.gopSize << ", startFrameId " << gop.startFrameId << endl;
        int32_t frameId = gop.startFrameId;
        for (auto i = 0; i < gop.gopSize; i++) {
            JsonFrameLayerInfo frame = frameVec_[frameId + i];
            frameMap_.emplace(frame.dts, frame);
            cout << "FrameId " << frame.frameId << ", Layer " << frame.layer << endl;
        }
    }
}

void ReferenceParserDemo::SetDecIntervalMs(int64_t decIntervalMs)
{
    decIntervalUs_ = decIntervalMs * MILL_TO_MICRO;
}

bool ReferenceParserDemo::CheckFrameLayerResult(const FrameLayerInfo &info, int64_t dts)
{
    JsonFrameLayerInfo frame = frameMap_[dts];
    if (!frame.discardable && info.isDiscardable) {
        cout << "FrameId " << frame.frameId << ", expect layer " << frame.layer << ", get layer " << info.layer
             << ", dts " << dts << ", match failed!" << endl;
        return false;
    }
    cout << "FrameId " << frame.frameId << ", dts" << dts << ", match success!" << endl;
    return true;
}

bool ReferenceParserDemo::CheckGopLayerResult(GopLayerInfo &GopLayerInfo, uint32_t gopid)
{
    return true;
}

int32_t ReferenceParserDemo::GetMaxDiscardLayer(const GopLayerInfo &GopLayerInfo)
{
    return GopLayerInfo.layerCount - 1 - MIN_REMAIN_LAYERS;
}

bool ReferenceParserDemo::DoAccurateSeek(int64_t seekTimeMs)
{
    demuxer_->SelectTrackByID(videoTrackId_);
    demuxer_->ReadSampleBuffer(videoTrackId_, buffer_);
    demuxer_->SeekToTime(seekTimeMs, SeekMode::SEEK_PREVIOUS_SYNC);
    demuxer_->StartReferenceParser(seekTimeMs);
    FrameLayerInfo frameInfo;
    int64_t pts = -1L;
    while (pts < seekTimeMs * MILL_TO_MICRO) {
        demuxer_->ReadSampleBuffer(videoTrackId_, buffer_);
        if (buffer_->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
            break;
        }
        demuxer_->GetFrameLayerInfo(buffer_, frameInfo);
        pts = buffer_->pts_;
        if (!frameInfo.isDiscardable) {
            usleep(decIntervalUs_);
        }
        if (!CheckFrameLayerResult(frameInfo, buffer_->dts_)) {
            return false;
        }
    }
    return true;
}

int32_t ReferenceParserDemo::IsFrameDiscard(FrameLayerInfo &frameInfo, bool &isDiscard)
{
    if (frameInfo.layer == -1) {
        isDiscard = frameInfo.isDiscardable;
        return 0;
    }

    if (checkedGopId_ != frameInfo.gopId) {
        GopLayerInfo gopLayerInfo;
        demuxer_->GetGopLayerInfo(frameInfo.gopId, gopLayerInfo);
        if (!CheckGopLayerResult(gopLayerInfo, frameInfo.gopId)) {
            return -1;
        }
        checkedGopId_ = frameInfo.gopId;
        maxDiscardLayer_ = GetMaxDiscardLayer(gopLayerInfo);
    }
    isDiscard = frameInfo.layer <= maxDiscardLayer_;
    return 0;
}

bool ReferenceParserDemo::DoVariableSpeedPlay(int64_t playTimeMs)
{
    demuxer_->SelectTrackByID(videoTrackId_);
    demuxer_->SeekToTime(playTimeMs, SeekMode::SEEK_PREVIOUS_SYNC);
    buffer_->pts_ = -1L;
    while (buffer_->pts_ < playTimeMs * MILL_TO_MICRO) {
        demuxer_->ReadSampleBuffer(videoTrackId_, buffer_);
    }

    demuxer_->StartReferenceParser(playTimeMs);
    FrameLayerInfo frameInfo;
    do {
        demuxer_->ReadSampleBuffer(videoTrackId_, buffer_);
        demuxer_->GetFrameLayerInfo(buffer_, frameInfo);
        if (!CheckFrameLayerResult(frameInfo, buffer_->dts_)) {
            return false;
        }

        if (IsFrameDiscard(frameInfo, isDiscard_) != 0) {
            return false;
        }
        
        if (!isDiscard_) {
            usleep(decIntervalUs_);
        }
    } while (!(buffer_->flag_ & AVCODEC_BUFFER_FLAG_EOS));
    return true;
}

} // namespace MediaAVCodec
} // namespace OHOS