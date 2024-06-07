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
#include <unordered_map>

namespace {
using namespace OHOS::MediaAVCodec::Sample;
enum DemoArgumentType : int {
    DEMO_ARG_UNKNOW = 0,
    DEMO_ARG_HELP,
    DEMO_ARG_CODEC_TYPE,
    DEMO_ARG_INPUT_FILE,
    DEMO_ARG_OUTPUT_FILE,
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
    DEMO_ARG_DATA_PRODUCER,
    DEMO_ARG_BITSTREAM_TYPE,
    DEMO_ARG_SEEK_MODE,
    DEMO_ARG_CODEC_COMSUMER,
    DEMO_ARG_THREAD_SLEEP_MODE,
    DEMO_ARG_ENCODER_SURFACE_MAX_INPUT_BUFFER,
    DEMO_ARG_END,
};

const std::unordered_map<DemoArgumentType, std::string> DEMO_ARGUMENT_TYPE_TO_STRING = {
    {DEMO_ARG_CODEC_TYPE,                       "codec_type"},
    {DEMO_ARG_INPUT_FILE,                       "input_file"},
    {DEMO_ARG_OUTPUT_FILE,                      "output_file"},
    {DEMO_ARG_CODEC_MIME,                       "codec_mime"},
    {DEMO_ARG_WIDTH,                            "width"},
    {DEMO_ARG_HEIGHT,                           "height"},
    {DEMO_ARG_FRAMERATE,                        "framerate"},
    {DEMO_ARG_PIXEL_FORMAT,                     "pixel_format"},
    {DEMO_ARG_BITRATE,                          "bitrate"},
    {DEMO_ARG_BITRATE_MODE,                     "bitrate_mode"},
    {DEMO_ARG_CODEC_RUN_MODE,                   "codec_run_mode"},
    {DEMO_ARG_FRAME_INTERVAL,                   "frame_interval"},
    {DEMO_ARG_REPEAT_TIMES,                     "repeat_times"},
    {DEMO_ARG_HDR_VIVID_VIDEO,                  "hdr_vivid_video"},
    {DEMO_ARG_NEED_DUMP_OUTPUT,                 "need_dump_output"},
    {DEMO_ARG_MAX_FRAMES,                       "max_frames"},
    {DEMO_ARG_DATA_PRODUCER,                    "data_producer"},
    {DEMO_ARG_BITSTREAM_TYPE,                   "bitstream_type"},
    {DEMO_ARG_SEEK_MODE,                        "seek_mode"},
    {DEMO_ARG_CODEC_COMSUMER,                   "codec_comsumer"},
    {DEMO_ARG_THREAD_SLEEP_MODE,                "thread_sleep_mode"},
    {DEMO_ARG_ENCODER_SURFACE_MAX_INPUT_BUFFER, "encoder_surface_max_input_buffer"},
};

constexpr struct option DEMO_LONG_ARGUMENT[] = {
    {"help",                             no_argument,        nullptr, DEMO_ARG_HELP},
    {"codec_type",                       required_argument,  nullptr, DEMO_ARG_CODEC_TYPE},
    {"input",                            required_argument,  nullptr, DEMO_ARG_INPUT_FILE},
    {"output",                           required_argument,  nullptr, DEMO_ARG_OUTPUT_FILE},
    {"mime",                             required_argument,  nullptr, DEMO_ARG_CODEC_MIME},
    {"width",                            required_argument,  nullptr, DEMO_ARG_WIDTH},
    {"height",                           required_argument,  nullptr, DEMO_ARG_HEIGHT},
    {"framerate",                        required_argument,  nullptr, DEMO_ARG_FRAMERATE},
    {"pixel_format",                     required_argument,  nullptr, DEMO_ARG_PIXEL_FORMAT},
    {"bitrate",                          required_argument,  nullptr, DEMO_ARG_BITRATE},
    {"bitrate_mode",                     required_argument,  nullptr, DEMO_ARG_BITRATE_MODE},
    {"codec_run_mode",                   required_argument,  nullptr, DEMO_ARG_CODEC_RUN_MODE},
    {"frame_interval",                   required_argument,  nullptr, DEMO_ARG_FRAME_INTERVAL},
    {"repeat_times",                     required_argument,  nullptr, DEMO_ARG_REPEAT_TIMES},
    {"hdr_vivid_video",                  required_argument,  nullptr, DEMO_ARG_HDR_VIVID_VIDEO},
    {"need_dump_output",                 required_argument,  nullptr, DEMO_ARG_NEED_DUMP_OUTPUT},
    {"max_frames",                       required_argument,  nullptr, DEMO_ARG_MAX_FRAMES},
    {"data_producer",                    required_argument,  nullptr, DEMO_ARG_DATA_PRODUCER},
    {"bitstream_type",                   required_argument,  nullptr, DEMO_ARG_BITSTREAM_TYPE},
    {"seek_mode",                        required_argument,  nullptr, DEMO_ARG_SEEK_MODE},
    {"codec_comsumer",                   required_argument,  nullptr, DEMO_ARG_CODEC_COMSUMER},
    {"thread_sleep_mode",                required_argument,  nullptr, DEMO_ARG_THREAD_SLEEP_MODE},
    {"encoder_surface_max_input_buffer", required_argument,  nullptr, DEMO_ARG_ENCODER_SURFACE_MAX_INPUT_BUFFER},
};

constexpr std::string_view HELP_TEXT = R"HELP_TEXT(
Video codec demo help:
    --help                              print this help info

