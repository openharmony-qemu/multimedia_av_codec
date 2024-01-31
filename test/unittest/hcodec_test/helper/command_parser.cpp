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
    OPT_API_TYPE = UINT8_MAX + 1,
    OPT_IS_ENCODER,
    OPT_IS_BUFFER_MODE,
    OPT_REPEAT_CNT,
    OPT_MAX_READ_CNT,
    OPT_PROTOCOL,
    OPT_PIXEL_FMT,
    OPT_FRAME_RATE,
    OPT_TIME_OUT,
    OPT_IS_HIGH_PERF_MODE,
    // encoder only
    OPT_MOCK_FRAME_CNT,
    OPT_COLOR_RANGE,
    OPT_COLOR_PRIMARY,
    OPT_COLOR_TRANSFER,
    OPT_COLOR_MATRIX,
    OPT_I_FRAME_INTERVAL,
    OPT_IDR_FRAME_NO,
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
    {"apiType",         required_argument,  nullptr, OPT_API_TYPE},
    {"isEncoder",       required_argument,  nullptr, OPT_IS_ENCODER},
    {"isBufferMode",    required_argument,  nullptr, OPT_IS_BUFFER_MODE},
    {"repeatCnt",       required_argument,  nullptr, OPT_REPEAT_CNT},
    {"maxReadFrameCnt", required_argument,  nullptr, OPT_MAX_READ_CNT},
    {"protocol",        required_argument,  nullptr, OPT_PROTOCOL},
    {"pixelFmt",        required_argument,  nullptr, OPT_PIXEL_FMT},
    {"frameRate",       required_argument,  nullptr, OPT_FRAME_RATE},
    {"timeout",         required_argument,  nullptr, OPT_TIME_OUT},
    {"isHighPerfMode",  required_argument,  nullptr, OPT_IS_HIGH_PERF_MODE},
    // encoder only
    {"mockFrameCnt",    required_argument,  nullptr, OPT_MOCK_FRAME_CNT},
    {"colorRange",      required_argument,  nullptr, OPT_COLOR_RANGE},
    {"colorPrimary",    required_argument,  nullptr, OPT_COLOR_PRIMARY},
    {"colorTransfer",   required_argument,  nullptr, OPT_COLOR_TRANSFER},
    {"colorMatrix",     required_argument,  nullptr, OPT_COLOR_MATRIX},
    {"iFrameInterval",  required_argument,  nullptr, OPT_I_FRAME_INTERVAL},
    {"idrFrameNo",      required_argument,  nullptr, OPT_IDR_FRAME_NO},
    {"profile",         required_argument,  nullptr, OPT_PROFILE},
    {"bitRateMode",     required_argument,  nullptr, OPT_BITRATE_MODE},
    {"bitRate",         required_argument,  nullptr, OPT_BITRATE},
    {"quality",         required_argument,  nullptr, OPT_QUALITY},
    // decoder only
    {"rotation",        required_argument,  nullptr, OPT_ROTATION},
    {"render",          required_argument,  nullptr, OPT_RENDER},
    {"decThenEnc",      required_argument,  nullptr, OPT_DEC_THEN_ENC},
    {"flushCnt",        required_argument,  nullptr, OPT_FLUSH_CNT},
    {nullptr,           no_argument,        nullptr, OPT_UNKONWN},
};

