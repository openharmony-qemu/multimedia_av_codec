/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <cinttypes>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <malloc.h>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <fstream>
#include <ctime>
#include "native_avbuffer.h"
#include "inner_demuxer_parser_sample.h"
#include "native_avcodec_base.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "native_avformat.h"
#include "av_common.h"
#include "layer_info_all_i_avc.h"
#include "layer_info_all_i__hevc.h"
#include "layer_info_ipb_avc.h"
#include "layer_info_ipb_hevc.h"
#include "layer_info_one_i_avc.h"
#include "layer_info_one_i_hevc.h"
#include "layer_info_sdtp_avc.h"
#include "layer_info_sdtp_hevc.h"
#include "layer_info_four_layer_avc.h"
#include "layer_info_four_layer_hevc.h"
#include "layer_info_ltr_avc.h"
#include "layer_info_ltr_hevc.h"
#include "layer_info_three_layer_avc.h"
#include "layer_info_three_layer_hevc.h"
#include "layer_info_two_layer_avc.h"
#include "layer_info_two_layer_hevc.h"
#include <random>

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

using json = nlohmann::json;

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
std::random_device rd;
InnerDemuxerParserSample::InnerDemuxerParserSample(const std::string &filePath)
{
    fd = open(filePath.c_str(), O_RDONLY);
    this->avsource_ = AVSourceFactory::CreateWithFD(fd, 0, GetFileSize(filePath));
    if (!avsource_) {
        printf("Source is null\n");
        return;
    }
    this->demuxer_ = AVDemuxerFactory::CreateWithSource(avsource_);
    if (!demuxer_) {
        printf("AVDemuxerFactory::CreateWithSource is failed\n");
        return;
    }
    int32_t ret = this->avsource_->GetSourceFormat(source_format_);
    if (ret != 0) {
        printf("GetSourceFormat is failed\n");
    }
    source_format_.GetIntValue(OH_MD_KEY_TRACK_COUNT, trackCount);
    source_format_.GetLongValue(OH_MD_KEY_DURATION, duration);
    printf("====>total tracks:%d duration:%" PRId64 "\n", trackCount, duration);
    int32_t trackType = 0;
    for (int32_t i = 0; i < trackCount; i++) {
        ret = this->avsource_->GetTrackFormat(track_format_, i);
        if (ret != 0) {
            printf("GetTrackFormat is failed\n");
        }
        track_format_.GetIntValue(OH_MD_KEY_TRACK_TYPE, trackType);
        if (trackType == MEDIA_TYPE_VID) {
            ret = this->demuxer_->SelectTrackByID(i);
            if (ret != 0) {
                printf("SelectTrackByID is failed\n");
            }
            videoTrackIdx = i;
        }
    }
}

InnerDemuxerParserSample::~InnerDemuxerParserSample()
{
    if (fd > 0) {
        close(fd);
        fd = 0;
    }
    if (avsource_ != nullptr) {
        avsource_ = nullptr;
    }
    if (demuxer_ != nullptr) {
        demuxer_ = nullptr;
    }
}

void InnerDemuxerParserSample::InitParameter(MP4Scene scene)
{
    InitMP4Scene(scene);
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    gopVec_ = gopJson_.get<std::vector<JsonGopInfo>>();
    frameVec_ = frameLayerJson_.get<std::vector<JsonFrameLayerInfo>>();
    for (auto gop : gopVec_) {
        int32_t frameId = gop.startFrameId;
        uint32_t layerMax = 0;
        std::map<uint8_t, uint32_t> layerNumFrame;
        for (int32_t i = 0; i < gop.gopSize; i++) {
            JsonFrameLayerInfo frame = frameVec_[frameId + i];
            if (frame.layer > layerMax) {
                layerMax = frame.layer;
            }
            auto it = layerNumFrame.find(frame.layer);
            if (it != layerNumFrame.end()) {
                layerNumFrame[frame.layer] = layerNumFrame[frame.layer] + 1;
            } else {
                layerNumFrame.emplace(frame.layer, 1);
            }
            frameMap_.emplace(frame.dts, frame);
        }
        gop.layerCount = layerMax + 1;
        gop.layerFrameNum = layerNumFrame;
        frameGopMap_.emplace(gop.gopId, gop);
    }
}

void InnerDemuxerParserSample::InitMP4Scene(MP4Scene scene)
{
    InitAVCScene(scene);
    InitHEVCScene(scene);
}

