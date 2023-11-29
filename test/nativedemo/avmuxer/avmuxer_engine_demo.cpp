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

#include "avmuxer_engine_demo.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <vector>

namespace OHOS {
namespace MediaAVCodec {
int AVMuxerEngineDemo::DoWriteSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample)
{
    if (avmuxer_ != nullptr &&
        avmuxer_->WriteSample(trackIndex, sample) == Status::OK) {
            return 0;
    }
    return -1;
}

int AVMuxerEngineDemo::DoAddTrack(int32_t &trackIndex, std::shared_ptr<Meta> trackDesc)
{
    Status ret;
    if ((ret = avmuxer_->AddTrack(trackIndex, trackDesc)) != Status::OK) {
        std::cout<<"AVMuxerDemo::DoAddTrack failed! ret:"<<static_cast<int32_t>(ret)<<std::endl;
        return -1;
    }
    return 0;
}

void AVMuxerEngineDemo::DoRunMuxer(const std::string &runMode)
{
    std::string outFileName = "engine_mux_" + runMode + "_" + audioType_ + "_" +
        videoType_ + "_" + coverType_ + "." + format_;
    outFd_ = open(outFileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (outFd_ < 0) {
        std::cout << "Open file failed! filePath is: " << outFileName << std::endl;
        return;
    }
    std::cout<<"==== open success! =====\noutputFileName: "<<outFileName<<"\n============"<<std::endl;
    long long testTimeStart = GetTimestamp();
    avmuxer_ = std::make_shared<MediaMuxer>(-1, -1);
    if (avmuxer_ == nullptr || avmuxer_->Init(outFd_, outputFormat_) != Status::OK) {
        std::cout << "avmuxer_ is null" << std::endl;
        return;
    }

    std::cout << "create muxer success " << avmuxer_ << std::endl;

    AddAudioTrack(audioParams_);
    AddVideoTrack(videoParams_);
    AddCoverTrack(coverParams_);

    std::cout << "add track success" << std::endl;

    if (avmuxer_->Start() != Status::OK) {
        return;
    }

    std::cout << "start muxer success" << std::endl;

    WriteCoverSample();

    std::cout<<"AVMuxerDemo::DoRunMuxer runMode is : "<<runMode<<std::endl;
    if (runMode.compare(RUN_NORMAL) == 0) {
        WriteTrackSample();
    } else if (runMode.compare(RUN_MUL_THREAD) == 0) {
        std::vector<std::thread> vecThread;
        vecThread.emplace_back(MulThdWriteTrackSample, this, audioTrackId_, audioFile_);
        vecThread.emplace_back(MulThdWriteTrackSample, this, videoTrackId_, videoFile_);
        for (uint32_t i = 0; i < vecThread.size(); ++i) {
            vecThread[i].join();
        }
    }

    std::cout << "write muxer success" << std::endl;

    if (avmuxer_->Stop() != Status::OK) {
        return;
    }
    std::cout << "stop muxer success" << std::endl;
    long long testTimeEnd = GetTimestamp();
    std::cout << "muxer used time: " << testTimeEnd - testTimeStart << "us" << std::endl;
}

void AVMuxerEngineDemo::DoRunMuxer()
{
    DoRunMuxer(std::string(RUN_NORMAL));
}

void AVMuxerEngineDemo::DoRunMultiThreadCase()
{
    DoRunMuxer(std::string(RUN_MUL_THREAD));
}
}  // namespace MediaAVCodec
}  // namespace OHOS