    --codec_type                        codec type (0: decoder; 2: encoder)
    --input                             input file path
    --output                            output file path
    --mime                              codec mime (video/avc: H264; video/hevc: H265)
    --width                             video width
    --height                            video height
    --framerate                         video framerate
    --pixel_format                      1: YUVI420          2: NV12     3: NV21
                                        4: SURFACE_FORMAT   5: RGBA
    --bitrate                           encoder bitrate (bps)
    --bitrate_mode                      encoder bitrate mode (0: CBR; 1: VBR; 2: CQ)
    --codec_run_mode                    0: Surface origin      1: Buffer SharedMemory
                                        2: Surface AVBuffer    3: Buffer AVBuffer
    --data_producer                     0: Demuxer;  1: Bitstream Reader;  2: Rawdata Reader
    --bitstream_type                    0: AnnexB;   1: AVCC
    --codec_comsumer                    0: Default;  1: Decoder render output

    --frame_interval                    frame push interval (ms)
    --repeat_times                      demo repeat times
    --hdr_vivid_video                   input file is hdr vivid video? (0: false; 1: true)
    --need_dump_output                  need to dump output stream? (0: false; 1: true)
    --max_frames                        number of frames to be processed
    --seek_mode                         demuxer seek mode:
                                            0: SEEK_MODE_NEXT_SYNC
                                            1: SEEK_MODE_PREVIOUS_SYNC
                                            2: SEEK_MODE_CLOSEST_SYNC
    --thread_sleep_mode                 0: Input sleep;  1: Output sleep
    --encoder_surface_max_input_buffer  set for encoder surface max input buffer count

Example:
    --codec_type 0 --input input.h264 --mime video/avc --width 1280 --height 720 --framerate 30 --pixel_format 1
    --codec_run_mode 0 --frame_interval 0 --repeat_times 1 --hdr_vivid_video 0 --need_dump_output 0 --max_frames 100
)HELP_TEXT";

inline void ShowHelp(SampleInfo &info, const char * const value)
{
    (void)info;
    (void)value;
    std::cout << HELP_TEXT << std::endl;
    exit(0);
}

inline void SetCodecType(SampleInfo &info, const char * const value)
{
    info.codecType = static_cast<CodecType>(std::stol(value));
    if (info.codecType == 0b10) { // 0b10: Encoder
        info.dataProducerInfo.dataProducerType = DATA_PRODUCER_TYPE_RAW_DATA_READER;
    }
}

inline void SetInputFilePath(SampleInfo &info, const char * const value)
{
    info.inputFilePath = value;
}

inline void SetOutputFilePath(SampleInfo &info, const char * const value)
{
    info.outputFilePath = value;
}

inline void SetMime(SampleInfo &info, const char * const value)
{
    info.codecMime = value;
}

inline void SetWidth(SampleInfo &info, const char * const value)
{
    info.videoWidth = std::stol(value);
}

inline void SetHeight(SampleInfo &info, const char * const value)
{
    info.videoHeight = std::stol(value);
}

inline void SetFramerate(SampleInfo &info, const char * const value)
{
    info.frameRate = std::stof(value);
}

inline void SetPixelFormat(SampleInfo &info, const char * const value)
{
    info.pixelFormat = static_cast<OH_AVPixelFormat>(std::stol(value));
}

inline void SetBitrate(SampleInfo &info, const char * const value)
{
    info.bitrate = std::stoll(value);
}

inline void SetBitrateMode(SampleInfo &info, const char * const value)
{
    info.bitrateMode = std::stol(value);
}

inline void SetCodecRunMode(SampleInfo &info, const char * const value)
{
    info.codecRunMode = static_cast<CodecRunMode>(std::stol(value));
}

inline void SetFrameInterval(SampleInfo &info, const char * const value)
{
    info.frameInterval = std::stol(value);
}

