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

#include "hcodec_list_unit_test.h"
#include <vector>
#include "hcodec_api.h"
#include "hcodec_list.h"
#include "hcodec_log.h"
#include "securec.h"

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
    TLOGI("----- %s -----", ::testing::UnitTest::GetInstance()->current_test_info()->name());
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

void HCodecListUnitTest::CheckRange(const Range& range)
{
    EXPECT_TRUE(range.minVal >= 0);
    EXPECT_TRUE(range.maxVal >= range.minVal);
}

void HCodecListUnitTest::CheckImgSize(const ImgSize& size)
{
    EXPECT_TRUE(size.width > 0);
    EXPECT_TRUE(size.height > 0);
}

HWTEST_F(HCodecListUnitTest, get_hcodec_capability_ok, TestSize.Level1)
{
    HCodecList testObj = HCodecList();
    vector<CapabilityData> capData;
    int32_t ret = testObj.GetCapabilityList(capData);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ASSERT_NE(0, capData.size());
    for (const CapabilityData& one : capData) {
        TLOGI("codecName=%s; codecType=%d; mimeType=%s; isVendor=%d; " \
            "maxInstance=%d; bitrate=%s; channels=%s; complexity=%s; " \
            "alignment=%s; width=%s; height=%s; frameRate=%s; " \
            "encodeQuality=%s; blockPerFrame=%s; blockPerSecond=%s; blockSize=%s; " \
            "sampleRate=%s; pixFormat=%s; bitDepth=%s; profiles=%s; " \
            "bitrateMode=%s; profileLevelsMap=%s; measuredFrameRate=%s; " \
            "supportSwapWidthHeight=%d",
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

HWTEST_F(HCodecListUnitTest, check_param, TestSize.Level1)
{
    vector<CapabilityData> caps;
    int32_t ret = GetHCodecCapabilityList(caps);
    ASSERT_EQ(ret, AVCS_ERR_OK);
    ASSERT_TRUE(!caps.empty());
    for (const CapabilityData& cap : caps) {
        if (cap.mimeType == "video/avc" || cap.mimeType == "video/hevc") {
            EXPECT_TRUE(!cap.codecName.empty());
            EXPECT_TRUE(cap.codecType == AVCODEC_TYPE_VIDEO_ENCODER || cap.codecType == AVCODEC_TYPE_VIDEO_DECODER);
            EXPECT_TRUE(cap.isVendor);
            EXPECT_TRUE(cap.maxInstance > 0);
            CheckRange(cap.bitrate);
            CheckImgSize(cap.alignment);
            CheckRange(cap.width);
            CheckRange(cap.height);
            EXPECT_TRUE(!cap.pixFormat.empty());
            EXPECT_TRUE(!cap.profiles.empty());
            EXPECT_TRUE(!cap.profileLevelsMap.empty());
        }
    }
}
}