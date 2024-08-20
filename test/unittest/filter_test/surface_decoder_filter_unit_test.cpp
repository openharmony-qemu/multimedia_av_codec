/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "surface_decoder_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void SurfaceDecoderFilterUnitTest::SetUpTestCase(void) {}

void SurfaceDecoderFilterUnitTest::TearDownTestCase(void) {}

void SurfaceDecoderFilterUnitTest::SetUp(void)
{
    surfaceDecoderFilter_ =
        std::make_shared<SurfaceDecoderFilter>("testSurfaceDecoderFilter", FilterType::FILTERTYPE_VDEC);
}

void SurfaceDecoderFilterUnitTest::TearDown(void)
{
    surfaceDecoderFilter_ = nullptr;
}

/**
 * @tc.name: First
 * @tc.desc: First
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, First, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS