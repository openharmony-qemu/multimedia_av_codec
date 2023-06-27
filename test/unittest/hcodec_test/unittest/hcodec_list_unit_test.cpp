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

#include <vector>
#include "securec.h"
#include "hcodec_list_unit_test.h"
#include "hcodec_list.h"
#include "hcodec_log.h"
#include "hcodec.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace testing::ext;

/*========================================================*/
/*               HCodecListUnitTest                */
/*========================================================*/
void HCodecListUnitTest::SetUpTestCase(void)
{
}

void HCodecListUnitTest::TearDownTestCase(void)
{
}

void HCodecListUnitTest::SetUp(void)
{
}

void HCodecListUnitTest::TearDown(void)
{
}

string HCodecListUnitTest::GetPrintInfo(const Range& obj)
{
    char info[64] = {0};
    (void)sprintf_s(info, sizeof(info), "[%d,%d]", obj.minVal, obj.maxVal);
    return string(info);
}

string HCodecListUnitTest::GetPrintInfo(const ImgSize& obj)
{
    char info[64] = {0};
    (void)sprintf_s(info, sizeof(info), "[w:%d,h:%d]", obj.width, obj.height);
    return string(info);
}

string HCodecListUnitTest::GetPrintInfo(const vector<int32_t>& obj)
{
    string info = "";
    for (const int32_t& one : obj) {
        char tmp[16] = {0};
        (void)sprintf_s(tmp, sizeof(tmp), "%d,", one);
        info += string(tmp);
    }
    return info;
}

string HCodecListUnitTest::GetPrintInfo(const map<int32_t, vector<int32_t>>& obj)
{
    string info = "";
    for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
        char tmp[16] = {0};
        (void)sprintf_s(tmp, sizeof(tmp), "%d", iter->first);
        info += "(K=" + string(tmp) + " V=" + GetPrintInfo(iter->second) + "),";
    }
    return info;
}

string HCodecListUnitTest::GetPrintInfo(const map<ImgSize, Range>& obj)
{
    string info = "";
    for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
        info += "(K=" + GetPrintInfo(iter->first) + " V=" + GetPrintInfo(iter->second) + "),";
    }
    return info;
}

HWTEST_F(HCodecListUnitTest, get_hcodec_capability_ok, TestSize.Level1)
{
    HCodecList testObj = HCodecList();
    vector<CapabilityData> capData;
    int32_t ret = testObj.GetCapabilityList(capData);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ASSERT_NE(0, capData.size());
    for (const CapabilityData& one : capData) {
        LOGI("codecName=%{public}s; codecType=%{public}d; mimeType=%{public}s; isVendor=%{public}d; " \
            "maxInstance=%{public}d; bitrate=%{public}s; channels=%{public}s; complexity=%{public}s; " \
            "alignment=%{public}s; width=%{public}s; height=%{public}s; frameRate=%{public}s; " \
            "encodeQuality=%{public}s; blockPerFrame=%{public}s; blockPerSecond=%{public}s; blockSize=%{public}s; " \
            "sampleRate=%{public}s; pixFormat=%{public}s; bitDepth=%{public}s; profiles=%{public}s; " \
            "bitrateMode=%{public}s; profileLevelsMap=%{public}s; measuredFrameRate=%{public}s; " \
            "supportSwapWidthHeight=%{public}d",
            one.codecName.c_str(), one.codecType, one.mimeType.c_str(), one.isVendor, one.maxInstance,
            GetPrintInfo(one.bitrate).c_str(), GetPrintInfo(one.channels).c_str(),
            GetPrintInfo(one.complexity).c_str(), GetPrintInfo(one.alignment).c_str(),
            GetPrintInfo(one.width).c_str(), GetPrintInfo(one.height).c_str(),
            GetPrintInfo(one.frameRate).c_str(), GetPrintInfo(one.encodeQuality).c_str(),
            GetPrintInfo(one.blockPerFrame).c_str(), GetPrintInfo(one.blockPerSecond).c_str(),
            GetPrintInfo(one.blockSize).c_str(), GetPrintInfo(one.sampleRate).c_str(),
            GetPrintInfo(one.pixFormat).c_str(), GetPrintInfo(one.bitDepth).c_str(),
            GetPrintInfo(one.profiles).c_str(), GetPrintInfo(one.bitrateMode).c_str(),
            GetPrintInfo(one.profileLevelsMap).c_str(), GetPrintInfo(one.measuredFrameRate).c_str(),
            one.supportSwapWidthHeight);
    }
}

HWTEST_F(HCodecListUnitTest, max_instance_avc_decoder, TestSize.Level1)
{
    string compName = "OMX.hisi.video.decoder.avc";
    constexpr int maxInst = 16;
    vector<shared_ptr<HCodec>> objPool;
    for (int i = 0; i < maxInst; ++i) {
        LOGI("%{public}s: inst %{public}d", compName.c_str(), i);
        std::shared_ptr<HCodec> testObj = HCodec::Create(compName);
        EXPECT_TRUE(testObj);
        objPool.push_back(testObj);
    }
    std::shared_ptr<HCodec> failObj = HCodec::Create(compName);
    EXPECT_FALSE(failObj);
    objPool.clear();
}

HWTEST_F(HCodecListUnitTest, max_instance_hevc_decoder, TestSize.Level1)
{
    string compName = "OMX.hisi.video.decoder.hevc";
    constexpr int maxInst = 16;
    vector<shared_ptr<HCodec>> objPool;
    for (int i = 0; i < maxInst; ++i) {
        LOGI("%{public}s: inst %{public}d", compName.c_str(), i);
        std::shared_ptr<HCodec> testObj = HCodec::Create(compName);
        EXPECT_TRUE(testObj);
        objPool.push_back(testObj);
    }
    std::shared_ptr<HCodec> failObj = HCodec::Create(compName);
    EXPECT_FALSE(failObj);
    objPool.clear();
}

HWTEST_F(HCodecListUnitTest, max_instance_avc_encoder, TestSize.Level1)
{
    string compName = "OMX.hisi.video.encoder.avc";
    constexpr int maxInst = 16;
    vector<shared_ptr<HCodec>> objPool;
    for (int i = 0; i < maxInst; ++i) {
        LOGI("%{public}s: inst %{public}d", compName.c_str(), i);
        std::shared_ptr<HCodec> testObj = HCodec::Create(compName);
        EXPECT_TRUE(testObj);
        objPool.push_back(testObj);
    }
    std::shared_ptr<HCodec> failObj = HCodec::Create(compName);
    EXPECT_FALSE(failObj);
    objPool.clear();
}

HWTEST_F(HCodecListUnitTest, max_instance_hevc_encoder, TestSize.Level1)
{
    string compName = "OMX.hisi.video.encoder.hevc";
    constexpr int maxInst = 16;
    vector<shared_ptr<HCodec>> objPool;
    for (int i = 0; i < maxInst; ++i) {
        LOGI("%{public}s: inst %{public}d", compName.c_str(), i);
        std::shared_ptr<HCodec> testObj = HCodec::Create(compName);
        EXPECT_TRUE(testObj);
        objPool.push_back(testObj);
    }
    std::shared_ptr<HCodec> failObj = HCodec::Create(compName);
    EXPECT_FALSE(failObj);
    objPool.clear();
}
}