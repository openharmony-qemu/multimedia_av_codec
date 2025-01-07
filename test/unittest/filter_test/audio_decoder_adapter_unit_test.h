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
#ifndef HISTREAMER_AUDIO_SINK_FILTER_UNIT_TEST_H
#define HISTREAMER_AUDIO_SINK_FILTER_UNIT_TEST_H

#include <atomic>
#include "gtest/gtest.h"
#include "avcodec_audio_codec.h"
#include "avbuffer_queue.h"
#include "audio_decoder_adapter.h"
#include "gmock/gmock.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class AudioDecoderAdapterUnitTest : public testing::Test {
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
    std::shared_ptr<AudioDecoderAdapter> audioDecoderAdapter_{ nullptr };
};


class MockAVCodecAudioCodec : public MediaAVCodec::AVCodecAudioCodec {
public:
    MOCK_METHOD(int32_t, Configure, (const std::shared_ptr<Meta> &meta), (override));
    MOCK_METHOD(int32_t, SetOutputBufferQueue, (const sptr<AVBufferQueueProducer> &bufferQueueProducer), (override));
    MOCK_METHOD(int32_t, Prepare, (), (override));
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetInputBufferQueue, (), (override));
    MOCK_METHOD(int32_t, Start, (), (override));
    MOCK_METHOD(int32_t, Stop, (), (override));
    MOCK_METHOD(int32_t, Flush, (), (override));
    MOCK_METHOD(int32_t, Reset, (), (override));
    MOCK_METHOD(int32_t, Release, (), (override));
    MOCK_METHOD(int32_t, NotifyEos, (), (override));
    MOCK_METHOD(int32_t, SetParameter, (const std::shared_ptr<Meta> &parameter), (override));
    MOCK_METHOD(int32_t, GetOutputFormat, (std::shared_ptr<Meta> &parameter), (override));
    MOCK_METHOD(int32_t, ChangePlugin, (const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta), (override));
    MOCK_METHOD(int32_t, SetCodecCallback, (const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &codecCallback), (override));
    MOCK_METHOD(int32_t, SetAudioDecryptionConfig, (const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag), (override));
    MOCK_METHOD(void, ProcessInputBuffer, (), (override));
    MOCK_METHOD(void, SetDumpInfo, (bool isDump, uint64_t instanceId), (override));
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_AUDIO_SINK_FILTER_UNIT_TEST_H