void ShowUsage()
{
    std::cout << "HCodec Test Options:" << std::endl;
    std::cout << " --help               help info." << std::endl;
    std::cout << " -i, --in             file name for input file." << std::endl;
    std::cout << " -w, --width          video width." << std::endl;
    std::cout << " -h, --height         video height." << std::endl;
    std::cout << " --apiType            0: codecbase, 1: new capi, 2: old capi." << std::endl;
    std::cout << " --isEncoder          1 is test encoder, 0 is test decoder" << std::endl;
    std::cout << " --isBufferMode       0 is surface mode, 1 is buffer mode." << std::endl;
    std::cout << " --repeatCnt          repeat test, default is 1" << std::endl;
    std::cout << " --maxReadFrameCnt    read up to frame count from input file" << std::endl;
    std::cout << " --protocol           video protocol. 0 is H264, 1 is H265" << std::endl;
    std::cout << " --pixelFmt           video pixel fmt. 1 is I420, 2 is NV12, 3 is NV21, 5 is RGBA" << std::endl;
    std::cout << " --frameRate          video frame rate." << std::endl;
    std::cout << " --timeout            thread timeout(ms). -1 means wait forever" << std::endl;
    std::cout << " --isHighPerfMode     0 is normal mode, 1 is high perf mode" << std::endl;
    // encoder only
    std::cout << " --mockFrameCnt       when read up to maxReadFrameCnt, just send old frames" << std::endl;
    std::cout << " --colorRange         color range. 1 is full range, 0 is limited range." << std::endl;
    std::cout << " --colorPrimary       color primary. see H.273 standard." << std::endl;
    std::cout << " --colorTransfer      color transfer characteristic. see H.273 standard." << std::endl;
    std::cout << " --colorMatrix        color matrix coefficient. see H.273 standard." << std::endl;
    std::cout << " --iFrameInterval     <0 means only one I frame, =0 means all intra" << std::endl;
    std::cout << "                      >0 means I frame interval in milliseconds" << std::endl;
    std::cout << " --idrFrameNo         which frame will be set to IDR frame." << std::endl;
    std::cout << " --profile            video profile, for 264: 0(baseline), 1(constrained baseline), " << std::endl;
    std::cout << "                      2(constrained high), 3(extended), 4(high), 8(main)" << std::endl;
    std::cout << "                      for 265: 0(main), 1(main 10)" << std::endl;
    std::cout << " --bitRateMode        bit rate mode for encoder. 0(CBR), 1(VBR), 2(CQ)" << std::endl;
    std::cout << " --bitRate            target encode bit rate (bps)" << std::endl;
    std::cout << " --quality            target encode quality" << std::endl;
    std::cout << " --render             0 means don't render, 1 means render to window" << std::endl;
    std::cout << " --decThenEnc         do surface encode after surface decode" << std::endl;
    std::cout << " --rotation           rotation angle after decode, eg. 0/90/180/270" << std::endl;
    std::cout << " --flushCnt           total flush count during decoding" << std::endl;
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
            case OPT_API_TYPE:
                opt.apiType = static_cast<ApiType>(stol(optarg));
                break;
            case OPT_IS_ENCODER:
                opt.isEncoder = stol(optarg);
                break;
            case OPT_IS_BUFFER_MODE:
                opt.isBufferMode = stol(optarg);
                break;
            case OPT_REPEAT_CNT:
                opt.repeatCnt = stol(optarg);
                break;
            case OPT_MAX_READ_CNT:
                opt.maxReadFrameCnt = stol(optarg);
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
            case OPT_IS_HIGH_PERF_MODE:
                opt.isHighPerfMode = stol(optarg);
                break;
            case OPT_MOCK_FRAME_CNT:
                opt.mockFrameCnt = stol(optarg);
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
            case OPT_IDR_FRAME_NO:
                opt.idrFrameNo = stol(optarg);
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
            default:
                break;
        }
    }
    return opt;
}

void CommandOpt::Print() const
{
    printf("-----------------------------\n");
    printf("api type=%d, %s, %s mode, render = %d\n", apiType,
        (isEncoder ? "encoder" : (decThenEnc ? "dec + enc" : "decoder")),
        isBufferMode ? "buffer" : "surface", render);
    printf("read inputFile %s up to %u frames\n", inputFile.c_str(), maxReadFrameCnt);
    printf("%u x %u @ %u fps\n", dispW, dispH, frameRate);
    printf("protocol = %s, pixFmt = %d\n", (protocol == H264) ? "264" : "265", pixFmt);
    printf("repeat %u times, timeout = %d\n", repeatCnt, timeout);
    printf("enableHighPerfMode : %s\n", isHighPerfMode ? "yes" : "no");

    if (mockFrameCnt.has_value()) {
        printf("mockFrameCnt %u\n", mockFrameCnt.value());
    }
    if (rangeFlag.has_value()) {
        printf("rangeFlag %d\n", rangeFlag.value());
    }
    if (primary.has_value()) {
        printf("primary %d\n", primary.value());
    }
    if (transfer.has_value()) {
        printf("transfer %d\n", transfer.value());
    }
    if (matrix.has_value()) {
        printf("matrix %d\n", matrix.value());
    }
    if (iFrameInterval.has_value()) {
        printf("iFrameInterval %d\n", iFrameInterval.value());
    }
    if (idrFrameNo.has_value()) {
        printf("idrFrameNo %u\n", idrFrameNo.value());
    }
    if (profile.has_value()) {
        printf("profile %d\n", profile.value());
    }
    if (rateMode.has_value()) {
        printf("rateMode %d\n", rateMode.value());
    }
    if (bitRate.has_value()) {
        printf("bitRate %u\n", bitRate.value());
    }
    if (quality.has_value()) {
        printf("quality %u\n", quality.value());
    }
    printf("rotation angle %u\n", rotation);
    printf("flush cnt %d\n", flushCnt);
    printf("-----------------------------\n");
}
}