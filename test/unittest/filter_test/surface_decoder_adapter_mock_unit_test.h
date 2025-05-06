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

#ifndef SURFACE_DECODER_ADAPTER_MOCK_UNIT_TEST_H
#define SURFACE_DECODER_ADAPTER_MOCK_UNIT_TEST_H

#include "gtest/gtest.h"
#include "surface_decoder_adapter.h"
#include "mock/mock_avcodec_video_decoder.h"
#include "mock/mock_avbuffer_queue_consumer.h"

namespace OHOS {
namespace Media {
class SurfaceDecoderMockUnitTest : public testing::Test {
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
    std::shared_ptr<SurfaceDecoderAdapter> surfaceDecoderAdapter_{ nullptr };
    std::shared_ptr<MockAVCodecVideoDecoder> mockCodecServer_{ nullptr };
    std::shared_ptr<MockDecoderAdapterCallback> mockDecoderAdapterCallback_{ nullptr };
    MockAVBufferQueueConsumer *mockInputBufferQueueConsumer_{ nullptr };
};
} // namespace Media
} // namespace OHOS
#endif // SURFACE_DECODER_ADAPTER_MOCK_UNIT_TEST_H