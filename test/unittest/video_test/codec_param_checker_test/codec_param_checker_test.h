/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef CODEC_PARAM_CHECK_TEST_H
#define CODEC_PARAM_CHECK_TEST_H
#include <gtest/gtest.h>
#include "avcodec_list.h"
#include "avcodec_video_encoder.h"
#include "codeclist_mock.h"
#include "format.h"
class AVCodecParamCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<OHOS::MediaAVCodec::AVCodecList> codeclist_;
    std::shared_ptr<OHOS::MediaAVCodec::AVCodecVideoEncoder> videoEncHevcInner_;
    std::shared_ptr<OHOS::MediaAVCodec::CodecListMock> capability_ = nullptr;
    OHOS::MediaAVCodec::CapabilityData *capabilityDataHevc_;
    OHOS::MediaAVCodec::Format formatInner_;
};
#endif // CODEC_PARAM_CHECK_TEST_H

