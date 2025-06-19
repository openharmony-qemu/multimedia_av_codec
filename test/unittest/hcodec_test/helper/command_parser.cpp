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
    OPT_REPEAT_CNT,
    OPT_MAX_READ_CNT,
    OPT_PROTOCOL,
    OPT_PIXEL_FMT,
    OPT_FRAME_RATE,
    OPT_TIME_OUT,
    OPT_IS_HIGH_PERF_MODE,
    OPT_SET_PARAMETER,
    OPT_SET_PER_FRAME,
    OPT_SET_RESOURCE,
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
    OPT_LTR_FRAME_COUNT,
    OPT_REPEAT_AFTER,
    OPT_REPEAT_MAX_CNT,
    OPT_LAYER_COUNT,
    OPT_WATERMARK,
    OPT_ENABLE_PARAMS_FEEDBACK,
    OPT_SQR_FACTOR,
    OPT_MAX_BITRATE,
    OPT_ENABLE_QP_MAP,
    OPT_IS_ABS_QP_MAP,
    OPT_QP_MAP_VALUE,
    OPT_TARGET_QP,
    OPT_GOP_B_MODE,
    // decoder only
    OPT_DEC_THEN_ENC,
    OPT_ROTATION,
    OPT_FLUSH_CNT,
    OPT_SCALE_MODE,
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
    {"setParameter",    required_argument,  nullptr, OPT_SET_PARAMETER},
    {"setPerFrame",     required_argument,  nullptr, OPT_SET_PER_FRAME},
    {"setResource",     required_argument,  nullptr, OPT_SET_RESOURCE},
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
    {"sqrFactor",       required_argument,  nullptr, OPT_SQR_FACTOR},
    {"maxBitrate",      required_argument,  nullptr, OPT_MAX_BITRATE},
    {"targetQp",        required_argument,  nullptr, OPT_TARGET_QP},
    {"qpRange",         required_argument,  nullptr, OPT_QP_RANGE},
    {"ltrFrameCount",   required_argument,  nullptr, OPT_LTR_FRAME_COUNT},
    {"repeatAfter",     required_argument,  nullptr, OPT_REPEAT_AFTER},
    {"repeatMaxCnt",    required_argument,  nullptr, OPT_REPEAT_MAX_CNT},
    {"layerCnt",        required_argument,  nullptr, OPT_LAYER_COUNT},
    {"waterMark",       required_argument,  nullptr, OPT_WATERMARK},
    {"paramsFeedback",  required_argument,  nullptr, OPT_ENABLE_PARAMS_FEEDBACK},
    {"enableQPMap",     required_argument,  nullptr, OPT_ENABLE_QP_MAP},
    {"isAbsQpMap",       required_argument,  nullptr, OPT_IS_ABS_QP_MAP},
    {"qpMapValue",      required_argument,  nullptr, OPT_QP_MAP_VALUE},
    {"gopBMode",        required_argument,  nullptr, OPT_GOP_B_MODE},
    // decoder only
    {"rotation",        required_argument,  nullptr, OPT_ROTATION},
    {"decThenEnc",      required_argument,  nullptr, OPT_DEC_THEN_ENC},
    {"flushCnt",        required_argument,  nullptr, OPT_FLUSH_CNT},
    {"scaleMode",       required_argument,  nullptr, OPT_SCALE_MODE},
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
    std::cout << " --setParameter       eg. 11:frameRate,60 or 24:requestIdr,1" << std::endl;
    std::cout << " --setPerFrame        eg. 11:ltr,1,0,30 or 24:qp,3,40 or 25:discard,1 or 30:ebr,16,30,25,0 or 30:roiParams,Top1,Left1-Bottom1,Right1=Offset1;Top2,Left2-Bottom2,Right2=Offset2;..."
              << std::endl;
    std::cout << " --setResource        eg. 11:/data/test/a.yuv,1280,720,2" << std::endl;
    std::cout << " [encoder only]" << std::endl;
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
    std::cout << " --bitRateMode        bit rate mode for encoder. 0(CBR), 1(VBR), 2(CQ), 3(SQR), 11(CRF)"
              << ", 10(CBR_VIDEOCALL)" << std::endl;
    std::cout << " --bitRate            target encode bit rate (bps)" << std::endl;
    std::cout << " --quality            target encode quality" << std::endl;
    std::cout << " --sqrFactor           target encode QP" << std::endl;
    std::cout << " --maxBitrate         sqr mode max bitrate" << std::endl;
    std::cout << " --qpRange            target encode qpRange, eg. 13,42" << std::endl;
    std::cout << " --ltrFrameCount      The number of long-term reference frames." << std::endl;
    std::cout << " --repeatAfter        repeat previous frame after target ms" << std::endl;
    std::cout << " --repeatMaxCnt       repeat previous frame up to target times" << std::endl;
    std::cout << " --layerCnt           target encode layerCnt, H264:2, H265:2 and 3" << std::endl;
    std::cout << " --waterMark          eg. /data/test/a.rgba,1280,720,2:16,16,1280,720" << std::endl;
    std::cout << " --gopBMode           gop mode for b frame. 1(adaptive-b mode), 2(h3b mode)" << std::endl;
    std::cout << " [decoder only]" << std::endl;
    std::cout << " --rotation           rotation angle after decode, eg. 0/90/180/270" << std::endl;
    std::cout << " --paramsFeedback     0 means don't feedback, 1 means feedback" << std::endl;
    std::cout << " --decThenEnc         do surface encode after surface decode" << std::endl;
    std::cout << " --flushCnt           total flush count during decoding" << std::endl;
    std::cout << " --scaleMode          target scale mode after decode, see @OH_ScalingMode" << std::endl;
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
            case OPT_SET_PARAMETER:
                opt.ParseParamFromCmdLine(SET_PARAM, optarg);
                break;
            case OPT_SET_PER_FRAME:
                opt.ParseParamFromCmdLine(PER_FRAME_PARAM, optarg);
                break;
            case OPT_SET_RESOURCE:
                opt.ParseParamFromCmdLine(RESOURCE_PARAM, optarg);
                break;
            // encoder only
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
            case OPT_SQR_FACTOR:
                opt.sqrFactor = stol(optarg);
                break;
            case OPT_MAX_BITRATE:
                opt.maxBitrate = stol(optarg);
                break;
            case OPT_TARGET_QP:
                opt.targetQp = stol(optarg);
                break;
            case OPT_QP_RANGE: {
                istringstream is(optarg);
                QPRange range;
                char tmp;
                is >> range.qpMin >> tmp >> range.qpMax;
                opt.qpRange = range;
                break;
            }
            case OPT_LTR_FRAME_COUNT:
                opt.ltrFrameCount = stol(optarg);
                break;
            case OPT_REPEAT_AFTER:
                opt.repeatAfter = stol(optarg);
                break;
            case OPT_REPEAT_MAX_CNT:
                opt.repeatMaxCnt = stol(optarg);
                break;
            case OPT_LAYER_COUNT:
                opt.layerCnt = stol(optarg);
                break;
            case OPT_WATERMARK:
                opt.ParseWaterMark(optarg);
                break;
            case OPT_ENABLE_PARAMS_FEEDBACK:
                opt.paramsFeedback = stol(optarg);
                break;
            case OPT_ENABLE_QP_MAP:
                opt.enableQPMap = stol(optarg);
                break;
            case OPT_GOP_B_MODE:
                opt.gopBMode = stol(optarg);
                break;
            // decoder only
            case OPT_DEC_THEN_ENC:
                opt.decThenEnc = stol(optarg);
                break;
            case OPT_ROTATION:
                opt.rotation = static_cast<VideoRotation>(stol(optarg));
                break;
            case OPT_FLUSH_CNT:
                opt.flushCnt = stol(optarg);
                break;
            case OPT_SCALE_MODE:
                opt.scaleMode = static_cast<OH_ScalingMode>(stol(optarg));
                break;
            default:
                break;
        }
    }
    return opt;
}

