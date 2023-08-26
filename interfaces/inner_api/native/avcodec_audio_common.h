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

#ifndef MEDIA_AVCODEC_AUDIO_COMMON_H
#define MEDIA_AVCODEC_AUDIO_COMMON_H

#include <cstdint>
namespace OHOS {
namespace MediaAVCodec {
/**
 * @enum Audio sample formats
 * @since 3.1
 * @version 3.1
 */
enum AudioSampleFormat : int32_t {
    SAMPLE_U8 = 0,
    SAMPLE_S16LE = 1,
    SAMPLE_S24LE = 2,
    SAMPLE_S32LE = 3,
    SAMPLE_F32LE = 4,
    SAMPLE_U8P = 5,
    SAMPLE_S16P = 6,
    SAMPLE_S24P = 7,
    SAMPLE_S32P = 8,
    SAMPLE_F32P = 9,
    INVALID_WIDTH = -1
};

/**
 * @enum Audio AAC Profile
 * @brief AAC mode type.  Note that the term profile is used with the MPEG-2
 * standard and the term object type and profile is used with MPEG-4
 * @since 3.1
 * @version 3.1
 */
enum AACProfile : int32_t {
    AAC_PROFILE_LC = 0,
    AAC_PROFILE_ELD = 1,
    AAC_PROFILE_ERLC = 2,
    AAC_PROFILE_HE = 3,
    AAC_PROFILE_HE_V2 = 4,
    AAC_PROFILE_LD = 5,
    AAC_PROFILE_MAIN = 6,
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_AV_COMMOM_H