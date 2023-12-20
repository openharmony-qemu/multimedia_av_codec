/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "command_parser.h"
#include <cinttypes>
#include <getopt.h>
#include <iostream>

namespace OHOS::MediaAVCodec {
using namespace std;

enum ShortOption {
    OPT_UNKONWN = 0,
    OPT_HELP,
    OPT_INPUT = 'i',
    OPT_WIDTH = 'w',
    OPT_HEIGHT = 'h',
    OPT_TEST_TYPE = UINT8_MAX + 1,
    OPT_IS_ENCODER,
    OPT_REPEAT_CNT,
    OPT_INPUT_FRAME_CNT,
    OPT_PROTOCOL,
    OPT_PIXEL_FMT,
    OPT_FRAME_RATE,
    OPT_TIME_OUT,
    OPT_IS_BUFFER_MODE,
    OPT_IS_HIGH_PERF_MODE,
    // encoder only
    OPT_COLOR_RANGE,
    OPT_COLOR_PRIMARY,
    OPT_COLOR_TRANSFER,
    OPT_COLOR_MATRIX,
    OPT_I_FRAME_INTERVAL,
    OPT_IDR_FRAME,
    OPT_PROFILE,
    OPT_BITRATE_MODE,
    OPT_BITRATE,
    OPT_QUALITY,
    // decoder only
    OPT_RENDER,
    OPT_DEC_THEN_ENC,
    OPT_ROTATION,
    OPT_FLUSH_CNT
};

static struct option g_longOptions[] = {
    {"help",            no_argument,        nullptr, OPT_HELP},
    {"in",              required_argument,  nullptr, OPT_INPUT},
    {"width",           required_argument,  nullptr, OPT_WIDTH},
    {"height",          required_argument,  nullptr, OPT_HEIGHT},
    {"testType",        required_argument,  nullptr, OPT_TEST_TYPE},
    {"isEncoder",       required_argument,  nullptr, OPT_IS_ENCODER},
    {"repeatCnt",       required_argument,  nullptr, OPT_REPEAT_CNT},
    {"inputCnt",        required_argument,  nullptr, OPT_INPUT_FRAME_CNT},
    {"protocol",        required_argument,  nullptr, OPT_PROTOCOL},
    {"pixelFmt",        required_argument,  nullptr, OPT_PIXEL_FMT},
    {"frameRate",       required_argument,  nullptr, OPT_FRAME_RATE},
    {"timeout",         required_argument,  nullptr, OPT_TIME_OUT},
    {"isBufferMode",    required_argument,  nullptr, OPT_IS_BUFFER_MODE},
    {"colorRange",      required_argument,  nullptr, OPT_COLOR_RANGE},
    {"colorPrimary",    required_argument,  nullptr, OPT_COLOR_PRIMARY},
    {"colorTransfer",   required_argument,  nullptr, OPT_COLOR_TRANSFER},
    {"colorMatrix",     required_argument,  nullptr, OPT_COLOR_MATRIX},
    {"iFrameInterval",  required_argument,  nullptr, OPT_I_FRAME_INTERVAL},
    {"IDRFrame",        required_argument,  nullptr, OPT_IDR_FRAME},
    {"profile",         required_argument,  nullptr, OPT_PROFILE},
    {"bitRateMode",     required_argument,  nullptr, OPT_BITRATE_MODE},
    {"bitRate",         required_argument,  nullptr, OPT_BITRATE},
    {"quality",         required_argument,  nullptr, OPT_QUALITY},
    {"rotation",        required_argument,  nullptr, OPT_ROTATION},
    {"render",          required_argument,  nullptr, OPT_RENDER},
    {"decThenEnc",      required_argument,  nullptr, OPT_DEC_THEN_ENC},
    {"flushCnt",        required_argument,  nullptr, OPT_FLUSH_CNT},
    {"isHighPerfMode",  required_argument,  nullptr, OPT_IS_HIGH_PERF_MODE},
    {nullptr,           no_argument,        nullptr, OPT_UNKONWN},
};

void ShowUsage()
{
    std::cout << "HCodec Test Options:" << std::endl;
    std::cout << " --help               help info." << std::endl;
    std::cout << " -i, --in             file name for input file." << std::endl;
    std::cout << " -w, --width          video width." << std::endl;
    std::cout << " -h, --height         video height." << std::endl;
    std::string testTypeDesc = "0 is test codecbase api, " \
                               "1 is test c api using sharedmem, " \
                               "2 is test c api using avbuffer";
    std::cout << " --testType           " << testTypeDesc << std::endl;
    std::cout << " --isEncoder          1 is test encoder, 0 is test decoder" << std::endl;
    std::cout << " --repeatCnt          repeat test, default is 1" << std::endl;
    std::cout << " --inputCnt           input frame count to encode/decode" << std::endl;
    std::cout << " --protocol           video protocol. 0 is H264, 1 is H265" << std::endl;
    std::cout << " --pixelFmt           video pixel fmt. 1 is I420, 2 is NV12, 3 is NV21, 5 is RGBA" << std::endl;
    std::cout << " --frameRate          video frame rate." << std::endl;
    std::cout << " --timeout            thread timeout(ms). -1 means wait forever" << std::endl;
    std::cout << " --isBufferMode       0 is surface mode, 1 is buffer mode." << std::endl;
    std::cout << " --colorRange         color range. 1 is full range, 0 is limited range." << std::endl;
    std::cout << " --colorPrimary       color primary. see H.273 standard." << std::endl;
    std::cout << " --colorTransfer      color transfer characteristic. see H.273 standard." << std::endl;
    std::cout << " --colorMatrix        color matrix coefficient. see H.273 standard." << std::endl;
    std::cout << " --iFrameInterval     <0 means only one I frame, =0 means all intra" << std::endl;
    std::cout << "                      >0 means I frame interval in milliseconds" << std::endl;
    std::cout << " --IDRFrame           >0 means that the frame is set to an IDR frame." << std::endl;
    std::cout << " --profile            video profile, for 264: 0(baseline), 1(constrained baseline), " << std::endl;
    std::cout << "                      2(constrained high), 3(extended), 4(high), 8(main)" << std::endl;
    std::cout << "                      for 265: 0(main)" << std::endl;
    std::cout << " --bitRateMode        bit rate mode for encoder. 0(CBR), 1(VBR), 2(CQ)" << std::endl;
    std::cout << " --bitRate            target encode bit rate (bps)" << std::endl;
    std::cout << " --quality            target encode quality" << std::endl;
    std::cout << " --render             0 means don't render, 1 means render to window" << std::endl;
    std::cout << " --decThenEnc         do surface encode after surface decode" << std::endl;
    std::cout << " --rotation           rotation angle after decode, eg. 0/90/180/270" << std::endl;
    std::cout << " --flushCnt           total flush count during decoding" << std::endl;
    std::cout << " --isHighPerfMode     0 is normal mode, 1 is high perf mode" << std::endl;
}

CommandOpt Parse(int argc, char *argv[])
{
    CommandOpt opt;
    int c;
    while ((c = getopt_long(argc, argv, "i:w:h:", g_longOptions, nullptr)) != -1) {
        switch (c) {
            case OPT_HELP:
                ShowUsage();
                break;
            case OPT_INPUT:
                opt.inputFile = string(optarg);
                break;
            case OPT_WIDTH:
                opt.dispW = stol(optarg);
                break;
            case OPT_HEIGHT:
                opt.dispH = stol(optarg);
                break;
            case OPT_TEST_TYPE:
                opt.testType = static_cast<DemoType>(stol(optarg));
                break;
            case OPT_IS_ENCODER:
                opt.isEncoder = stol(optarg);
                break;
            case OPT_REPEAT_CNT:
                opt.repeatCnt = stol(optarg);
                break;
            case OPT_INPUT_FRAME_CNT:
                opt.inputCnt = stol(optarg);
                break;
            case OPT_PROTOCOL:
                opt.protocol = static_cast<CodeType>(stol(optarg));
                break;
            case OPT_PIXEL_FMT:
                opt.pixFmt = static_cast<VideoPixelFormat>(stol(optarg));
                break;
            case OPT_FRAME_RATE:
                opt.frameRate = stol(optarg);
                break;
            case OPT_TIME_OUT:
                opt.timeout = stol(optarg);
                break;
            case OPT_IS_BUFFER_MODE:
                opt.isBufferMode = stol(optarg);
                break;
            case OPT_COLOR_RANGE:
                opt.rangeFlag = stol(optarg);
                break;
            case OPT_COLOR_PRIMARY:
                opt.primary = static_cast<ColorPrimary>(stol(optarg));
                break;
            case OPT_COLOR_TRANSFER:
                opt.transfer = static_cast<TransferCharacteristic>(stol(optarg));
                break;
            case OPT_COLOR_MATRIX:
                opt.matrix = static_cast<MatrixCoefficient>(stol(optarg));
                break;
            case OPT_I_FRAME_INTERVAL:
                opt.iFrameInterval = stol(optarg);
                break;
            case OPT_IDR_FRAME:
                opt.numIdrFrame = stol(optarg);
                break;
            case OPT_PROFILE:
                opt.profile = stol(optarg);
                break;
            case OPT_BITRATE_MODE:
                opt.rateMode = static_cast<VideoEncodeBitrateMode>(stol(optarg));
                break;
            case OPT_BITRATE:
                opt.bitRate = stol(optarg);
                break;
            case OPT_QUALITY:
                opt.quality = stol(optarg);
                break;
            case OPT_RENDER:
                opt.render = stol(optarg);
                break;
            case OPT_DEC_THEN_ENC:
                opt.decThenEnc = stol(optarg);
                break;
            case OPT_ROTATION:
                opt.rotation = static_cast<VideoRotation>(stol(optarg));
                break;
            case OPT_FLUSH_CNT:
                opt.flushCnt = stol(optarg);
                break;
            case OPT_IS_HIGH_PERF_MODE:
                opt.isHighPerfMode = stol(optarg);
                break;
            default:
                break;
        }
    }
    return opt;
}

void CommandOpt::Print() const
{
    std::string testTypeDesc;
    if (testType == DemoType::TEST_CODEC_BASE) {
        testTypeDesc = "codecbase api";
    } else if (testType == DemoType::TEST_C_API_USING_SHARED_MEM) {
        testTypeDesc = "c api using shared mem";
    } else {
        testTypeDesc = "c api using avbuffer";
    }
    printf("-----------------------------\n");
    printf("test %s, %s, repeat %u times\n", testTypeDesc.c_str(),
           isEncoder ? "encoder" : (decThenEnc ? "dec + enc" : "decoder"),
           repeatCnt);
    printf("read inputFile %s up to %u frames\n", inputFile.c_str(), inputCnt);
    printf("%u x %u @ %u fps\n", dispW, dispH, frameRate);
    printf("protocol = %s, pixFmt = %d\n", (protocol == H264) ? "264" : "265", pixFmt);
    printf("%s mode, render = %d\n", isBufferMode ? "buffer" : "surface", render);
    printf("timeout = %d\n", timeout);
    printf("range %d, primary %d, transfer %d, matrix %d\n", rangeFlag, primary, transfer, matrix);
    printf("I frame interval %d\n", iFrameInterval);
    printf("profile %d\n", profile);
    printf("bit rate mode %d, bit rate %" PRId64 ", quality %u\n", rateMode, bitRate, quality);
    printf("rotation angle %u\n", rotation);
    printf("flush cnt %d\n", flushCnt);
    printf("Set NO.%u frame as the IDR Frame\n", numIdrFrame);
    printf("enableHighPerfMode : %s\n", isHighPerfMode ? "yes" : "no");
    printf("-----------------------------\n");
}
}