void CommandOpt::ParseParamFromCmdLine(ParamType paramType, const char *cmd)
{
    string s(cmd);
    auto pos = s.find(':');
    if (pos == string::npos) {
        return;
    }
    auto frameNo = stoul(s.substr(0, pos));
    string paramList = s.substr(pos + 1);
    switch (paramType) {
        case SET_PARAM: {
            ParseSetParameter(frameNo, paramList);
            break;
        }
        case PER_FRAME_PARAM: {
            ParsePerFrameParam(frameNo, paramList);
            break;
        }
        case RESOURCE_PARAM: {
            ResourceParams dst;
            ParseResourceParam(paramList, dst);
            resourceParamsMap[frameNo] = dst;
            break;
        }
        default: {
            break;
        }
    }
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
    if (key == "targetQp") {
        uint32_t targetQp;
        value >> targetQp;
        setParameterParamsMap[frameNo].targetQp = targetQp;
    }
    if (key == "sqr") { // sqr,bitrate(optional),maxBitrate(optional),sqrFactor(optional)
        SQRParam sqrParam;
        std::string token;
        std::vector<std::optional<int>> sqrUserSet;
        while (std::getline(value, token, ',')) {
            sqrUserSet.push_back(token.empty() ? std::nullopt : std::optional<int32_t>(std::stoi(token)));
        }
        if (sqrUserSet.size() > 0) sqrParam.bitrate = sqrUserSet[0];
        if (sqrUserSet.size() > 1) sqrParam.maxBitrate = sqrUserSet[1];
        if (sqrUserSet.size() > 2) sqrParam.sqrFactor = sqrUserSet[2]; // sqrFactor index:2
        setParameterParamsMap[frameNo].sqrParam = sqrParam;
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
    if (key == "discard") {
        bool discard;
        value >> discard;
        perFrameParamsMap[frameNo].discard = discard;
    }
    if (key == "ebr") {
        EBRParam ebrParam;
        value >> ebrParam.minQp >> c >> ebrParam.maxQp >> c >> ebrParam.startQp >> c >> ebrParam.isSkip;
        perFrameParamsMap[frameNo].ebrParam = ebrParam;
    }
    if (key == "roiParams") {
        string roiParams;
        value >> roiParams;
        perFrameParamsMap[frameNo].roiParams = roiParams;
    }
    if (key == "isAbsQpMap") {
        bool absQp;
        value >> absQp;
        perFrameParamsMap[frameNo].absQpMap = absQp;
    }
    if (key == "qpMapValue") {
        int32_t qpMapValue;
        value >> qpMapValue;
        perFrameParamsMap[frameNo].qpMapValue = qpMapValue;
        if (!perFrameParamsMap[frameNo].absQpMap.has_value()) {
            perFrameParamsMap[frameNo].absQpMap = false;
        }
    }
}

void CommandOpt::ParseResourceParam(const std::string &src, ResourceParams& dst)
{
    auto pos = src.find(',');
    if (pos == string::npos) {
        return;
    }
    dst.inputFile = src.substr(0, pos);
    istringstream value(src.substr(pos + 1));
    int pixelFmt;
    char c;
    value >> dst.dispW >> c >> dst.dispH >> c >> pixelFmt;
    dst.pixFmt = static_cast<VideoPixelFormat>(pixelFmt);
}

void CommandOpt::ParseWaterMark(const char *cmd) // "/data/test/a.rgba,1280,720,2:16,16,1280,720"
{
    string s(cmd);
    auto pos = s.find(':');
    if (pos == string::npos) {
        return;
    }
    waterMark.isSet = true;
    string resource = s.substr(0, pos);           // "/data/test/a.rgba,1280,720,2"
    ParseResourceParam(resource, waterMark.waterMarkFile);
    istringstream coordinate(s.substr(pos + 1));  // "16,16,1280,720"
    char c;
    coordinate >> waterMark.dstX >> c >> waterMark.dstY >> c >> waterMark.dstW >> c >> waterMark.dstH;
}

void CommandOpt::Print() const
{
    TLOGI("-----------------------------");
    TLOGI("api type=%d, %s, %s mode", apiType,
        (isEncoder ? "encoder" : (decThenEnc ? "dec + enc" : "decoder")),
        isBufferMode ? "buffer" : "surface");
    TLOGI("read inputFile %s up to %u frames", inputFile.c_str(), maxReadFrameCnt);
    TLOGI("%u x %u @ %u fps", dispW, dispH, frameRate);
    TLOGI("protocol = %d, pixFmt = %d", protocol, pixFmt);
    TLOGI("repeat %u times, timeout = %d", repeatCnt, timeout);
    TLOGI("enableHighPerfMode : %s", isHighPerfMode ? "yes" : "no");

    if (gopBMode.has_value()) {
        TLOGI("gopBMode : %d", gopBMode.value());
    }
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
    if (sqrFactor.has_value()) {
        TLOGI("sqrFactor %u", sqrFactor.value());
    }
    if (maxBitrate.has_value()) {
        TLOGI("maxBitrate %u", maxBitrate.value());
    }
    if (targetQp.has_value()) {
        TLOGI("targetQp %u", targetQp.value());
    }
    if (qpRange.has_value()) {
        TLOGI("qpRange %u~%u", qpRange->qpMin, qpRange->qpMax);
    }
    if (roiParams.has_value()) {
        TLOGI("roiParams: %s",roiParams->c_str());
    }
    if (waterMark.isSet) {
        TLOGI("dstX %d, dstY %d, dstW %d, dstH %d",
              waterMark.dstX, waterMark.dstY, waterMark.dstW, waterMark.dstH);
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
        if (setparam.frameRate.has_value()) {
            TLOGI("    frameRate %f", setparam.frameRate.value());
        }
        if (setparam.sqrParam.has_value()) {
            TLOGI("    sqrParam: %s %s %s",
                setparam.sqrParam->bitrate.has_value() ?
                    ("bitrate="+std::to_string(setparam.sqrParam->bitrate.value())).c_str() : "",
                setparam.sqrParam->maxBitrate.has_value() ?
                    ("maxBitrate="+std::to_string(setparam.sqrParam->maxBitrate.value())).c_str() : "",
                setparam.sqrParam->sqrFactor.has_value() ?
                    ("sqrFactor="+std::to_string(setparam.sqrParam->sqrFactor.value())).c_str() : ""
            );
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
        if (perFrame.absQpMap.has_value() && perFrame.qpMapValue.has_value()) {
            TLOGI("    qpMap, type %d (0: delta qp, 1: abs qp), value %d",
                  perFrame.absQpMap.value(), perFrame.qpMapValue.value());
        }
        if (perFrame.roiParams.has_value()) {
            TLOGI("roiParams:  %s", perFrame.roiParams.value().c_str());
        }
    }
    for (const auto &[frameNo, resourceParam] : resourceParamsMap) {
        TLOGI("frameNo = %u, filePath = %s, %u x %u, pixFmt = %d", frameNo,
              resourceParam.inputFile.c_str(), resourceParam.dispW, resourceParam.dispH, resourceParam.pixFmt);
    }
    TLOGI("-----------------------------");
}
}