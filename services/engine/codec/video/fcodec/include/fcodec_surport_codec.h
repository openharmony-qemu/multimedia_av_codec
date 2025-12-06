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

#ifndef FCODEC_SURPORT_CODEC_H
#define FCODEC_SURPORT_CODEC_H

#include "avcodec_codec_name.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

// 解码器信息结构体
struct CodecInfo {
    const std::string_view codecName;
    const std::string_view mimeType;
    const char *ffmpegCodec;
};

// 解码器支持列表
constexpr CodecInfo SUPPORT_VCODEC[] = {
    {AVCodecCodecName::VIDEO_DECODER_AVC_NAME, CodecMimeType::VIDEO_AVC, "h264"},           // H.264/AVC 视频解码器
    {AVCodecCodecName::VIDEO_DECODER_H263_NAME, CodecMimeType::VIDEO_H263, "h263"},         // H.263 视频解码器
    {AVCodecCodecName::VIDEO_DECODER_MPEG2_NAME, CodecMimeType::VIDEO_MPEG2, "mpeg2video"}, // MPEG-2 视频解码器
    {AVCodecCodecName::VIDEO_DECODER_MPEG4_NAME, CodecMimeType::VIDEO_MPEG4, "mpeg4"},      // MPEG-4 视频解码器
#ifdef SUPPORT_CODEC_VC1
    {AVCodecCodecName::VIDEO_DECODER_VC1_NAME, CodecMimeType::VIDEO_VC1, "vc1"},            // VC-1 视频解码器
    {AVCodecCodecName::VIDEO_DECODER_WVC1_NAME, CodecMimeType::VIDEO_WVC1, "vc1"},            // WVC1 视频解码器
#endif
    {AVCodecCodecName::VIDEO_DECODER_MSVIDEO1_NAME, CodecMimeType::VIDEO_MSVIDEO1, "msvideo1"}, // MS Video 1 视频解码器
    {AVCodecCodecName::VIDEO_DECODER_WMV3_NAME, CodecMimeType::VIDEO_WMV3, "wmv3"},             // WMV3 视频解码器
    {AVCodecCodecName::VIDEO_DECODER_MJPEG_NAME, CodecMimeType::VIDEO_MJPEG, "mjpeg"},          // MJPEG 视频解码器
#ifdef SUPPORT_CODEC_RV
    {AVCodecCodecName::VIDEO_DECODER_RV30_NAME, CodecMimeType::VIDEO_RV30, "rv30"}, // RV30 视频解码器
    {AVCodecCodecName::VIDEO_DECODER_RV40_NAME, CodecMimeType::VIDEO_RV40, "rv40"}, // RV40 视频解码器
#endif
};

// 支持的解码器数量
constexpr uint32_t SUPPORT_VCODEC_NUM = sizeof(SUPPORT_VCODEC) / sizeof(SUPPORT_VCODEC[0]);
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_SURPORT_CODEC_H