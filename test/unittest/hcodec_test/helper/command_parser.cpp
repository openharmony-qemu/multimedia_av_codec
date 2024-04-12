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
#include <sstream>
#include "hcodec_log.h"

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
    OPT_LTR_FRAME_COUNT,
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
    OPT_PROFILE,
    OPT_BITRATE_MODE,
    OPT_BITRATE,
    OPT_QUALITY,
    OPT_QP_RANGE,
    // decoder only
    OPT_RENDER,
    OPT_DEC_THEN_ENC,
    OPT_ROTATION,
    OPT_FLUSH_CNT,
    OPT_SET_PARAMETER,
    OPT_SET_PER_FRAME
};

static struct option g_longOptions[] = {
    {"help",            no_argument,        nullptr, OPT_HELP},
    {"in",              required_argument,  nullptr, OPT_INPUT},
    {"width",           required_argument,  nullptr, OPT_WIDTH},
    {"height",          required_argument,  nullptr, OPT_HEIGHT},
    {"apiType",         required_argument,  nullptr, OPT_API_TYPE},
    {"isEncoder",       required_argument,  nullptr, OPT_IS_ENCODER},
    {"isBufferMode",    required_argument,  nullptr, OPT_IS_BUFFER_MODE},
    {"ltrFrameCount",   required_argument,  nullptr, OPT_LTR_FRAME_COUNT},
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
    {"profile",         required_argument,  nullptr, OPT_PROFILE},
    {"bitRateMode",     required_argument,  nullptr, OPT_BITRATE_MODE},
    {"bitRate",         required_argument,  nullptr, OPT_BITRATE},
    {"quality",         required_argument,  nullptr, OPT_QUALITY},
    {"qpRange",         required_argument,  nullptr, OPT_QP_RANGE},
    // decoder only
    {"rotation",        required_argument,  nullptr, OPT_ROTATION},
    {"render",          required_argument,  nullptr, OPT_RENDER},
    {"decThenEnc",      required_argument,  nullptr, OPT_DEC_THEN_ENC},
    {"flushCnt",        required_argument,  nullptr, OPT_FLUSH_CNT},
    {"setParameter",    required_argument,  nullptr, OPT_SET_PARAMETER},
    {"setPerFrame",     required_argument,  nullptr, OPT_SET_PER_FRAME},
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
    std::cout << " --ltrFrameCount      The number of long-term reference frames." << std::endl;
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
    std::cout << " --profile            video profile, for 264: 0(baseline), 1(constrained baseline), " << std::endl;
    std::cout << "                      2(constrained high), 3(extended), 4(high), 8(main)" << std::endl;
    std::cout << "                      for 265: 0(main), 1(main 10)" << std::endl;
    std::cout << " --bitRateMode        bit rate mode for encoder. 0(CBR), 1(VBR), 2(CQ)" << std::endl;
    std::cout << " --bitRate            target encode bit rate (bps)" << std::endl;
    std::cout << " --quality            target encode quality" << std::endl;
    std::cout << " --qpRange            target encode qpRange, eg. 13,42" << std::endl;
    std::cout << " --render             0 means don't render, 1 means render to window" << std::endl;
    std::cout << " --decThenEnc         do surface encode after surface decode" << std::endl;
    std::cout << " --rotation           rotation angle after decode, eg. 0/90/180/270" << std::endl;
    std::cout << " --flushCnt           total flush count during decoding" << std::endl;
    std::cout << " --setParameter       eg. 11:frameRate,60 or 24:requestIdr,1" << std::endl;
    std::cout << " --setPerFrame        eg. 11:ltr,1,0,30 or 24:qp,3,40" << std::endl;
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
            case OPT_LTR_FRAME_COUNT:
                opt.ltrFrameCount = stol(optarg);
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
            case OPT_QP_RANGE: {
                istringstream is(optarg);
                QPRange range;
                char tmp;
                is >> range.qpMin >> tmp >> range.qpMax;
                opt.qpRange = range;
                break;
            }
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
            case OPT_SET_PARAMETER:
                opt.ParseParamFromCmdLine(false, optarg);
                break;
            case OPT_SET_PER_FRAME:
                opt.ParseParamFromCmdLine(true, optarg);
                break;
            default:
                break;
        }
    }
    return opt;
}

void CommandOpt::ParseParamFromCmdLine(bool isPerFrame, const char *cmd)
{
    string s(cmd);
    auto pos = s.find(':');
    if (pos == string::npos) {
        return;
    }
    auto frameNo = stoul(s.substr(0, pos));
    string paramList = s.substr(pos + 1);
    isPerFrame ? ParsePerFrameParam(frameNo, paramList) : ParseSetParameter(frameNo, paramList);
}

