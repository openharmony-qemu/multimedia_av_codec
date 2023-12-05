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

#ifndef AVCODEC_FFMPEG_MUXER_CONVERTER_H
#define AVCODEC_FFMPEG_MUXER_CONVERTER_H

#include <string>
#include <vector>
#include "meta/video_types.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/error.h"
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
bool Mime2CodecId(const std::string &mime, AVCodecID &codecId);
std::pair<bool, AVColorPrimaries> ColorPrimary2AVColorPrimaries(ColorPrimary primary);
std::pair<bool, AVColorTransferCharacteristic> ColorTransfer2AVColorTransfer(TransferCharacteristic transfer);
std::pair<bool, AVColorSpace> ColorMatrix2AVColorSpace(MatrixCoefficient matrix);
void ReplaceDelimiter(const std::string &delimiters, char newDelimiter, std::string &str);
std::string AVStrError(int errNum);
int64_t ConvertTimeFromFFmpeg(int64_t pts, AVRational base);
int64_t ConvertTimeToFFmpeg(int64_t timestampUs, AVRational base);
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_FFMPEG_MUXER_CONVERTER_H
