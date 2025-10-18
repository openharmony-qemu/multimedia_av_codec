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
#ifndef __AVCODEC_AUDIO_ADPCM_DECODER_DEMO_H__
#define __AVCODEC_AUDIO_ADPCM_DECODER_DEMO_H__

#include <fstream>
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {
namespace Plugins {

class AudioAdpcmDecoderDemo : public DataCallback {
public:
    bool ReadBuffer(std::shared_ptr<AVBuffer> buffer);
    Status ProcessDecoder();
    bool InitDecoder(std::string name);
    void ReleaseDecoder();
    void RunCase();

    void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
    }

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
    }

    void OnEvent(const std::shared_ptr<PluginEvent> event) override
    {
        (void)event;
    }

protected:
    int32_t outputBufferCapacity_;
    int32_t inputBufferCapacity_;
    std::ifstream inputFile_;
    std::ofstream outputFile_;
    std::shared_ptr<Meta> meta_ = nullptr;
    std::shared_ptr<CodecPlugin> decoder_ = nullptr;
};

}
}
}

#endif
