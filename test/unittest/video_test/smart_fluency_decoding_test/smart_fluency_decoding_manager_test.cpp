/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <memory>
#include <vector>
#include "smart_fluency_decoding_manager.h"
#include "smart_fluency_decoding.h"
#include "meta/video_types.h"
#include "avcodec_mime_type.h"
#include "avcodec_common.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer.h"

using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SFDManagerTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(SFDManagerTest, EnableMVOutput_FormatTag_002, TestSize.Level1)
{
    SmartFluencyDecodingManager manager("video/avc", AVCODEC_TYPE_VIDEO_DECODER);
    Format format;
    manager.EnableMVOutput(format);
    int32_t mvUpload = 0;
    EXPECT_FALSE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload));

    SmartFluencyDecodingManager managerHevc("video/hevc", AVCODEC_TYPE_VIDEO_DECODER);
    managerHevc.EnableMVOutput(format);
    EXPECT_TRUE(format.GetIntValue(Tag::VIDEO_DECODER_ENABLE_MV_UPLOAD, mvUpload));
    EXPECT_EQ(mvUpload, 1);
}