void InnerDemuxerParserSample::InitAVCScene(MP4Scene scene)
{
    switch (scene) {
        case MP4Scene::ONE_I_FRAME_AVC:
            gopJson_ = GopInfoOneIAvc;
            frameLayerJson_ = FrameLayerInfoOneIAvc;
            break;
        case MP4Scene::ALL_I_FRAME_AVC:
            gopJson_ = GopInfoAllIAvc;
            frameLayerJson_ = FrameLayerInfoAllIAvc;
            break;
        case MP4Scene::IPB_FRAME_AVC:
            gopJson_ = GopInfoIpbAvc;
            frameLayerJson_ = FrameLayerInfoIpbAvc;
            break;
        case MP4Scene::SDTP_FRAME_AVC:
            gopJson_ = GopInfoStdpAvc;
            frameLayerJson_ = FrameLayerInfoStdpAvc;
            break;
        case MP4Scene::OPENGOP_FRAME_AVC:
            break;
        case MP4Scene::LTR_FRAME_AVC:
            gopJson_ = GopInfoLTRAvc;
            frameLayerJson_ = FrameLayerInfoLTRAvc;
            break;
        case MP4Scene::TWO_LAYER_FRAME_AVC:
            gopJson_ = GopInfoTwoLayerAvc;
            frameLayerJson_ = FrameLayerInfoTwoLayerAvc;
            break;
        case MP4Scene::THREE_LAYER_FRAME_AVC:
            gopJson_ = GopInfoThreeLayerAvc;
            frameLayerJson_ = FrameLayerInfoThreeLayerAvc;
            break;
        case MP4Scene::FOUR_LAYER_FRAME_AVC:
            gopJson_ = GopInfoFourLayerAvc;
            frameLayerJson_ = FrameLayerInfoFourLayerAvc;
            break;
        default:
            break;
    }
}

void InnerDemuxerParserSample::InitHEVCScene(MP4Scene scene)
{
    switch (scene) {
        case MP4Scene::ONE_I_FRAME_HEVC:
            gopJson_ = GopInfoOneIHevc;
            frameLayerJson_ = FrameLayerInfoOneIHevc;
            break;
        case MP4Scene::ALL_I_FRAME_HEVC:
            gopJson_ = GopInfoAllIHevc;
            frameLayerJson_ = FrameLayerInfoAllIHevc;
            break;
        case MP4Scene::IPB_FRAME_HEVC:
            gopJson_ = GopInfoIpbHevc;
            frameLayerJson_ = FrameLayerInfoIpbHevc;
            break;
        case MP4Scene::SDTP_FRAME_HEVC:
            gopJson_ = GopInfoStdpHevc;
            frameLayerJson_ = FrameLayerInfoStdpHevc;
            break;
        case MP4Scene::OPENGOP_FRAME_HEVC:
            break;
        case MP4Scene::LTR_FRAME_HEVC:
            gopJson_ = GopInfoLTRHevc;
            frameLayerJson_ = FrameLayerInfoLTRHevc;
            break;
        case MP4Scene::TWO_LAYER_FRAME_HEVC:
            gopJson_ = GopInfoTwoLayerHevc;
            frameLayerJson_ = FrameLayerInfoTwoLayerHevc;
            break;
        case MP4Scene::THREE_LAYER_FRAME_HEVC:
            gopJson_ = GopInfoThreeLayerHevc;
            frameLayerJson_ = FrameLayerInfoThreeLayerHevc;
            break;
        case MP4Scene::FOUR_LAYER_FRAME_HEVC:
            gopJson_ = GopInfoFourLayerHevc;
            frameLayerJson_ = FrameLayerInfoFourLayerHevc;
            break;
        default:
            break;
    }
}

