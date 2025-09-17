/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FILE_INFO_H
#define FILE_INFO_H

#include "meta/meta_key.h"
#include "meta/media_types.h"
#include "base_config.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
// source format: {{key, value}, {key, value}, {key, value}}
// type order：Int, Long, Float, Double, String, Buffer, IntBuffer
SourceInfoMap  sourceInfoMap = {
    {"test_264_B_Gop25_4sec_cover.mp4",
        {{{Tag::MEDIA_TRACK_COUNT, 3}, {Tag::MEDIA_HAS_VIDEO, 1}, {Tag::MEDIA_HAS_AUDIO, 1},
          {Tag::MEDIA_FILE_TYPE, static_cast<int32_t>(FileType::MP4)}},
         {{Tag::MEDIA_DURATION, 4120000}},
         {}, {},
         {{Tag::MEDIA_TITLE, "title"}, {Tag::MEDIA_ARTIST, "artist"}, {Tag::MEDIA_ALBUM, "album"},
          {Tag::MEDIA_ALBUM_ARTIST, "album artist"}, {Tag::MEDIA_DATE, "2023"}, {Tag::MEDIA_COMMENT, "comment"},
          {Tag::MEDIA_GENRE, "genre"}, {Tag::MEDIA_COPYRIGHT, "Copyright"}, {Tag::MEDIA_LYRICS, "lyrics"},
          {Tag::MEDIA_DESCRIPTION, "description"}},
         {}, {}}
    },
};

// track format: {trackIndex, {{key, value}, {key, value}, {key, value}}}
// type order：Int, Long, Float, Double, String, Buffer, IntBuffer
TrackInfoMap trackInfoMap = {
    {"test_264_B_Gop25_4sec_cover.mp4",
        {{0, {{{Tag::MEDIA_TYPE, MediaType::MEDIA_TYPE_VID},
               {Tag::VIDEO_WIDTH, 1920}, {Tag::VIDEO_HEIGHT, 1080}},
              {{Tag::MEDIA_BITRATE, 7782407}}, {}, {{Tag::VIDEO_FRAME_RATE, 25.000000}},
              {{Tag::MIME_TYPE, MimeType::VIDEO_AVC}}, {}, {}}},
         {1, {{{Tag::MEDIA_TYPE, MediaType::MEDIA_TYPE_AUD},
               {Tag::AUDIO_SAMPLE_RATE, 44100}, {Tag::AUDIO_CHANNEL_COUNT, 2},
               {Tag::AUDIO_AAC_IS_ADTS, 1}, {Tag::AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32P}},
              {{Tag::MEDIA_BITRATE, 128563}, {Tag::AUDIO_CHANNEL_LAYOUT, AudioChannelLayout::STEREO}},
              {}, {}, {{Tag::MIME_TYPE, MimeType::AUDIO_AAC}}, {}, {}}}},
    },
};

// read info: {trackIndex, {frameCount, keyFrameCount}}
ReadInfoMap  readInfoMap = {
    {"test_264_B_Gop25_4sec_cover.mp4", {{0, {103, 5}}, {1, {174, 174}}}},
};

// seek info: {seekTime, {seekSuccess, {{trackIndex, frameCount}, {trackIndex, frameCount}}}}
// seek mode order: next, previous, closest
SeekInfoMap  seekInfoMap = {
    {"test_264_B_Gop25_4sec_cover.mp4",
        {{0,    {{true,  {{0, 103}, {1, 174}}}, {true,  {{0, 103}, {1, 174}}}, {true,  {{0, 103}, {1, 174}}}}},
         {2000, {{true,  {{0, 53},  {1, 90}}},  {true,  {{0, 53},  {1, 91}}},  {true,  {{0, 53},  {1, 91}}}}},
         {1920, {{true,  {{0, 53},  {1, 90}}},  {true,  {{0, 78},  {1, 134}}}, {true,  {{0, 53},  {1, 90}}}}},
         {2160, {{true,  {{0, 28},  {1, 47}}},  {true,  {{0, 53},  {1, 91}}},  {true,  {{0, 53},  {1, 91}}}}},
         {2200, {{true,  {{0, 28},  {1, 47}}},  {true,  {{0, 53},  {1, 91}}},  {true,  {{0, 53},  {1, 91}}}}},
         {2440, {{true,  {{0, 28},  {1, 47}}},  {true,  {{0, 53},  {1, 91}}},  {true,  {{0, 28},  {1, 47}}}}},
         {2600, {{true,  {{0, 28},  {1, 47}}},  {true,  {{0, 53},  {1, 91}}},  {true,  {{0, 28},  {1, 47}}}}},
         {2700, {{true,  {{0, 28},  {1, 47}}},  {true,  {{0, 53},  {1, 91}}},  {true,  {{0, 28},  {1, 47}}}}},
         {4080, {{false, {}},                   {true,  {{0, 3},   {1, 5}}},   {true,  {{0, 3},   {1, 5}}}}},
         {4100, {{false, {}},                   {true,  {{0, 3},   {1, 5}}},   {true,  {{0, 3},   {1, 5}}}}}},
    },
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FILE_INFO_H