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
#include "av_common.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "start_code_detector.h"

namespace OHOS::MediaAVCodec {
struct CommandOpt {
    bool testCodecBaseApi = true;
    bool isEncoder = false;
    uint32_t repeatCnt = 1;
    std::string inputFile;
    uint32_t inputCnt = 0;  // 0 means read whole file, else means read inputCnt frames
    uint32_t dispW = 0;
    uint32_t dispH = 0;
    CodeType protocol = H264;
    VideoPixelFormat pixFmt = NV12;
    uint32_t frameRate = 30;
    int32_t timeout = -1;
    bool isBufferMode = false;
    bool isHighPerfMode = false;
    // encoder only
    bool rangeFlag = false;
    ColorPrimary primary = COLOR_PRIMARY_UNSPECIFIED;
    TransferCharacteristic transfer = TRANSFER_CHARACTERISTIC_UNSPECIFIED;
    MatrixCoefficient matrix = MATRIX_COEFFICIENT_UNSPECIFIED;
    int iFrameInterval = 100; // 100ms
    uint32_t numIdrFrame = 0;
    int profile = 0;
    VideoEncodeBitrateMode rateMode = CBR;
    int64_t bitRate = 200'000;  // 200000bps = 200kbps
    uint32_t quality = 50;
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