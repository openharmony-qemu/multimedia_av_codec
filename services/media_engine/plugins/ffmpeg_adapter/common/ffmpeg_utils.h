/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include "meta/video_types.h"
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
bool Mime2CodecId(const std::string &mime, AVCodecID &codecId);
std::string AVStrError(int errnum);
int64_t ConvertTimeFromFFmpeg(int64_t pts, AVRational base);
int64_t ConvertPts(int64_t pts, int64_t startTime);
int64_t ConvertTimeToFFmpeg(int64_t timestampUs, AVRational base);
std::string_view ConvertFFmpegMediaTypeToString(AVMediaType mediaType);
bool StartWith(const char* name, const char* chars);
uint32_t ConvertFlagsFromFFmpeg(const AVPacket& pkt, bool memoryNotEnough);
int64_t CalculateTimeByFrameIndex(AVStream* avStream, int keyFrameIdx);
void ReplaceDelimiter(const std::string &delimiters, char newDelimiter, std::string &str);

std::vector<std::string> SplitString(const char* str, char delimiter);
std::vector<std::string> SplitString(const std::string& str, char delimiter);

std::pair<bool, AVColorPrimaries> ColorPrimary2AVColorPrimaries(ColorPrimary primary);
std::pair<bool, AVColorTransferCharacteristic> ColorTransfer2AVColorTransfer(TransferCharacteristic transfer);
std::pair<bool, AVColorSpace> ColorMatrix2AVColorSpace(MatrixCoefficient matrix);

std::vector<uint8_t> GenerateAACCodecConfig(int32_t profile, int32_t sampleRate, int32_t channels);
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // FFMPEG_UTILS_H
