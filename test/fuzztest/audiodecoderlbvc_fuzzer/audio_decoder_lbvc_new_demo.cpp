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
 
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <securec.h>
#include <chrono>
#include "avcodec_codec_name.h"
#include "avcodec_errors.h"
#include "demo_log.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avbuffer.h"
#include "audio_decoder_lbvc_new_demo.h"
#include "audio_decoder_base_new_demo.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioBufferNewDemo;

namespace OHOS {

static uint32_t g_supportedSampleRateSet[] = {16000};
static int32_t g_supportedChannels[] = {1};

uint16_t g_supportedSampleRateSetSize = sizeof(g_supportedSampleRateSet) / sizeof(g_supportedSampleRateSet[0]);
uint16_t g_supportedChannelsSize = sizeof(g_supportedChannels) / sizeof(g_supportedChannels[0]);

void LbvcFuzzDemo::RandomSetMeta(const uint8_t *data)
{
    int32_t channel = g_supportedChannels[static_cast<uint8_t>((*data) % g_supportedChannelsSize)];
    data++;
    uint32_t sampleRate = g_supportedSampleRateSet[static_cast<uint8_t>((*data) % g_supportedSampleRateSetSize)];
    data++;
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channel);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);

    return;
}

bool LbvcFuzzDemo::DoLbvcParserWithParserAPI(const uint8_t *data, size_t size)
{
    if (size < 2) { // 2 for ramdom set format data
        return false;
    }
    BaseFuzzDemo base;
    base.signal_ = new ADecBufferSignal();
    base.audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME).data());
    base.cb_ = base.GetCbFunc();
    DEMO_CHECK_AND_RETURN_RET_LOG(OH_AudioCodec_RegisterCallback(base.audioDec_, base.cb_, base.signal_) == AV_ERR_OK,
                                  false, "Fatal: OH_AudioCodec_RegisterCallback fail");
    base.format = OH_AVFormat_Create();
    RandomSetMeta(data);
    base.data_ = data;
    base.size_ = size;
    base.Start();
    base.Stop();
    base.Release();
    return true;
}

} // namespace OHOS