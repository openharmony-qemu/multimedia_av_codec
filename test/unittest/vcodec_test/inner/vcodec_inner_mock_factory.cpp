/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "videodec_inner_mock.h"
#include "videoenc_inner_mock.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<VideoDecMock> VCodecMockFactory::CreateVideoDecMockByMime(const std::string &mime)
{
    auto videoDec = VideoDecoderFactory::CreateByMime(mime);
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecInnerMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoDecMock> VCodecMockFactory::CreateVideoDecMockByName(const std::string &name)
{
    auto videoDec = VideoDecoderFactory::CreateByName(name);
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecInnerMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> VCodecMockFactory::CreateVideoEncMockByMime(const std::string &mime)
{
    auto videoEnc = VideoEncoderFactory::CreateByMime(mime);
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncInnerMock>(videoEnc);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> VCodecMockFactory::CreateVideoEncMockByName(const std::string &name)
{
    auto videoEnc = VideoEncoderFactory::CreateByName(name);
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncInnerMock>(videoEnc);
    }
    return nullptr;
}
}  // namespace MediaAVCodec
}  // namespace OHOS