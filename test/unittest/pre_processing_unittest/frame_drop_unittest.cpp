/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "frame_drop_filter.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {
using namespace testing::ext;

// ========== Configure 测试 ==========

class FrameDropFilterTest : public testing::Test {};

/**
 * @tc.name: FrameDropFilter_Configure_NoDropKey_Disabled_001
 * @tc.desc: 未配置丢帧目标时Configure返回OK
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Configure_NoDropKey_Disabled, TestSize.Level1)
{
    FrameDropFilter filter;
    int32_t ret = filter.Configure(0.0, 30.0);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: FrameDropFilter_Configure_ValidConfig_Enabled_001
 * @tc.desc: 配置有效丢帧参数后成功
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Configure_ValidConfig_Enabled, TestSize.Level1)
{
    FrameDropFilter filter;
    int32_t ret = filter.Configure(25.0, 30.0);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: FrameDropFilter_Configure_NegativeFps_Disabled_001
 * @tc.desc: 负帧率目标时不使能
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Configure_NegativeFps_Disabled, TestSize.Level1)
{
    FrameDropFilter filter;
    int32_t ret = filter.Configure(-1.0, 30.0);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

// ========== ShouldDrop 未使能/未配置 测试 ==========

/**
 * @tc.name: FrameDropFilter_ShouldDrop_NotConfigured_ReturnFalse_001
 * @tc.desc: 未Configure时ShouldDrop返回false
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, ShouldDrop_NotConfigured_ReturnFalse, TestSize.Level1)
{
    FrameDropFilter filter;
    EXPECT_FALSE(filter.ShouldDropFrame(0));
}

/**
 * @tc.name: FrameDropFilter_ShouldDrop_Disabled_ReturnFalse_001
 * @tc.desc: 未配置丢帧目标时ShouldDrop返回false
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, ShouldDrop_Disabled_ReturnFalse, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(0.0, 30.0);
    EXPECT_FALSE(filter.ShouldDropFrame(0));
}

// ========== ShouldDrop 时间戳策略（PTS递增） ==========

/**
 * @tc.name: FrameDropFilter_ShouldDrop_FirstFrameKept_001
 * @tc.desc: 已启用丢帧时第一帧保留
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, ShouldDrop_FirstFrameKept, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(25.0, 30.0);
    EXPECT_FALSE(filter.ShouldDropFrame(0));
}

/**
 * @tc.name: FrameDropFilter_Timestamp_30fpsTo25fps_001
 * @tc.desc: PTS递增时选择时间戳策略，30fps→25fps保留约5/6
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Timestamp_30fpsTo25fps, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(25.0, 30.0);

    int kept = 0;
    for (int i = 0; i < 60; i++) {
        if (!filter.ShouldDropFrame(i * 33333)) {
            kept++;
        }
    }
    EXPECT_GE(kept, 45);
    EXPECT_LE(kept, 55);
}

/**
 * @tc.name: FrameDropFilter_Timestamp_60fpsTo30fps_001
 * @tc.desc: PTS递增时60fps→30fps，60帧中保留28~32帧
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Timestamp_60fpsTo30fps, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(30.0, 60.0);

    int kept = 0;
    for (int i = 0; i < 60; i++) {
        if (!filter.ShouldDropFrame(i * 16667)) {
            kept++;
        }
    }
    EXPECT_GE(kept, 28);
    EXPECT_LE(kept, 32);
}

/**
 * @tc.name: FrameDropFilter_Timestamp_30fpsTo29_97fps_001
 * @tc.desc: PTS递增时30fps→29.97fps，3000帧中必须丢帧
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Timestamp_30fpsTo29_97fps, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(29.97, 30.0);

    int kept = 0;
    for (int i = 0; i < 3000; i++) {
        if (!filter.ShouldDropFrame(i * 33333)) {
            kept++;
        }
    }
    EXPECT_GE(kept, 2994);
    EXPECT_LT(kept, 3000);
}

/**
 * @tc.name: FrameDropFilter_Timestamp_VFR_001
 * @tc.desc: 可变帧率输入，降采样结果在合理范围内
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Timestamp_VFR, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(25.0, 30.0);

    int64_t ptsArray[] = {0,      50000,  83333,  116666, 150000, 183333, 216666, 250000,
                          283333, 316666, 350000, 383333, 416666, 450000, 483333};
    int kept = 0;
    for (int i = 0; i < 15; i++) {
        if (!filter.ShouldDropFrame(ptsArray[i])) {
            kept++;
        }
    }
    EXPECT_GE(kept, 10);
    EXPECT_LE(kept, 15);
}

// ========== ShouldDrop 比例策略（PTS不变） ==========

/**
 * @tc.name: FrameDropFilter_Ratio_30fpsTo25fps_001
 * @tc.desc: PTS不变时选择比例策略，30fps→25fps保留25/30
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Ratio_30fpsTo25fps, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(25.0, 30.0);

    int kept = 0;
    for (int i = 0; i < 30; i++) {
        if (!filter.ShouldDropFrame(0)) {
            kept++;
        }
    }
    EXPECT_EQ(kept, 25);
}

/**
 * @tc.name: FrameDropFilter_Ratio_60fpsTo30fps_001
 * @tc.desc: PTS不变时60fps→30fps，保留30/60
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Ratio_60fpsTo30fps, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(30.0, 60.0);

    int kept = 0;
    for (int i = 0; i < 60; i++) {
        if (!filter.ShouldDropFrame(0)) {
            kept++;
        }
    }
    EXPECT_EQ(kept, 30);
}

/**
 * @tc.name: FrameDropFilter_Ratio_30fpsTo29_97fps_001
 * @tc.desc: PTS不变时30fps→29.97fps，3000帧中必须丢帧
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Ratio_30fpsTo29_97fps, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(29.97, 30.0);

    int kept = 0;
    for (int i = 0; i < 3000; i++) {
        if (!filter.ShouldDropFrame(0)) {
            kept++;
        }
    }
    EXPECT_GE(kept, 2994);
    EXPECT_LT(kept, 3000);
}

/**
 * @tc.name: FrameDropFilter_Ratio_30fpsTo15fps_001
 * @tc.desc: PTS不变时30fps→15fps，保留15/30
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Ratio_30fpsTo15fps, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(15.0, 30.0);

    int kept = 0;
    for (int i = 0; i < 30; i++) {
        if (!filter.ShouldDropFrame(0)) {
            kept++;
        }
    }
    EXPECT_EQ(kept, 15);
}

/**
 * @tc.name: FrameDropFilter_Ratio_InvalidRatio_PassThrough_001
 * @tc.desc: 目标>源帧率时全部保留（透传）
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Ratio_InvalidRatio_PassThrough, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(30.0, 15.0);

    int kept = 0;
    for (int i = 0; i < 30; i++) {
        if (!filter.ShouldDropFrame(0)) {
            kept++;
        }
    }
    EXPECT_EQ(kept, 30);
}

// ========== 溢出/回绕 测试 ==========

/**
 * @tc.name: FrameDropFilter_Ratio_OverflowPts_001
 * @tc.desc: PTS为UINT64_MAX附近值，比例策略正常工作
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Ratio_OverflowPts, TestSize.Level1)
{
    constexpr uint64_t largePts = UINT64_MAX - 10;
    FrameDropFilter filter;
    filter.Configure(15.0, 30.0);

    int kept = 0;
    for (int i = 0; i < 30; i++) {
        if (!filter.ShouldDropFrame(largePts)) {
            kept++;
        }
    }
    EXPECT_EQ(kept, 15);
}

/**
 * @tc.name: FrameDropFilter_Timestamp_OverflowPts_001
 * @tc.desc: PTS从UINT64_MAX附近开始递增（回绕），时间戳策略正常丢帧
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Timestamp_OverflowPts, TestSize.Level1)
{
    constexpr uint64_t startPts = UINT64_MAX - 33333;
    FrameDropFilter filter;
    filter.Configure(15.0, 30.0);

    int kept = 0;
    for (int i = 0; i < 60; i++) {
        uint64_t pts = startPts + static_cast<uint64_t>(i) * 33333ULL;
        if (!filter.ShouldDropFrame(pts)) {
            kept++;
        }
    }
    EXPECT_GE(kept, 28);
    EXPECT_LE(kept, 35);
}

/**
 * @tc.name: FrameDropFilter_Timestamp_PtsZero_001
 * @tc.desc: PTS从0开始，时间戳策略正常丢帧
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Timestamp_PtsZero, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(15.0, 30.0);

    int kept = 0;
    for (int i = 0; i < 60; i++) {
        if (!filter.ShouldDropFrame(static_cast<uint64_t>(i) * 33333ULL)) {
            kept++;
        }
    }
    EXPECT_GE(kept, 28);
    EXPECT_LE(kept, 35);
}

// ========== FlushTimeStamp 测试 ==========

/**
 * @tc.name: FrameDropFilter_FlushTimeStamp_TimestampResets_001
 * @tc.desc: FlushTimeStamp后时间戳策略重置基准，下一帧作为新起点
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, FlushTimeStamp_TimestampResets, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(15.0, 30.0);

    // Feed some frames to establish timestamp strategy
    filter.ShouldDropFrame(0);
    for (int i = 1; i <= 10; i++) {
        filter.ShouldDropFrame(static_cast<uint64_t>(i) * 33333ULL);
    }

    filter.FlushTimeStamp();

    // After flush, next frame should be kept as new base
    EXPECT_FALSE(filter.ShouldDropFrame(500000));
}

/**
 * @tc.name: FrameDropFilter_Reconfigure_SwitchesStrategy_001
 * @tc.desc: 重新Configure可以切换策略
 * @tc.type: FUNC
 */
HWTEST_F(FrameDropFilterTest, Reconfigure_SwitchesStrategy, TestSize.Level1)
{
    FrameDropFilter filter;
    filter.Configure(25.0, 30.0);
    filter.ShouldDropFrame(0);
    filter.ShouldDropFrame(33333);

    int32_t ret = filter.Configure(15.0, 30.0);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    int kept = 0;
    for (int i = 0; i < 30; i++) {
        if (!filter.ShouldDropFrame(0)) {
            kept++;
        }
    }
    EXPECT_EQ(kept, 15);
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
