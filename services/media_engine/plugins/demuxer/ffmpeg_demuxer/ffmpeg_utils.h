/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef FFMPEG_UTILS_H
#define FFMPEG_UTILS_H

#include <string>
#include "meta/media_types.h"
#include "meta/audio_types.h"
#include "meta/video_types.h"
#include "buffer/avbuffer.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/rational.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
const uint32_t MS_TO_SEC = 1000;
std::string AVStrError(int errnum);
int64_t ConvertTimeFromFFmpeg(int64_t pts, AVRational base);
int64_t ConvertTimeToFFmpeg(int64_t timestampNs, AVRational base);
int64_t ConvertTimeToFFmpegByUs(int64_t timestampUs, AVRational base);
bool StartWith(const char* name, const char* chars);
uint32_t ConvertFlagsFromFFmpeg(const AVPacket& pkt, bool memoryNotEnough);
int64_t CalculateTimeByFrameIndex(AVStream* avStream, int keyFrameIdx);

std::vector<std::string> SplitString(const char* str, char delimiter);
std::vector<std::string> SplitString(const std::string& str, char delimiter);

void SetDropTag(const AVPacket& pkt, std::shared_ptr<AVBuffer> sample, AVCodecID codecId);
int64_t AvTime2Us(int64_t hTime);
bool IsBeginAsAnnexb(const uint8_t *sample, int32_t size);
bool IsHevcSyncFrame(const uint8_t *sample, int32_t size);
bool IsPktOnlyParameterSets(const uint8_t *sample, int32_t size);
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // FFMPEG_UTILS_H
