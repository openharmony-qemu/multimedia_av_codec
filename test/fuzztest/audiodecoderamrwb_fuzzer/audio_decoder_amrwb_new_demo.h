/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#ifndef AVCODEC_AUDIO_AVBUFFER_DECODER_NEW_DEMO_H
#define AVCODEC_AUDIO_AVBUFFER_DECODER_NEW_DEMO_H

#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <atomic>
#include <queue>
#include <string>
#include <thread>
#include "native_avcodec_audiocodec.h"
#include "nocopyable.h"
#include "common/native_mfmagic.h"
#include "avcodec_audio_common.h"
#include "audio_decoder_base_new_demo.h"

namespace OHOS {
namespace MediaAVCodec {
namespace AudioBufferNewDemo {

using namespace std;
using namespace OHOS::Media;

class AmrwbFuzzDemo : public BaseFuzzDemo {
public:
    void RandomSetMeta(const uint8_t *data);
    bool DoAmrwbParserWithParserAPI(const uint8_t *data, size_t size);
};
} // namespace AudioBufferNewDemo
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_AVBUFFER_DECODER_NEW_DEMO_H