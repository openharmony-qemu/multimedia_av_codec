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
#ifndef SURFACE_ENCODER_ADAPTER_UNITTEST_H
#define SURFACE_ENCODER_ADAPTER_UNITTEST_H
#include "gtest/gtest.h"
#include "surface_encoder_adapter.h"
#include "mock/mock_avcodec_video_encoder.h"

namespace OHOS {
namespace Media {
class SurfaceEncoderAdapterUnitTest : public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);

protected:
    std::shared_ptr<MockAVCodecVideoEncoder> mockCodecServer_ { nullptr };
    std::shared_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter_ { nullptr };
};
} // namespace Media
} // namespace OHOS
#endif // SURFACE_ENCODER_ADAPTER_UNITTEST_H