void CommandOpt::ParseSetParameter(uint32_t frameNo, const string &s)
{
    auto pos = s.find(',');
    if (pos == string::npos) {
        return;
    }
    char c;
    string key = s.substr(0, pos);
    istringstream value(s.substr(pos + 1));
    if (key == "requestIdr") {
        bool requestIdr;
        value >> requestIdr;
        setParameterParamsMap[frameNo].requestIdr = requestIdr;
    }
    if (key == "bitRate") {
        uint32_t bitRate;
        value >> bitRate;
        setParameterParamsMap[frameNo].bitRate = bitRate;
    }
    if (key == "frameRate") {
        double fr;
        value >> fr;
        setParameterParamsMap[frameNo].frameRate = fr;
    }
    if (key == "qpRange") {
        QPRange qpRange;
        value >> qpRange.qpMin >> c >> qpRange.qpMax;
        setParameterParamsMap[frameNo].qpRange = qpRange;
    }
}

void CommandOpt::ParsePerFrameParam(uint32_t frameNo, const string &s)
{
    auto pos = s.find(',');
    if (pos == string::npos) {
        return;
    }
    string key = s.substr(0, pos);
    istringstream value(s.substr(pos + 1));
    char c;
    if (key == "requestIdr") {
        bool requestIdr;
        value >> requestIdr;
        perFrameParamsMap[frameNo].requestIdr = requestIdr;
    }
    if (key == "qpRange") {
        QPRange qpRange;
        value >> qpRange.qpMin >> c >> qpRange.qpMax;
        perFrameParamsMap[frameNo].qpRange = qpRange;
    }
    if (key == "ltr") {
        LTRParam ltr;
        value >> ltr.markAsLTR >> c >> ltr.useLTR >> c >> ltr.useLTRPoc;
        perFrameParamsMap[frameNo].ltrParam = ltr;
    }
}

void CommandOpt::Print() const
{
    TLOGI("-----------------------------");
    TLOGI("api type=%d, %s, %s mode, render = %d", apiType,
        (isEncoder ? "encoder" : (decThenEnc ? "dec + enc" : "decoder")),
        isBufferMode ? "buffer" : "surface", render);
    TLOGI("read inputFile %s up to %u frames", inputFile.c_str(), maxReadFrameCnt);
    TLOGI("%u x %u @ %u fps", dispW, dispH, frameRate);
    TLOGI("protocol = %s, pixFmt = %d", (protocol == H264) ? "264" : "265", pixFmt);
    TLOGI("repeat %u times, timeout = %d", repeatCnt, timeout);
    TLOGI("enableHighPerfMode : %s", isHighPerfMode ? "yes" : "no");

    if (mockFrameCnt.has_value()) {
        TLOGI("mockFrameCnt %u", mockFrameCnt.value());
    }
    if (rangeFlag.has_value()) {
        TLOGI("rangeFlag %d", rangeFlag.value());
    }
    if (primary.has_value()) {
        TLOGI("primary %d", primary.value());
    }
    if (transfer.has_value()) {
        TLOGI("transfer %d", transfer.value());
    }
    if (matrix.has_value()) {
        TLOGI("matrix %d", matrix.value());
    }
    if (iFrameInterval.has_value()) {
        TLOGI("iFrameInterval %d", iFrameInterval.value());
    }
    if (profile.has_value()) {
        TLOGI("profile %d", profile.value());
    }
    if (rateMode.has_value()) {
        TLOGI("rateMode %d", rateMode.value());
    }
    if (bitRate.has_value()) {
        TLOGI("bitRate %u", bitRate.value());
    }
    if (quality.has_value()) {
        TLOGI("quality %u", quality.value());
    }
    if (qpRange.has_value()) {
        TLOGI("qpRange %u~%u", qpRange->qpMin, qpRange->qpMax);
    }
    TLOGI("rotation angle %u", rotation);
    TLOGI("flush cnt %d", flushCnt);
    for (const auto &[frameNo, setparam] : setParameterParamsMap) {
        TLOGI("frameNo = %u:", frameNo);
        if (setparam.requestIdr.has_value()) {
            TLOGI("    requestIdr %d", setparam.requestIdr.value());
        }
        if (setparam.qpRange.has_value()) {
            TLOGI("    qpRange %u~%u", setparam.qpRange->qpMin, setparam.qpRange->qpMax);
        }
        if (setparam.bitRate.has_value()) {
            TLOGI("    bitRate %u", setparam.bitRate.value());
        }
        if (setparam.frameRate.has_value()) {
            TLOGI("    frameRate %f", setparam.frameRate.value());
        }
    }
    for (const auto &[frameNo, perFrame] : perFrameParamsMap) {
        TLOGI("frameNo = %u:", frameNo);
        if (perFrame.requestIdr.has_value()) {
            TLOGI("    requestIdr %d", perFrame.requestIdr.value());
        }
        if (perFrame.qpRange.has_value()) {
            TLOGI("    qpRange %u~%u", perFrame.qpRange->qpMin, perFrame.qpRange->qpMax);
        }
        if (perFrame.ltrParam.has_value()) {
            TLOGI("    LTR, markAsLTR %d, useLTR %d, useLTRPoc %u",
                  perFrame.ltrParam->markAsLTR, perFrame.ltrParam->useLTR, perFrame.ltrParam->useLTRPoc);
        }
    }
    TLOGI("-----------------------------");
}
}