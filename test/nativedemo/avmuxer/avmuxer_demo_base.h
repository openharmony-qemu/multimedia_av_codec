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
#ifndef AVMUXER_DEMO_BASE
#define AVMUXER_DEMO_BASE

#include "avmuxer.h"
#include "avmuxer_demo_common.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxerDemoBase {
public:
    AVMuxerDemoBase();
    virtual ~AVMuxerDemoBase() = default;
    void RunCase();
    void RunMultiThreadCase();
    std::string GetOutputFileName(std::string header);

protected:
    virtual void DoRunMuxer() = 0;
    virtual void DoRunMultiThreadCase()= 0;
    virtual int DoWriteSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample) = 0;
    virtual int DoAddTrack(int32_t &trackIndex, std::shared_ptr<Meta> trackDesc) = 0;
    virtual sptr<AVBufferQueueProducer> DoGetInputBufferQueue(uint32_t trackIndex) { return nullptr; };
    int AddVideoTrack(const VideoTrackParam *param);
    int AddAudioTrack(const AudioTrackParam *param);
    int AddCoverTrack(const VideoTrackParam *param);
    void WriteTrackSample();
    void WriteAvTrackSample();
    void WriteSingleTrackSample(uint32_t trackId, std::shared_ptr<std::ifstream> file);
    void WriteCoverSample();
    void WriteTrackSampleByBufferQueue();
    void WriteAvTrackSampleByBufferQueue();
    void WriteSingleTrackSampleByBufferQueue(sptr<AVBufferQueueProducer> bufferQueue,
        std::shared_ptr<std::ifstream> file);
    void SelectFormatMode();
    void SelectAudioMode();
    void SelectVideoMode();
    void SelectCoverMode();
    int SelectMode();
    int SelectModeAndOpenFile();
    bool ReadSampleDataInfo(std::shared_ptr<std::ifstream> file, std::shared_ptr<AVBuffer> &buffer);
    bool ReadSampleDataInfoByBufferQueue(std::shared_ptr<std::ifstream> file, std::shared_ptr<AVBuffer> &buffer,
        sptr<AVBufferQueueProducer> bufferQueue);
    void Reset();
    static void MulThdWriteTrackSample(AVMuxerDemoBase *muxerBase, uint32_t trackId,
        std::shared_ptr<std::ifstream> file);
    static void MulThdWriteTrackSampleByBufferQueue(AVMuxerDemoBase *muxerBase,
        sptr<AVBufferQueueProducer> bufferQueue, std::shared_ptr<std::ifstream> file);

    const static AudioTrackParam *audioParams_;
    const static VideoTrackParam *videoParams_;
    const static VideoTrackParam *coverParams_;
    static std::string videoType_;
    static std::string audioType_;
    static std::string coverType_;
    static std::string format_;
    static Plugins::OutputFormat outputFormat_;
    static bool hasSetMode_;

    int32_t videoTrackId_ {-1};
    int32_t audioTrackId_ {-1};
    int32_t coverTrackId_ {-1};
    sptr<AVBufferQueueProducer> videoBufferQueue_ {nullptr};
    sptr<AVBufferQueueProducer> audioBufferQueue_ {nullptr};
    sptr<AVBufferQueueProducer> coverBufferQueue_ {nullptr};
    std::shared_ptr<std::ifstream> audioFile_ {nullptr};
    std::shared_ptr<std::ifstream> videoFile_ {nullptr};
    std::shared_ptr<std::ifstream> coverFile_ {nullptr};
    int32_t outFd_ {-1};
    int64_t audioPts_ {0};
    int64_t videoPts_ {0};
};
} // MediaAVCodec
} // OHOS
#endif