inline void SetRepeatTimes(SampleInfo &info, const char * const value)
{
    info.repeatTimes = std::stoul(value);
}

inline void SetHdrVividVideo(SampleInfo &info, const char * const value)
{
    info.isHDRVivid = std::stol(value);
    if (info.isHDRVivid) {
        info.hevcProfile = HEVC_PROFILE_MAIN_10;
    }
}

inline void SetNeedDumpOutput(SampleInfo &info, const char * const value)
{
    info.needDumpOutput = std::stol(value);
}

inline void SetMaxFrames(SampleInfo &info, const char * const value)
{
    info.maxFrames = std::stoul(value);
}

inline void SetDataProducer(SampleInfo &info, const char * const value)
{
    info.dataProducerInfo.dataProducerType = static_cast<DataProducerType>(std::stol(value));
}

inline void SetBitstreamType(SampleInfo &info, const char * const value)
{
    info.dataProducerInfo.bitstreamType = static_cast<BitstreamType>(std::stol(value));
}

inline void SetSeekMode(SampleInfo &info, const char * const value)
{
    info.dataProducerInfo.seekMode = static_cast<OH_AVSeekMode>(std::stol(value));
}

inline void SetCodecComsumerType(SampleInfo &info, const char * const value)
{
    info.codecComsumerType = static_cast<CodecComsumerType>(std::stol(value));
}

inline void SetThreadSleepMode(SampleInfo &info, const char * const value)
{
    info.threadSleepMode = static_cast<ThreadSleepMode>(std::stol(value));
}

inline void SetEncoderSurfaceMaxInputBufferCount(SampleInfo &info, const char * const value)
{
    info.encoderSurfaceMaxInputBuffer = std::stol(value);
}

const std::unordered_map<DemoArgumentType, void (*)(SampleInfo &info, const char * const value)> ARG_OPT_MAP = {
    {DEMO_ARG_HELP,                             ShowHelp},
    {DEMO_ARG_CODEC_TYPE,                       SetCodecType},
    {DEMO_ARG_INPUT_FILE,                       SetInputFilePath},
    {DEMO_ARG_OUTPUT_FILE,                      SetOutputFilePath},
    {DEMO_ARG_CODEC_MIME,                       SetMime},
    {DEMO_ARG_WIDTH,                            SetWidth},
    {DEMO_ARG_HEIGHT,                           SetHeight},
    {DEMO_ARG_FRAMERATE,                        SetFramerate},
    {DEMO_ARG_PIXEL_FORMAT,                     SetPixelFormat},
    {DEMO_ARG_BITRATE,                          SetBitrate},
    {DEMO_ARG_BITRATE_MODE,                     SetBitrateMode},
    {DEMO_ARG_CODEC_RUN_MODE,                   SetCodecRunMode},
    {DEMO_ARG_FRAME_INTERVAL,                   SetFrameInterval},
    {DEMO_ARG_REPEAT_TIMES,                     SetRepeatTimes},
    {DEMO_ARG_HDR_VIVID_VIDEO,                  SetHdrVividVideo},
    {DEMO_ARG_NEED_DUMP_OUTPUT,                 SetNeedDumpOutput},
    {DEMO_ARG_MAX_FRAMES,                       SetMaxFrames},
    {DEMO_ARG_DATA_PRODUCER,                    SetDataProducer},
    {DEMO_ARG_BITSTREAM_TYPE,                   SetBitstreamType},
    {DEMO_ARG_SEEK_MODE,                        SetSeekMode},
    {DEMO_ARG_CODEC_COMSUMER,                   SetCodecComsumerType},
    {DEMO_ARG_THREAD_SLEEP_MODE,                SetThreadSleepMode},
    {DEMO_ARG_ENCODER_SURFACE_MAX_INPUT_BUFFER, SetEncoderSurfaceMaxInputBufferCount},
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
SampleInfo ParseDemoArg(int argc, char *argv[])
{
    SampleInfo info;
    DemoArgumentType argType = DEMO_ARG_UNKNOW;
    while ((argType = static_cast<DemoArgumentType>(getopt_long(argc, argv, "", DEMO_LONG_ARGUMENT, nullptr))) != -1) {
        if (argType <= DEMO_ARG_UNKNOW || argType >= DEMO_ARG_END) {
            std::cout << "Unknow arg type: " << argType << ", value: " << optarg << std::endl;
            continue;
        }
        ARG_OPT_MAP.at(argType)(info, optarg);
        std::cout << "Set arg successfully, type: " << DEMO_ARGUMENT_TYPE_TO_STRING.at(argType)
                  << ", value: " << optarg << std::endl;
    }
    return info;
}
} // Sample
} // MediaAVCodec
} // OHOS
