
/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <audio_hdi_codec_inner_unit_test.h>
#include <gtest/gtest.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <queue>
#include <string>
#include <hdi_codec.h>
#include "avcodec_common.h"
#include "avcodec_audio_decoder.h"
#include "avcodec_info.h"
#include "avcodec_codec_name.h"

namespace {
using namespace::std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;
using namespace Hdi;
using namespace OHOS::HDI::Codec::V4_0;

const string LBVC_DECODER_COMPONENT_NAME = "OMX.audio.decoder.lbvc";
constexpr uint32_t OMX_AUDIO_CODEC_PARAM_INDEX = 0x6F000000 + 0x00A0000B;
uint32_t g_bufferSize = 8192;
CapabilityData audioLbvcCapability;

class AudioHdiCodecInnerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    Status CheckInit();
protected:
    std::shared_ptr<Hdi::HdiCodec> hdiCodec_;
    std::shared_ptr<HdiCodecInner> hdiCodecInner_;
    std::shared_ptr<CapabilityData> cap_;
    std::unique_ptr<std::ifstream> soFile_;
    std::shared_ptr<AVCodecAudioDecoder> audioDec_ = {nullptr};
};

void AudioHdiCodecInnerUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioHdiCodecInnerUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioHdiCodecInnerUnitTest::SetUp(void)
{
    hdiCodecInner_ = std::make_shared<HdiCodecInner>();
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioHdiCodecInnerUnitTest::TearDown(void)
{
    hdiCodecInner_->Release();
    if (audioDec_) {
        audioDec_->Release();
    }
    cout << "[TearDown]: over!!!" << endl;
}

Status AudioHdiCodecInnerUnitTest::CheckInit()
{
    std::shared_ptr<Hdi::HdiCodec> codec = std::make_shared<HdiCodec>();
    Status ret = codec->InitComponent(LBVC_DECODER_COMPONENT_NAME);
    codec->Release();
    codec = nullptr;
    return ret;
}

/**
 * @tc.name: InitComponent_001
 * @tc.desc: ret == HDF_SUCCESS && compNode_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, InitComponent_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compNode = hdiCodecInner_->GetCompNode();
    compNode = nullptr;
    Status ret = hdiCodecInner_->HdiCodec::InitComponent(LBVC_DECODER_COMPONENT_NAME);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_NE(compNode, nullptr);
    auto& compCb = hdiCodecInner_->GetCompCb();
    EXPECT_NE(compCb, nullptr);
}

/**
 * @tc.name: InitComponent_002
 * @tc.desc: ret != HDF_SUCCESS && compNode_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, InitComponent_002, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compNode = hdiCodecInner_->GetCompNode();
    compNode = nullptr;
    Status ret = hdiCodecInner_->HdiCodec::InitComponent("INVALID_COMPONENT_NAME");
    EXPECT_NE(Status::OK, ret);
    EXPECT_EQ(compNode, nullptr);
    auto& compCb = hdiCodecInner_->GetCompCb();
    EXPECT_EQ(compCb, nullptr);
}

/**
 * @tc.name: InitComponent_003
 * @tc.desc: ret == HDF_SUCCESS && compNode_ != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, InitComponent_003, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compNode = hdiCodecInner_->GetCompNode();
    Status ret = hdiCodecInner_->HdiCodec::InitComponent(LBVC_DECODER_COMPONENT_NAME);
    EXPECT_EQ(Status::OK, ret);
    AudioCodecOmxParam param;
    hdiCodecInner_->InitParameter(param);
    param.sampleRate = static_cast<uint32_t>(16000); // sampleRate
    param.sampleFormat = static_cast<uint32_t>(1); // SAMPLE_S16LE
    param.channels = static_cast<uint32_t>(1); // channel num
    int8_t *p = reinterpret_cast<int8_t *>(&param);
    std::vector<int8_t> paramVec(p, p + sizeof(AudioCodecOmxParam));
    ret = hdiCodecInner_->SetParameter(OMX_AUDIO_CODEC_PARAM_INDEX, paramVec);
    EXPECT_EQ(Status::OK, ret);
    EXPECT_NE(compNode, nullptr);
    auto& compCb = hdiCodecInner_->GetCompCb();
    EXPECT_NE(compCb, nullptr);
}

/**
 * @tc.name: IsSupportCodecType_001
 * @tc.desc: compMgr_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, IsSupportCodecType_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    bool ret = hdiCodecInner_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name: IsSupportCodecType_002
 * @tc.desc: compMgr_ != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, IsSupportCodecType_002, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    hdiCodecInner_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability); // GetComponentManager called
    bool ret = hdiCodecInner_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name: InitBuffersByPort_001
 * @tc.desc: portIndex equals to PortIndex::INPUT_PORT or PortIndex::OUTPUT_PORT
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, InitBuffersByPort_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    hdiCodecInner_->InitComponent(LBVC_DECODER_COMPONENT_NAME);
    Status ret = hdiCodecInner_->InitBuffers(g_bufferSize);
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name: FreeBuffer_001
 * @tc.desc: omxBuffer != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, FreeBuffer_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    hdiCodecInner_->InitComponent(LBVC_DECODER_COMPONENT_NAME);
    Status ret = hdiCodecInner_->InitBuffers(g_bufferSize);
    EXPECT_EQ(Status::OK, ret);
    ret = hdiCodecInner_->Reset();
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name: FreeBuffer_002
 * @tc.desc: omxBuffer == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, FreeBuffer_002, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    hdiCodecInner_->InitComponent(LBVC_DECODER_COMPONENT_NAME);
    Status ret = hdiCodecInner_->Reset();
    EXPECT_EQ(Status::OK, ret);
}

/**
 * @tc.name: Release_001
 * @tc.desc: compMgr_ != nullptr && componentId_ > 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, Release_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    hdiCodecInner_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability); // GetComponentManager called
    hdiCodecInner_->InitComponent(LBVC_DECODER_COMPONENT_NAME); // get componentId_
    auto& componentId = hdiCodecInner_->GetCompId();
    EXPECT_GT(componentId, 0);
    hdiCodecInner_->Release();
    EXPECT_EQ(compMgr, nullptr);
}

/**
 * @tc.name: Release_002
 * @tc.desc: compMgr_ == nullptr && componentId_ > 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, Release_002, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    hdiCodecInner_->InitComponent(LBVC_DECODER_COMPONENT_NAME); // get componentId_
    auto& componentId = hdiCodecInner_->GetCompId();
    EXPECT_GT(componentId, 0);
    hdiCodecInner_->Release();
    EXPECT_EQ(compMgr, nullptr);
}

/**
 * @tc.name: Release_003
 * @tc.desc: compMgr_ != nullptr && componentId_ <= 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, Release_003, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    hdiCodecInner_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability); // GetComponentManager called
    auto& componentId = hdiCodecInner_->GetCompId();
    EXPECT_LE(componentId, 0);
    hdiCodecInner_->Release();
    EXPECT_EQ(compMgr, nullptr);
}

/**
 * @tc.name: Release_004
 * @tc.desc: compMgr_ == nullptr && componentId_ <= 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, Release_004, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    auto& componentId = hdiCodecInner_->GetCompId();
    EXPECT_LE(componentId, 0);
    hdiCodecInner_->Release();
    EXPECT_EQ(compMgr, nullptr);
}

/**
 * @tc.name: Release_005
 * @tc.desc: compCb_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, Release_005, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    hdiCodecInner_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability); // GetComponentManager called
    hdiCodecInner_->InitComponent(LBVC_DECODER_COMPONENT_NAME); // get componentId_
    auto& componentId = hdiCodecInner_->GetCompId();
    EXPECT_GT(componentId, 0);
    auto& compCb = hdiCodecInner_->GetCompCb();
    compCb = nullptr;
    hdiCodecInner_->Release();
    EXPECT_EQ(compMgr, nullptr);
}

/**
 * @tc.name: Release_006
 * @tc.desc: compCb_ != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, Release_006, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto& compMgr = hdiCodecInner_->GetCompMgr();
    compMgr = nullptr;
    hdiCodecInner_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability); // GetComponentManager called
    hdiCodecInner_->InitComponent(LBVC_DECODER_COMPONENT_NAME); // get componentId_
    auto& componentId = hdiCodecInner_->GetCompId();
    EXPECT_GT(componentId, 0);
    auto& compCb = hdiCodecInner_->GetCompCb();
    EXPECT_NE(compCb, nullptr);
    hdiCodecInner_->Release();
    EXPECT_EQ(compMgr, nullptr);
}

/**
 * @tc.name: EventHandler_001
 * @tc.desc: hdiCodec_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, EventHandler_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto hdiCodec = std::make_shared<HdiCodec>();
    auto hdiCodecCb = std::make_shared<HdiCodec::HdiCallback>(hdiCodec);
    auto& hdiCodec_ = hdiCodecCb->HdiCallback::GetHdiCodec();
    hdiCodec_ = nullptr;
    auto& event = hdiCodecInner_->GetCompEvent();
    const EventInfo info = {};
    int32_t ret =hdiCodecCb->EventHandler(event, info);
    EXPECT_EQ(Status::OK, static_cast<Status>(ret));
}

/**
 * @tc.name: EventHandler_002
 * @tc.desc: hdiCodec_ != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, EventHandler_002, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto hdiCodec = std::make_shared<HdiCodec>();
    auto hdiCodecCb = std::make_shared<HdiCodec::HdiCallback>(hdiCodec);
    auto& hdiCodec_ = hdiCodecCb->HdiCallback::GetHdiCodec();
    EXPECT_NE(hdiCodec_, nullptr);
    auto& event = hdiCodecInner_->GetCompEvent();
    const EventInfo info = {};
    int32_t ret =hdiCodecCb->EventHandler(event, info);
    EXPECT_EQ(Status::OK, static_cast<Status>(ret));
}

/**
 * @tc.name: EmptyBufferDone_001
 * @tc.desc: hdiCodec_ != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, EmptyBufferDone_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto hdiCodec = std::make_shared<HdiCodec>();
    auto hdiCodecCb = std::make_shared<HdiCodec::HdiCallback>(hdiCodec);
    auto& hdiCodec_ = hdiCodecCb->HdiCallback::GetHdiCodec();
    EXPECT_NE(hdiCodec_, nullptr);
    int64_t appData = 0;
    const OmxCodecBuffer& buffer = {};
    int32_t ret =hdiCodecCb->EmptyBufferDone(appData, buffer);
    EXPECT_EQ(Status::OK, static_cast<Status>(ret));
}

/**
 * @tc.name: EmptyBufferDone_002
 * @tc.desc: hdiCodec_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, EmptyBufferDone_002, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto hdiCodec = std::make_shared<HdiCodec>();
    auto hdiCodecCb = std::make_shared<HdiCodec::HdiCallback>(hdiCodec);
    auto& hdiCodec_ = hdiCodecCb->HdiCallback::GetHdiCodec();
    hdiCodec_ = nullptr;
    int64_t appData = 0;
    const OmxCodecBuffer& buffer = {};
    int32_t ret =hdiCodecCb->EmptyBufferDone(appData, buffer);
    EXPECT_EQ(Status::OK, static_cast<Status>(ret));
}

/**
 * @tc.name: FillBufferDone_001
 * @tc.desc: hdiCodec_ != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, FillBufferDone_001, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto hdiCodec = std::make_shared<HdiCodec>();
    auto hdiCodecCb = std::make_shared<HdiCodec::HdiCallback>(hdiCodec);
    auto& hdiCodec_ = hdiCodecCb->HdiCallback::GetHdiCodec();
    EXPECT_NE(hdiCodec_, nullptr);
    int64_t appData = 0;
    const OmxCodecBuffer& buffer = {};
    int32_t ret =hdiCodecCb->FillBufferDone(appData, buffer);
    EXPECT_EQ(Status::OK, static_cast<Status>(ret));
}

/**
 * @tc.name: FillBufferDone_002
 * @tc.desc: hdiCodec_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AudioHdiCodecInnerUnitTest, FillBufferDone_002, TestSize.Level1)
{
    if (CheckInit() != Status::OK) {
        return;
    }
    auto hdiCodec = std::make_shared<HdiCodec>();
    auto hdiCodecCb = std::make_shared<HdiCodec::HdiCallback>(hdiCodec);
    auto& hdiCodec_ = hdiCodecCb->HdiCallback::GetHdiCodec();
    hdiCodec_ = nullptr;
    int64_t appData = 0;
    const OmxCodecBuffer& buffer = {};
    int32_t ret =hdiCodecCb->FillBufferDone(appData, buffer);
    EXPECT_EQ(Status::OK, static_cast<Status>(ret));
}
}