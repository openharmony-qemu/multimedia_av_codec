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
#include <atomic>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "audio_muxer_demo.h"
#include "avmuxer_sample.h"
#include "avmuxer_mpeg4_plugin_mock.h"
#define FUZZ_PROJECT_NAME "audiomuxerhevc_fuzzer"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;

namespace OHOS {

void SetMpeg4MuxerParam(std::shared_ptr<Meta> &param)
{
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_HEVC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    param->Set<Tag::MEDIA_ENABLE_MOOV_FRONT>(1); // moov front
    param->Set<Tag::VIDEO_WIDTH>(480);  // 480
    param->Set<Tag::VIDEO_HEIGHT>(720);  // 720
    param->Set<Tag::VIDEO_FRAME_RATE>(60.0); // 60.0 fps
    param->Set<Tag::VIDEO_DELAY>(0);

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(2);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("uri");
    param->SetData("latitude", 10.0);
    param->SetData("longitude", 20.0);
    param->SetData("altitude", 30.0);
    param->SetData(OH_MD_KEY_COMMENT, "fuzz_test_comment");
    param->SetData(OH_MD_KEY_TITLE, "fuzz_test_title");
    param->SetData(OH_MD_KEY_ARTIST, "fuzz_test_artist");
    param->SetData(OH_MD_KEY_LYRICS, "fuzz_test_artist_lyrics");
    param->SetData(OH_MD_KEY_GENRE, "fuzz_test_artist_genre");
    param->SetData("com.openharmony.version", 5); // 5 test version
    param->SetData("com.openharmony.model", "LNA-AL00");
    param->SetData("com.openharmony.capture.fps", 30.00f); // 30.00f test capture fps
    param->SetData("com.openharmony.capture.test.buffer", binVec);
}

void AudioMpeg4MuxerFuzzTest(const uint8_t *data, size_t size)
{
    int32_t trackId = -1;
    int32_t fd = open("AudioMpeg4MuxerFuzzTest.mp4", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {return;}
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd, AV_OUTPUT_FORMAT_MPEG_4);
    if (avmuxerMock == nullptr) {return;}
    AVMuxerMpeg4PluginMock* avmuxer = reinterpret_cast<AVMuxerMpeg4PluginMock*>(avmuxerMock.get());

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    SetMpeg4MuxerParam(param);
    avmuxer->SetParameter(param);

    avmuxer->AddTrack(trackId, param);

    param->SetData(Tag::MIME_TYPE, Plugins::MimeType::TIMED_METADATA);
    param->SetData("timed_metadata_key", 1);
    param->SetData("timed_metadata_track_id", trackId);
    avmuxer->AddTrack(trackId, param);
    avmuxer->Start();
    if (size > 0) {
        OH_AVCodecBufferAttr info;
        info.flags = 0;
        info.size = size;
        info.pts = 0;
        info.offset = 0;
        OH_AVBuffer *buffer = OH_AVBuffer_Create(size);
        if (memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, data, info.size) != 0) {
            std::cout<<"memcpy failed!"<<std::endl;
        }
        OH_AVBuffer_SetBufferAttr(buffer, &info);
        avmuxer->WriteSampleBuffer(trackId, buffer);
        OH_AVBuffer_Destroy(buffer);
    }
    avmuxer->Stop();
    close(fd);
}

bool AudioMuxerHEVCFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ hevc
    AVMuxerDemo* aMuxerBufferDemo = new AVMuxerDemo();
    aMuxerBufferDemo->InitFile("h265");
    auto res = aMuxerBufferDemo->RunCase(data, size);
    delete aMuxerBufferDemo;
    return res;
}

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioMuxerHEVCFuzzTest(data, size);
    OHOS::AudioMpeg4MuxerFuzzTest(data, size);
    return 0;
}