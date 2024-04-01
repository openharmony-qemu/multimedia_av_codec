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
#ifndef FFMPEG_DECODER_PLUGIN_H
#define FFMPEG_DECODER_PLUGIN_H

#include <functional>
#include <memory>
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "aac/ffmpeg_aac_decoder_plugin.h"
#include "flac/ffmpeg_flac_decoder_plugin.h"
#include "mp3/ffmpeg_mp3_decoder_plugin.h"
#include "vorbis/ffmpeg_vorbis_decoder_plugin.h"
#include "amrnb/ffmpeg_amrnb_decoder_plugin.h"
#include "amrwb/ffmpeg_amrwb_decoder_plugin.h"
#include "ape/ffmpeg_ape_decoder_plugin.h"

#endif // FFMPEG_DECODER_PLUGIN_H
