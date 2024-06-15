/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <memory>
#include <iostream>
#include "gtest/gtest.h"
#include "avcodec_sysevent.h"

using namespace testing::ext;

namespace OHOS {
namespace MediaAVCodec {
class AVCodecSysEventTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
    void TearDown(void) {};
};

HWTEST_F(AVCodecSysEventTest, FaultEventWrite, TestSize.Level1)
{
    FaultType faultType = FaultType::FAULT_TYPE_INVALID;
    const std::string msg = "avcodec fault event for test";
    const std::string module = "unitest";
    FaultEventWrite(faultType, msg, module);
}

HWTEST_F(AVCodecSysEventTest, ServiceStartEventWrite, TestSize.Level1)
{
    uint32_t useTime = 0;
    const std::string module = "unitest";
    ServiceStartEventWrite(useTime, module);
}

HWTEST_F(AVCodecSysEventTest, CodecStartEventWrite, TestSize.Level1)
{
    CodecDfxInfo codecDfxInfo;
    codecDfxInfo.clientPid = 0;
    codecDfxInfo.clientUid = 0;
    codecDfxInfo.codecInstanceId = 0;
    codecDfxInfo.codecName = "unitest";
    codecDfxInfo.codecIsVendor = "unitest";
    codecDfxInfo.codecMode = "unitest";
    codecDfxInfo.encoderBitRate = 0;
    codecDfxInfo.videoWidth = 0;
    codecDfxInfo.videoHeight = 0;
    codecDfxInfo.videoFrameRate = 0;
    codecDfxInfo.videoPixelFormat = "unitest";
    codecDfxInfo.audioChannelCount = 0;
    codecDfxInfo.audioSampleRate = 0;
    CodecStartEventWrite(codecDfxInfo);
}

HWTEST_F(AVCodecSysEventTest, CodecStopEventWrite, TestSize.Level1)
{
    int32_t clientPid = 0;
    int32_t clientUid = 0;
    int32_t codecInstanceId = 0;
    CodecStopEventWrite(clientPid, clientUid, codecInstanceId);
}

HWTEST_F(AVCodecSysEventTest, DemuxerInitEventWrite, TestSize.Level1)
{
    uint32_t downloadSize = 0;
    std::string sourceType = "unitest";
    DemuxerInitEventWrite(downloadSize, sourceType);
}
}
}