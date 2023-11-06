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

#ifndef FFMPEG_UTILS_H
#define FFMPEG_UTILS_H

#include <string>
#include <type_traits>
#include <vector>
#include "media_description.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/error.h"
#ifdef __cplusplus
};
#endif


namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace Ffmpeg {
bool Mime2CodecId(const std::string_view &mime, AVCodecID &codecId);
std::pair<bool, AVColorPrimaries> ColorPrimary2AVColorPrimaries(ColorPrimary primary);
std::pair<bool, AVColorTransferCharacteristic> ColorTransfer2AVColorTransfer(TransferCharacteristic transfer);
std::pair<bool, AVColorSpace> ColorMatrix2AVColorSpace(MatrixCoefficient matrix);

void ReplaceDelimiter(const std::string &delmiters, char newDelimiter, std::string &str);
std::vector<std::string> SplitString(const char* str, char delimiter);
std::vector<std::string> SplitString(const std::string &str, char delimiter);

std::string AVStrError(int errnum);

/**
 * Convert time from ffmpeg to time in HST_TIME_BASE.
 * @param pts ffmpeg time
 * @param base ffmpeg time_base
 * @return time in HST_TIME_BASE
 */
int64_t ConvertTimeFromFFmpeg(int64_t pts, AVRational base);

/**
 * Convert time in TIME_BASE to ffmpeg time.
 * @param time time in HST_TIME_BASE
 * @param base ffmpeg time_base
 * @return time in ffmpeg.
 */
int64_t ConvertTimeToFFmpeg(int64_t timestampUs, AVRational base);
} // namespace Ffmpeg
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FFMPEG_UTILS_H
