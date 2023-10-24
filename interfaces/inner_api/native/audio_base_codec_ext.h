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
#ifndef AUDIO_BASE_CODEC_EXT_H
#define AUDIO_BASE_CODEC_EXT_H
#include <string>
#include <stdint.h>

namespace OHOS {
namespace MediaAVCodec {
class AudioBaseCodecExt {
public:
    virtual int32_t Init() = 0;
    virtual int32_t ProcessSendData(void *inputBuffer, int32_t *len) = 0;
    virtual int32_t ProcessRecieveData(unsigned char **outBuffer, int32_t len) = 0;
    virtual int32_t SetPluginParameter(int32_t key, int32_t value) = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t GetInputBufferSize() = 0;
    virtual int32_t GetOutputBufferSize() = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS

typedef int32_t OpusPluginClassCreateFun(OHOS::MediaAVCodec::AudioBaseCodecExt **pluginPtr);
#endif