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

#ifndef CODECLIST_UTILS_H
#define CODECLIST_UTILS_H
namespace OHOS {
namespace MediaAVCodec {
/**
 * @brief Codec Type
 *
 * @since 3.1
 * @version 3.1
 */
enum class CodecType : int32_t {
    AVCODEC_INVALID = -1,
    AVCODEC_HCODEC = 0,
    AVCODEC_VIDEO_CODEC,
    AVCODEC_VIDEO_HEVC_DECODER,
    AVCODEC_AUDIO_CODEC,
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_UTILS_H