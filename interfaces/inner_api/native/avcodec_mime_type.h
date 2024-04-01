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

#ifndef MEDIA_AVCODEC_MIME_TYPE_H
#define MEDIA_AVCODEC_MIME_TYPE_H
#include <string_view>

namespace OHOS {
namespace MediaAVCodec {
/**
 * @enum Media mime type
 *
 * @since 4.0
 */
class AVCodecMimeType {
public:
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_AAC = "audio/mp4a-latm";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_FLAC = "audio/flac";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_VORBIS = "audio/vorbis";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_AMRNB = "audio/3gpp";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_OPUS = "audio/opus";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_MPEG = "audio/mpeg";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_AMRWB = "audio/amr-wb";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_VIVID = "audio/av3a";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_G711MU = "audio/g711mu";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_LBVC = "audio/lbvc";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_APE = "audio/x-ape";

    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_HEVC = "video/hevc";

    static constexpr std::string_view MEDIA_MIMETYPE_IMAGE_JPG = "image/jpeg";
    static constexpr std::string_view MEDIA_MIMETYPE_IMAGE_PNG = "image/png";
    static constexpr std::string_view MEDIA_MIMETYPE_IMAGE_BMP = "image/bmp";

private:
    AVCodecMimeType() = delete;
    ~AVCodecMimeType() = delete;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_MIME_TYPE_H