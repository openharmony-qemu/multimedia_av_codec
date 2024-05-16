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
namespace Media {
using namespace OHOS::MediaAVCodec;
class MediaDfxTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
    void TearDown(void) {};
};

HWTEST_F(MediaDfxTest, FAULT_DEMUXER_EVENT, TestSize.Level1)
{
    DemuxerFaultInfo dmuxerDfxInfo;
    dmuxerDfxInfo.appName = "appName";
    dmuxerDfxInfo.instanceId = "1";
    dmuxerDfxInfo.callerType = "player_framework";
    dmuxerDfxInfo.sourceType = 1;
    dmuxerDfxInfo.containerFormat = "video/mp4v-es;audio/mp4a-latm";
    dmuxerDfxInfo.streamType = "streamType";
    dmuxerDfxInfo.errMsg = "errorMessage";
    FaultDemuxerEventWrite(dmuxerDfxInfo);
}

HWTEST_F(MediaDfxTest, FAULT_MUXER_EVENT, TestSize.Level1)
{
    MuxerFaultInfo muxerFaultInfo;
    muxerFaultInfo.appName = "appName";
    muxerFaultInfo.instanceId = "1";
    muxerFaultInfo.callerType = "player_framework";
    muxerFaultInfo.videoCodec = "video/mp4v-es";
    muxerFaultInfo.audioCodec = "audio/mp4a-latm";
    muxerFaultInfo.containerFormat = "video/mp4v-es;audio/mp4a-latm";
    muxerFaultInfo.errMsg = "errorMessage";
    FaultMuxerEventWrite(muxerFaultInfo);
}

HWTEST_F(MediaDfxTest, FAULT_AUDIO_CODEC_EVENT, TestSize.Level1)
{
    AudioCodecFaultInfo audioCodecFaultInfo;
    audioCodecFaultInfo.appName = "appName";
    audioCodecFaultInfo.instanceId = "1";
    audioCodecFaultInfo.callerType = "player_framework";
    audioCodecFaultInfo.audioCodec = "audio/mp4a-latm";
    audioCodecFaultInfo.errMsg = "errorMessage";
    FaultAudioCodecEventWrite(audioCodecFaultInfo);
}

HWTEST_F(MediaDfxTest, FAULT_VIDEO_CODEC_EVENT, TestSize.Level1)
{
    VideoCodecFaultInfo videoCodecFaultInfo;
    videoCodecFaultInfo.appName = "appName";
    videoCodecFaultInfo.instanceId = "1";
    videoCodecFaultInfo.callerType = "player_framework";
    videoCodecFaultInfo.videoCodec = "video/mp4v-es";
    videoCodecFaultInfo.errMsg = "errorMessage";
    FaultVideoCodecEventWrite(videoCodecFaultInfo);
}

HWTEST_F(MediaDfxTest, FAULT_RECORD_AUDIO_EVENT, TestSize.Level1)
{
    AudioSourceFaultInfo audioSourceFaultInfo;
    audioSourceFaultInfo.appName = "appName";
    audioSourceFaultInfo.instanceId = "1";
    audioSourceFaultInfo.audioSourceType = 1;
    audioSourceFaultInfo.errMsg = "errorMessage";
    FaultRecordAudioEventWrite(audioSourceFaultInfo);
}
}
}