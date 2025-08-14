/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef DEMUXER_SAMPLE_H
#define DEMUXER_SAMPLE_H

#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

namespace OHOS {
    struct Params {
        int64_t time;
        int64_t setTrackType;
        long setDuration;
        float setHeight;
        double setFrameRate;
        int64_t setCodecConfigSize;
        int32_t sampleRate;
        int32_t channelCount;
        int32_t setVideoHeight;
        int32_t setVideoWidth;
    };
namespace Media {
class DemuxerSample {
public:
    DemuxerSample() = default;
    ~DemuxerSample();
    const char *filePath = "/data/test/fuzz_create.mp4";
    void RunNormalDemuxer(uint32_t createSize, const char *uri, const char *setLanguage, Params params);
    void RunNormalDemuxerApi11(uint32_t createSize, const char *uri, const char *setLanguage, Params params);
    void GetAndSetFormat(const char *setLanguage, Params params);
private:
    void ResetFlag();
    int CreateDemuxer();
    int32_t gWidth = 3840;
    int32_t gHeight = 2160;
    int gTrackType = 0;
    int32_t gTrackCount;
    OH_AVCodecBufferAttr attr;
    bool gReadEnd = false;
    int fd;
    OH_AVFormat *sourceFormat;
    OH_AVFormat *userFormat;
    OH_AVFormat *format;
    OH_AVFormat *audioFormat;
    OH_AVFormat *videoFormat;
    OH_AVSource *source;
    OH_AVSource *uriSource;
    OH_AVDemuxer *demuxer;
    OH_AVMemory *memory;
    OH_AVBuffer *buffer;
    const uint8_t *gBaseFuzzData = nullptr;
    size_t gBaseFuzzSize = 0;
    size_t gBaseFuzzPos;
};
} // namespace Media
} // namespace OHOS

#endif // DEMUXER_SAMPLE_H
