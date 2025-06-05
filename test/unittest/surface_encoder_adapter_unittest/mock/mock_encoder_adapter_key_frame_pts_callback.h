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

#ifndef MOCK_ENCODER_ADAPTER_KEY_FRAME_PTS_CALLBACK_H
#define MOCK_ENCODER_ADAPTER_KEY_FRAME_PTS_CALLBACK_H

#include <memory>
#include <string>
#include "gmock/gmock.h"
#include "surface_encoder_adapter.h"

namespace OHOS {
namespace Media {
class MockEncoderAdapterKeyFramePtsCallback : public EncoderAdapterKeyFramePtsCallback {
public:
    virtual ~MockEncoderAdapterKeyFramePtsCallback() = default;

    MOCK_METHOD(void, OnReportKeyFramePts, (std::string KeyFramePts), (override));
    MOCK_METHOD(void, OnReportFirstFramePts, (int64_t firstFramePts), (override));
};
} // namespace Media
} // namespace OHOS
#endif // MOCK_ENCODER_ADAPTER_KEY_FRAME_PTS_CALLBACK_H