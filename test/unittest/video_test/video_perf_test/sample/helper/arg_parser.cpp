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

#include "arg_parser.h"
#include <iostream>
#include <getopt.h>

namespace {
enum DemoShortArgument : int {
    DEMO_ARG_UNKNOW = 0,
    DEMO_ARG_HELP,
    DEMO_ARG_CODEC_TYPE,
    DEMO_ARG_INPUT_FILE,
    DEMO_ARG_CODEC_MIME,
    DEMO_ARG_WIDTH,
    DEMO_ARG_HEIGHT,
    DEMO_ARG_FRAMERATE,
    DEMO_ARG_PIXEL_FORMAT,
    DEMO_ARG_BITRATE,
    DEMO_ARG_BITRATE_MODE,
    DEMO_ARG_CODEC_RUN_MODE,
    DEMO_ARG_FRAME_INTERVAL,
    DEMO_ARG_REPEAT_TIMES,
    DEMO_ARG_HDR_VIVID_VIDEO,
    DEMO_ARG_NEED_DUMP_OUTPUT,
    DEMO_ARG_MAX_FRAMES,
};

constexpr struct option DEMO_LONG_ARGUMENT[] = {
    {"help",                no_argument,        nullptr, DEMO_ARG_HELP},
    {"codec_type",          required_argument,  nullptr, DEMO_ARG_CODEC_TYPE},
    {"file",                required_argument,  nullptr, DEMO_ARG_INPUT_FILE},
    {"mime",                required_argument,  nullptr, DEMO_ARG_CODEC_MIME},
    {"width",               required_argument,  nullptr, DEMO_ARG_WIDTH},
    {"height",              required_argument,  nullptr, DEMO_ARG_HEIGHT},
    {"framerate",           required_argument,  nullptr, DEMO_ARG_FRAMERATE},
    {"pixel_format",        required_argument,  nullptr, DEMO_ARG_PIXEL_FORMAT},
    {"bitrate",             required_argument,  nullptr, DEMO_ARG_BITRATE},
    {"bitrate_mode",        required_argument,  nullptr, DEMO_ARG_BITRATE_MODE},
    {"codec_run_mode",      required_argument,  nullptr, DEMO_ARG_CODEC_RUN_MODE},
    {"frame_interval",      required_argument,  nullptr, DEMO_ARG_FRAME_INTERVAL},
    {"repeat_times",        required_argument,  nullptr, DEMO_ARG_REPEAT_TIMES},
    {"hdr_vivid_video",     required_argument,  nullptr, DEMO_ARG_HDR_VIVID_VIDEO},
    {"need_dump_output",    required_argument,  nullptr, DEMO_ARG_NEED_DUMP_OUTPUT},
    {"max_frames",          required_argument,  nullptr, DEMO_ARG_MAX_FRAMES},
};

const std::string HELP_TEXT =
R"HELP_TEXT(Video codec demo help:
    --help                      print this help info

    --codec_type                codec type (0: decoder; 1: encoder)
    --file                      input file path
    --mime                      codec mime (H264: video/avc; H265: video/hevc)
    --width                     video width
    --height                    video height
    --framerate                 video framerate
    --pixel_format              1: YUVI420          2: NV12     3: NV21
                                4: SURFACE_FORMAT   5: RGBA
    --bitrate                   encoder bitrate (bps)
    --bitrate_mode              encoder bitrate mode (0: CBR; 1: VBR; 2: CQ)
    --codec_run_mode            0: Surface origin      1: Buffer SharedMemory
                                2: Surface AVBuffer    3: Buffer AVBuffer

    --frame_interval            frame push interval (ms)
    --repeat_times              demo repeat times
    --hdr_vivid_video           input file is hdr vivid video? (0: false; 1: true)
    --need_dump_output          need to dump output stream? (0: false; 1: true)
    --max_frames                number of frames to be processed

Example:
    --codec_type 0 --file input.h264 --mime video/avc --width 1280 --height 720 --framerate 30 --pixel_format 1
    --codec_run_mode 0 --frame_interval 0 --repeat_times 1 --hdr_vivid_video 0 --need_dump_output 0 --max_frames 100
)HELP_TEXT";

void ShowHelp()
{
    std::cout << HELP_TEXT << std::endl;
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
SampleInfo ParseDemoArg(int argc, char *argv[])
{
    SampleInfo info;
    int32_t argType = DEMO_ARG_UNKNOW;
    while ((argType = getopt_long(argc, argv, "", DEMO_LONG_ARGUMENT, nullptr)) != -1) {
        switch (argType) {
            case DEMO_ARG_HELP:
                ShowHelp();
                break;
            case DEMO_ARG_CODEC_TYPE:
                info.codecType = static_cast<CodecType>(std::stol(optarg));
                break;
            case DEMO_ARG_INPUT_FILE:
                info.inputFilePath = optarg;
                break;
            case DEMO_ARG_CODEC_MIME:
                info.codecMime = optarg;
                break;
            case DEMO_ARG_WIDTH:
                info.videoWidth = std::stol(optarg);
                break;
            case DEMO_ARG_HEIGHT:
                info.videoHeight = std::stol(optarg);
                break;
            case DEMO_ARG_FRAMERATE:
                info.frameRate = std::stof(optarg);
                break;
            case DEMO_ARG_PIXEL_FORMAT:
                info.pixelFormat = static_cast<OH_AVPixelFormat>(std::stol(optarg));
                break;
            case DEMO_ARG_BITRATE:
                info.bitrate = std::stoll(optarg);
                break;
            case DEMO_ARG_BITRATE_MODE:
                info.bitrate = std::stol(optarg);
                break;
            case DEMO_ARG_CODEC_RUN_MODE:
                info.codecRunMode = static_cast<CodecRunMode>(std::stol(optarg));
                break;
            case DEMO_ARG_FRAME_INTERVAL:
                info.frameInterval = std::stol(optarg);
                break;
            case DEMO_ARG_REPEAT_TIMES:
                info.repeatTimes = std::stoul(optarg);
                break;
            case DEMO_ARG_HDR_VIVID_VIDEO:
                info.isHDRVivid = std::stol(optarg);
                break;
            case DEMO_ARG_NEED_DUMP_OUTPUT:
                info.needDumpOutput = std::stol(optarg);
                break;
            case DEMO_ARG_MAX_FRAMES:
                info.maxFrames = std::stoul(optarg);
                break;
            default:
                std::cout << "Unknow arg type: " << argType << ", value: " << optarg << std::endl;
                break;
        }
    }
    return info;
}
} // Sample
} // MediaAVCodec
} // OHOS
