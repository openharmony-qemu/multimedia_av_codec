/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_AUDIO_AVBUFFER_MUXER_DEMO_H
#define AVCODEC_AUDIO_AVBUFFER_MUXER_DEMO_H

#include <unistd.h>
#include <fcntl.h>
#include "avmuxer.h"
#include "nocopyable.h"
#include "avcodec_errors.h"
#include "native_avmuxer.h"
#include "native_avcodec_base.h"
#include "common/native_mfmagic.h"

namespace OHOS {
namespace MediaAVCodec {
namespace AudioBufferDemo {

enum class AudioMuxerFormatType : int32_t {
    TYPE_AAC = 0,
    TYPE_H264 = 1,
    TYPE_H265 = 2,
    TYPE_MPEG4 = 3,
    TYPE_HDR = 4,
    TYPE_JPG = 5,
    TYPE_AMRNB = 6,
    TYPE_AMRWB = 7,
    TYPE_MPEG3 = 8,
    TYPE_MAX = 20,
};

typedef struct AudioTrackParam {
    const char* mimeType;
    int32_t sampleRate;
    int32_t channels;
    int32_t frameSize;
    int32_t width;
    int32_t height;
    double frameRate;
    int32_t videoDelay;
    int32_t colorPrimaries;
    int32_t colorTransfer;
    int32_t colorMatrixCoeff;
    int32_t colorRange;
    int32_t isHdrVivid;
    bool isNeedCover;
    bool isNeedVideo;
} AudioTrackParam;

class AVMuxerDemo : public NoCopyable {
public:
    AVMuxerDemo();
    ~AVMuxerDemo();

    bool InitFile(const std::string& inputFile);
    bool RunCase(const uint8_t *data, size_t size);

    int32_t AddTrack(OH_AVMuxer* muxer, int32_t& trackIndex, AudioTrackParam param);
    int32_t AddCoverTrack(OH_AVMuxer* muxer, int32_t& trackId, AudioTrackParam param);

    AudioTrackParam InitFormatParam(AudioMuxerFormatType type);
    OH_AVMuxer* Create();
    int32_t Start();
    int32_t Stop();
    int32_t Destroy();

    int32_t SetRotation(OH_AVMuxer* muxer, int32_t rotation);

    void WriteTrackCover(OH_AVMuxer *muxer, int32_t trackIndex);
    void WriteSingleTrackSampleAVBuffer(OH_AVMuxer *muxer, int32_t trackIndex);
    bool UpdateWriteBufferInfoAVBuffer(OH_AVBuffer **buffer, OH_AVCodecBufferAttr *info);
    void SetParameter(const uint8_t *data, size_t size);

private:
    OH_AVMuxer *avmuxer_ = {nullptr};
    OH_AVBuffer *avbuffer = {nullptr};
    int32_t outputFd_ = {-1};
    size_t inputdatasize = 0;
    std::string inputdata;
    std::string output_path_;
    OH_AVOutputFormat output_format_;
    AudioMuxerFormatType audioType_;
};
} // namespace AudioBufferDemo
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_AVBUFFER_MUXER_DEMO_H
