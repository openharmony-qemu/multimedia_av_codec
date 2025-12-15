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

#ifndef VP9_DECODER_LOADER_H
#define VP9_DECODER_LOADER_H
#include "video_codec_loader.h"
#include "codecbase.h"
#include "codeclistbase.h"
namespace OHOS {
namespace MediaAVCodec {
class Vp9DecoderLoader : public VideoCodecLoader {
public:
    static std::shared_ptr<CodecBase> CreateByName(const std::string &name);
    static int32_t GetCapabilityList(std::vector<CapabilityData> &caps);

private:
    Vp9DecoderLoader();
    ~Vp9DecoderLoader() = default;
    void CloseLibrary();
    static Vp9DecoderLoader &GetInstance();

    std::mutex mutex_;
    int32_t vp9DecoderCount_ = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif