/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef CODEC_CAPABILITY_ADAPTER_UNITTEST_H
#define CODEC_CAPABILITY_ADAPTER_UNITTEST_H

#include "gtest/gtest.h"
#include "codec_capability_adapter.h"
#include "mock/mock_avcodec_list.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class CodecCapabilityAdapterUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    MediaAVCodec::CapabilityData capabilityData_;
    std::shared_ptr<MockAVCodecList> mockAvcodecList_ { nullptr };
    std::shared_ptr<CodecCapabilityAdapter> codecCapabilityAdapter_ { nullptr };
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // CODEC_CAPABILITY_ADAPTER_UNITTEST_H