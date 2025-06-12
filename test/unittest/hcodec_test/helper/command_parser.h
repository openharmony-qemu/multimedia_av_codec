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
 *
 * Description: header of HCode System test command parse
 */

#ifndef HCODEC_TEST_COMMAND_PARSE_H
#define HCODEC_TEST_COMMAND_PARSE_H

#include <string>
#include <optional>
#include "native_avcodec_base.h"
#include "av_common.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "start_code_detector.h"

namespace OHOS::MediaAVCodec {

enum class ApiType {
    TEST_CODEC_BASE,
    TEST_C_API_NEW,
    TEST_C_API_OLD,
};

struct QPRange {
    uint32_t qpMin;
    uint32_t qpMax;
};

struct LTRParam {
    bool markAsLTR;
    bool useLTR;
    uint32_t useLTRPoc;
};

struct EBRParam {
    int32_t minQp;
    int32_t maxQp;
    int32_t startQp;
    int32_t isSkip;
};

struct SQRParam {
    std::optional<int32_t> bitrate;
    std::optional<int32_t> maxBitrate;
    std::optional<int32_t> sqrFactor;
};

enum ParamType {
    SET_PARAM,
    PER_FRAME_PARAM,
    RESOURCE_PARAM,
};

struct SetParameterParams {
    std::optional<bool> requestIdr;
    std::optional<uint32_t> bitRate;
    std::optional<double> frameRate;
    std::optional<QPRange> qpRange;
    std::optional<VideoRotation> rotate;
    std::optional<OH_ScalingMode> scaleMode;
    std::optional<uint32_t> targetQp;
    std::optional<SQRParam> sqrParam;
};

struct PerFrameParams {
    std::optional<bool> requestIdr;
    std::optional<QPRange> qpRange;
    std::optional<LTRParam> ltrParam;
    std::optional<bool> discard;
    std::optional<EBRParam> ebrParam;
    std::optional<std::string> roiParams;
    std::optional<bool> absQpMap;
    std::optional<int32_t> qpMapValue;
};

struct ResourceParams {
    std::string inputFile;
    uint32_t dispW = 0;
    uint32_t dispH = 0;
    VideoPixelFormat pixFmt = VideoPixelFormat::NV12;
};

struct WaterMarkParam {
    bool isSet = false;
    ResourceParams waterMarkFile;
    int32_t dstX;
    int32_t dstY;
    int32_t dstW;
    int32_t dstH;
};

struct CommandOpt {
    ApiType apiType = ApiType::TEST_CODEC_BASE;
    bool isEncoder = false;
    bool isBufferMode = false;
    uint32_t ltrFrameCount = 0;
    uint32_t repeatCnt = 1;
    std::string inputFile;
    uint32_t maxReadFrameCnt = 0; // 0 means read whole file
    uint32_t dispW = 0;
    uint32_t dispH = 0;
    CodeType protocol = H264;
    VideoPixelFormat pixFmt = VideoPixelFormat::NV12;
    uint32_t frameRate = 30;
    int32_t timeout = -1;
    bool isHighPerfMode = false;
    // encoder only
    bool enableInputCb = false;
    std::optional<uint32_t> mockFrameCnt;  // when read up to maxReadFrameCnt, stop read and send input directly
    std::optional<bool> rangeFlag;
    std::optional<ColorPrimary> primary;
    std::optional<TransferCharacteristic> transfer;
    std::optional<MatrixCoefficient> matrix;
    std::optional<int32_t> iFrameInterval;
    std::optional<int> profile;
    std::optional<VideoEncodeBitrateMode> rateMode;
    std::optional<uint32_t> bitRate;  // bps
    std::optional<uint32_t> quality;
    std::optional<QPRange> qpRange;
    std::optional<uint32_t> ltrListLen;
    std::optional<int32_t> repeatAfter;
    std::optional<int32_t> repeatMaxCnt;
    std::optional<uint32_t> layerCnt;
    std::optional<int32_t> isVrrEnable;
    std::optional<int32_t> sqrFactor;
    std::optional<int32_t> maxBitrate;
    std::optional<std::string> roiParams;
    std::optional<uint32_t> targetQp;
    WaterMarkParam waterMark;
    bool paramsFeedback;
    bool enableQPMap = false;
    std::optional<int32_t> gopBMode;

    // decoder only
    bool decThenEnc = false;
    VideoRotation rotation = VIDEO_ROTATION_0;
    int flushCnt = 0;
    std::optional<uint32_t> scaleMode;

    // associate with frame number
    std::map<uint32_t, SetParameterParams> setParameterParamsMap;
    std::map<uint32_t, PerFrameParams> perFrameParamsMap;
    std::map<uint32_t, ResourceParams> resourceParamsMap;

    void Print() const;
    void ParseParamFromCmdLine(ParamType paramType, const char *cmd);
    void ParseSetParameter(uint32_t frameNo, const std::string &s);
    void ParsePerFrameParam(uint32_t frameNo, const std::string &s);
    static void ParseResourceParam(const std::string &src, ResourceParams& dst);
    void ParseWaterMark(const char *cmd);
};

CommandOpt Parse(int argc, char *argv[]);
void ShowUsage();
}
#endif