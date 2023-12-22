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

struct CommandOpt {
    ApiType apiType = ApiType::TEST_CODEC_BASE;
    bool isEncoder = false;
    bool isBufferMode = false;
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
    std::optional<uint32_t> mockFrameCnt;  // when read up to maxReadFrameCnt, stop read and send input directly
    std::optional<bool> rangeFlag;
    std::optional<ColorPrimary> primary;
    std::optional<TransferCharacteristic> transfer;
    std::optional<MatrixCoefficient> matrix;
    std::optional<int32_t> iFrameInterval;
    std::optional<uint32_t> idrFrameNo;
    std::optional<int> profile;
    std::optional<VideoEncodeBitrateMode> rateMode;
    std::optional<uint32_t> bitRate;  // bps
    std::optional<uint32_t> quality;
    // decoder only
    bool render = false;
    bool decThenEnc = false;
    VideoRotation rotation = VIDEO_ROTATION_0;
    int flushCnt = 0;

    void Print() const;
};

CommandOpt Parse(int argc, char *argv[]);
void ShowUsage();
}
#endif