size_t InnerDemuxerParserSample::GetFileSize(const std::string& filePath)
{
    size_t fileSize = 0;
    if (!filePath.empty()) {
        struct stat fileStatus {};
        if (stat(filePath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<size_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

bool InnerDemuxerParserSample::RunSeekScene(WorkPts workPts)
{
    int64_t pts = GetPtsFromWorkPts(workPts);
    float durationNum = 0.0;
    if (pts > duration / durationNum) {
        cout << "pts > duration" << endl;
        return true;
    }
    int32_t ret = 0;
    ret = this->demuxer_->StartReferenceParser(pts);
    cout << "StartReferenceParser pts:" << pts << endl;
    if (ret != 0) {
        cout << "StartReferenceParser fail ret:" << ret << endl;
        return false;
    }
    bool checkResult = true;
    ret = demuxer_->SeekToTime(pts, Media::SeekMode::SEEK_PREVIOUS_SYNC);
    if (ret != 0) {
        cout << "SeekToTime fail ret:" << ret << endl;
        return false;
    }
    usleep(usleepTime);
    FrameLayerInfo frameLayerInfo;
    bool isEosFlag = true;
    int32_t ptsNum = 1000;
    while (isEosFlag) {
        ret = this->demuxer_->ReadSampleBuffer(videoTrackIdx, avBuffer);
        if (ret != 0) {
            cout << "ReadSampleBuffer fail ret:" << ret << endl;
            isEosFlag = false;
            break;
        }
        if (avBuffer->pts_ >= pts * ptsNum || avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
            cout << "read sample end" << endl;
            isEosFlag = false;
            break;
        }
        this->demuxer_->GetFrameLayerInfo(avBuffer, frameLayerInfo);
        cout << "isDiscardable: " << frameLayerInfo.isDiscardable << ", gopId: " << frameLayerInfo.gopId
             << ", layer: " << frameLayerInfo.layer << ", dts_: " << avBuffer->dts_ << endl;
        checkResult = CheckFrameLayerResult(frameLayerInfo, avBuffer->dts_, false);
        if (!checkResult) {
            cout << "CheckFrameLayerResult is false!!" << endl;
            break;
        }
    }
    return checkResult;
}

bool InnerDemuxerParserSample::RunSpeedScene(WorkPts workPts)
{
    int64_t pts = GetPtsFromWorkPts(workPts);
    int32_t ret = 0;
    ret = demuxer_->SeekToTime(pts, Media::SeekMode::SEEK_PREVIOUS_SYNC);
    if (ret != 0) {
        return false;
    }
    ret = this->demuxer_->StartReferenceParser(pts);
    if (ret != 0) {
        return false;
    }
    usleep(usleepTime);
    FrameLayerInfo frameLayerInfo;
    GopLayerInfo gopLayerInfo;
    bool isEosFlag = true;
    bool checkResult = true;
    int num = 0;
    while (isEosFlag) {
        ret = this->demuxer_->ReadSampleBuffer(videoTrackIdx, avBuffer);
        if (ret != 0) {
            isEosFlag = false;
            break;
        }
        if (avBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
            isEosFlag = false;
            break;
        }
        if (avBuffer->pts_ >= pts * num) {
            ret = this->demuxer_->GetFrameLayerInfo(avBuffer, frameLayerInfo);
            if (ret != 0) {
                checkResult = false;
                break;
            }
            checkResult = CheckFrameLayerResult(frameLayerInfo, avBuffer->dts_, true);
            if (!checkResult) {
                break;
            }
            ret = this->demuxer_->GetGopLayerInfo(frameLayerInfo.gopId, gopLayerInfo);
            if (ret != 0) {
                checkResult = false;
                break;
            }
            checkResult = CheckGopLayerResult(gopLayerInfo, frameLayerInfo.gopId);
            if (!checkResult) {
                break;
            }
        }
    }
    return checkResult;
}

bool InnerDemuxerParserSample::CheckFrameLayerResult(FrameLayerInfo &info, int64_t dts, bool speedScene)
{
    JsonFrameLayerInfo frame = frameMap_[dts];
    if ((frame.discardable && info.isDiscardable) || (!frame.discardable && !info.isDiscardable)) {
        if (speedScene) {
            if ((GetGopIdFromFrameId(frame.frameId) != info.gopId) || (frame.layer != info.layer)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool InnerDemuxerParserSample::CheckGopLayerResult(GopLayerInfo &info, int32_t gopId)
{
    JsonGopInfo frame = frameGopMap_[gopId];
    bool conditionOne = (frame.gopSize != info.gopSize);
    bool conditionTwo = (frame.layerCount != info.layerCount);
    bool conditionThree = (!std::equal(frame.layerFrameNum.begin(),
    frame.layerFrameNum.end(), info.layerFrameNum.begin()));
    if (conditionOne || conditionTwo || conditionThree) {
        return false;
    }
    return true;
}

uint32_t InnerDemuxerParserSample::GetGopIdFromFrameId(int32_t frameId)
{
    uint32_t gopId = 0;
    for (auto gop : gopVec_) {
        if ((frameId >= gop.startFrameId) && (frameId < gop.startFrameId + gop.gopSize)) {
            gopId = gop.gopId;
            break;
        }
    }
    return gopId;
}

int64_t InnerDemuxerParserSample::GetPtsFromWorkPts(WorkPts workPts)
{
    int64_t pts = 0;
    float num = 1000.0;
    switch (workPts) {
        case WorkPts::START_PTS:
            pts = 0;
            break;
        case WorkPts::END_PTS:
            pts = duration / num;
            break;
        case WorkPts::RANDOM_PTS:
            srand(time(NULL));
            pts = rd() % duration / num;
            break;
        case WorkPts::SPECIFIED_PTS:
            pts = specified_pts;
            break;
        default:
            break;
    }
    cout << "GetPtsFromWorkPts pts:" << pts << endl;
    return pts;
}
}
}