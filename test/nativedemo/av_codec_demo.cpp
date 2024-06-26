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

#include <climits>
#include <iostream>
#include <vector>
#include "avmuxer_demo_runner.h"
#include "avdemuxer_demo_runner.h"
#include "avcodec_audio_decoder_inner_demo.h"
#include "avcodec_audio_encoder_inner_demo.h"
#include "avcodec_audio_decoder_demo.h"
#include "avcodec_audio_aac_encoder_demo.h"
#include "avcodec_audio_avbuffer_aac_encoder_demo.h"
#include "avcodec_audio_avbuffer_amrnb_encoder_demo.h"
#include "avcodec_audio_avbuffer_amrwb_encoder_demo.h"
#include "avcodec_audio_avbuffer_decoder_inner_demo.h"
#include "avcodec_audio_flac_encoder_demo.h"
#include "avcodec_audio_avbuffer_flac_encoder_demo.h"
#include "avcodec_audio_avbuffer_g711mu_encoder_demo.h"
#include "avcodec_audio_avbuffer_lbvc_decoder_inner_demo.h"
#include "avcodec_audio_avbuffer_lbvc_encoder_inner_demo.h"
#include "avcodec_audio_avbuffer_mp3_encoder_demo.h"
#include "avcodec_audio_opus_encoder_demo.h"
#include "avcodec_audio_g711mu_encoder_demo.h"
#include "codeclist_demo.h"
#include "avcodec_video_decoder_demo.h"
#include "avcodec_video_decoder_inner_demo.h"
#include "avcodec_audio_avbuffer_decoder_demo.h"
#include "avcodec_e2e_demo.h"
#include "avcodec_e2e_demo_api10.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioDemo;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;
using namespace OHOS::MediaAVCodec::AudioFlacDemo;
using namespace OHOS::MediaAVCodec::AudioFlacEncDemo;  // AudioEncoderBufferDemo
using namespace OHOS::MediaAVCodec::AudioOpusDemo;
using namespace OHOS::MediaAVCodec::AudioG711muDemo;
using namespace OHOS::MediaAVCodec::AudioAvbufferG711muDemo;
using namespace OHOS::MediaAVCodec::AudioAvbufferMp3EncoderDemo;
using namespace OHOS::MediaAVCodec::AudioAacDemo;
using namespace OHOS::MediaAVCodec::AudioAacEncDemo;
using namespace OHOS::MediaAVCodec::InnerAudioDemo;
using namespace OHOS::MediaAVCodec::VideoDemo;
using namespace OHOS::MediaAVCodec::InnerVideoDemo;
using namespace OHOS::MediaAVCodec::E2EDemo;
using namespace OHOS::MediaAVCodec::InnerAudioDecoderLbvcDemo;
using namespace OHOS::MediaAVCodec::InnerAudioEncoderLbvcDemo;
using namespace OHOS::MediaAVCodec::AudioAvbufferAmrNbDemo;
using namespace OHOS::MediaAVCodec::AudioAvbufferAmrWbDemo;
using namespace std;

static int RunAudioDecoder()
{
    cout << "Please select number for format (default AAC Decoder): " << endl;
    cout << "0: AAC" << endl;
    cout << "1: FLAC" << endl;
    cout << "2: MP3" << endl;
    cout << "3: VORBIS" << endl;
    cout << "4: AMRNB" << endl;
    cout << "5: AMRWB" << endl;
    cout << "6: OPUS" << endl;
    cout << "7: G711MU" << endl;
    string mode;
    AudioFormatType audioFormatType = TYPE_AAC;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        audioFormatType = TYPE_AAC;
    } else if (mode == "1") {
        audioFormatType = TYPE_FLAC;
    } else if (mode == "2") {
        audioFormatType = TYPE_MP3;
    } else if (mode == "3") {
        audioFormatType = TYPE_VORBIS;
    } else if (mode == "4") {
        audioFormatType = TYPE_AMRNB;
    } else if (mode == "5") {
        audioFormatType = TYPE_AMRWB;
    } else if (mode == "6") {
        audioFormatType = TYPE_OPUS;
    } else if (mode == "7") {
        audioFormatType = TYPE_G711MU;
    } else {
        cout << "no that selection" << endl;
        return 0;
    }
    auto audioDec = std::make_unique<ADecDemo>();
    audioDec->RunCase(audioFormatType);
    cout << "demo audio decoder end" << endl;
    return 0;
}

static int RunAudioAVBufferDecoder()
{
    cout << "Please select number for format (default AAC Decoder): " << endl;
    cout << "0: AAC" << endl;
    cout << "1: FLAC" << endl;
    cout << "2: MP3" << endl;
    cout << "3: VORBIS" << endl;
    cout << "4: AMR-NB" << endl;
    cout << "5: AMR-WB" << endl;
    cout << "6: G711MU" << endl;
    cout << "7: APE" << endl;

    string mode;
    AudioBufferFormatType audioFormatType = AudioBufferFormatType::TYPE_AAC;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        audioFormatType = AudioBufferFormatType::TYPE_AAC;
    } else if (mode == "1") {
        audioFormatType = AudioBufferFormatType::TYPE_FLAC;
    } else if (mode == "2") {
        audioFormatType = AudioBufferFormatType::TYPE_MP3;
    } else if (mode == "3") {
        audioFormatType = AudioBufferFormatType::TYPE_VORBIS;
    } else if (mode == "4") {
        audioFormatType = AudioBufferFormatType::TYPE_AMRNB;
    } else if (mode == "5") {
        audioFormatType = AudioBufferFormatType::TYPE_AMRWB;
    } else if (mode == "6") {
        audioFormatType = AudioBufferFormatType::TYPE_G711MU;
    } else if (mode == "7") {
        audioFormatType = AudioBufferFormatType::TYPE_APE;
    } else {
        cout << "no that selection" << endl;
        return 0;
    }
    auto audioDec = std::make_unique<ADecBufferDemo>();
    audioDec->RunCase(audioFormatType);
    cout << "demo audio decoder end" << endl;
    return 0;
}

static int RunAudioAVBufferDecoderDrm()
{
    cout << "RunAudioAVBufferDecoderDrm" << endl;
    cout << "Please select number for format (default AAC Decoder): " << endl;
    cout << "0: AAC" << endl;
    cout << "1: FLAC" << endl;
    cout << "2: MP3" << endl;

    string mode;
    AudioBufferFormatType audioFormatType = AudioBufferFormatType::TYPE_AAC;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        audioFormatType = AudioBufferFormatType::TYPE_AAC;
    } else if (mode == "1") {
        audioFormatType = AudioBufferFormatType::TYPE_FLAC;
    } else if (mode == "2") {
        audioFormatType = AudioBufferFormatType::TYPE_MP3;
    } else {
        cout << "no that selection" << endl;
        return 0;
    }
    auto audioDec = std::make_unique<ADecBufferDemo>();
    audioDec->RunDrmCase(audioFormatType);
    cout << "demo audio decoder end" << endl;
    return 0;
}

static int RunAudioEncoder()
{
    cout << "Please select number for format (default AAC Encoder): " << endl;
    cout << "0: AAC\n" << "1: FLAC" << endl;
    cout << "2: OPUS" << endl;
    cout << "3: G711MU" << endl;
    cout << "4: AAC-API11" << endl;
    cout << "5: FLAC-API11" << endl;
    cout << "6: G711MU-API11" << endl;
    cout << "7: AMR-NB-API11" << endl;
    cout << "8: AMR-WB-API11" << endl;
    cout << "9: MP3-API11" << endl;
    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        auto audioEnc = std::make_unique<AEncAacDemo>();
        audioEnc->RunCase();
    } else if (mode == "1") {
        auto audioEnc = std::make_unique<AEncFlacDemo>();
        audioEnc->RunCase();
    } else if (mode == "2") {
        auto audioEnc = std::make_unique<AEncOpusDemo>();
        audioEnc->RunCase();
    } else if (mode == "3") {
        auto audioEnc = std::make_unique<AEncG711muDemo>();
        audioEnc->RunCase();
    } else if (mode == "4") {
        auto audioEnc = std::make_unique<AudioBufferAacEncDemo>();
        audioEnc->RunCase();
    } else if (mode == "5") {
        auto audioEnc = std::make_unique<AudioBufferFlacEncDemo>();
        audioEnc->RunCase();
    } else if (mode == "6") {
        auto audioEnc = std::make_unique<AEncAvbufferG711muDemo>();
        audioEnc->RunCase();
    } else if (mode == "7") {
        auto audioEnc = std::make_unique<AEncAvbufferAmrNbDemo>();
        audioEnc->RunCase();
    } else if (mode == "8") {
        auto audioEnc = std::make_unique<AEncAvbufferAmrWbDemo>();
        audioEnc->RunCase();
    } else if (mode == "9") {
        auto audioEnc = std::make_unique<AEncAvbufferMp3Demo>();
        audioEnc->RunCase();
    } else {
        cout << "no that selection" << endl;
        return 0;
    }
    cout << "demo audio encoder end" << endl;
    return 0;
}

static int RunAudioInnerDecoder()
{
    cout << "Please select number for format (default AAC Decoder): " << endl;
    cout << "0: AAC" << endl;
    cout << "1: FLAC" << endl;
    cout << "2: MP3" << endl;
    cout << "3: VORBIS" << endl;
    cout << "4: DecoderInner-API11" << endl;
    cout << "5: LBVC" << endl;
    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        auto audioDec = std::make_unique<ADecInnerDemo>();
        audioDec->RunCase();
    } else if (mode == "1") {
        auto audioDec = std::make_unique<ADecInnerDemo>();
        audioDec->RunCase();
    } else if (mode == "2") {
        auto audioDec = std::make_unique<ADecInnerDemo>();
        audioDec->RunCase();
    } else if (mode == "3") {
        auto audioDec = std::make_unique<ADecInnerDemo>();
        audioDec->RunCase();
    } else if (mode == "4") {
        auto audioDec = std::make_unique<AudioDecInnerAvBufferDemo>();
        audioDec->RunCase();
    } else if (mode == "5") {
        auto audioDec = std::make_unique<AudioDecInnerAvBufferLbvcDemo>();
        audioDec->RunCase();
    } else {
        cout << "no that selection" << endl;
        return 0;
    }
    cout << "demo audio decoder end" << endl;
    return 0;
}

static int RunAudioInnerEncoder()
{
    cout << "Please select number for format (default AAC Encoder): " << endl;
    cout << "0: AAC" << endl;
    cout << "1: FLAC" << endl;
    cout << "2: LBVC" << endl;
    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        auto audioEnc = std::make_unique<AEnInnerDemo>();
        audioEnc->RunCase();
    } else if (mode == "1") {
        auto audioEnc = std::make_unique<AEnInnerDemo>();
        audioEnc->RunCase();
    } else if (mode == "2") {
        auto audioEnc = std::make_unique<AudioEncInnerAvBufferLbvcDemo>();
        audioEnc->RunCase();
    } else {
        cout << "no that selection" << endl;
        return 0;
    }
    cout << "demo audio encoder end" << endl;
    return 0;
}

static int RunCodecList()
{
    auto codecList = std::make_unique<CodecListDemo>();
    if (codecList == nullptr) {
        cout << "codec list is null" << endl;
        return 0;
    }
    codecList->RunCase();
    cout << "codec list end" << endl;
    return 0;
}

static int RunVideoDecoder()
{
    cout << "Please select number for output mode (default buffer mode): " << endl;
    cout << "0: buffer" << endl;
    cout << "1: surface file" << endl;
    cout << "2: surface render" << endl;

    string mode;
    (void)getline(cin, mode);
    if (mode != "0" && mode != "1" && mode != "2") {
        cout << "parameter invalid" << endl;
        return 0;
    }

    auto videoDec = std::make_unique<VDecDemo>();
    if (videoDec == nullptr) {
        cout << "video decoder is null" << endl;
        return 0;
    }
    videoDec->RunCase(mode);
    cout << "demo video decoder end" << endl;
    return 0;
}

static int RunVideoDecoderDrm()
{
    cout << "RunVideoDecoderDrm: " << endl;
    cout << "Please select number for output mode (default buffer mode): " << endl;
    cout << "0: buffer" << endl;
    cout << "1: surface file" << endl;
    cout << "2: surface render" << endl;

    string mode;
    (void)getline(cin, mode);
    if (mode != "0" && mode != "1" && mode != "2") {
        cout << "parameter invalid" << endl;
        return 0;
    }

    auto videoDec = std::make_unique<VDecDemo>();
    if (videoDec == nullptr) {
        cout << "video decoder is null" << endl;
        return 0;
    }
    videoDec->RunDrmCase();
    cout << "demo video decoder end" << endl;
    return 0;
}

static int RunVideoInnerDecoder()
{
    cout << "Please select number for output mode (default buffer mode): " << endl;
    cout << "0: buffer" << endl;
    cout << "1: surface file" << endl;
    cout << "2: surface render" << endl;

    string mode;
    (void)getline(cin, mode);
    if (mode != "0" && mode != "1" && mode != "2") {
        cout << "parameter invalid" << endl;
        return 0;
    }

    auto videoDec = std::make_unique<VDecInnerDemo>();
    if (videoDec == nullptr) {
        cout << "video decoder is null" << endl;
        return 0;
    }
    videoDec->RunCase(mode);
    cout << "demo video decoder end" << endl;
    return 0;
}

static int RunE2EDemo()
{
    cout << "Please select number for api version (default api11): " << endl;
    cout << "0: api11" << endl;
    cout << "1: api10" << endl;

    string mode;
    (void)getline(cin, mode);
    if (mode != "0" && mode != "1") {
        cout << "parameter invalid" << endl;
        return 0;
    }
    const char *path = "/data/test/media/input.mp4";
    if (mode == "0") {
        auto e2eDemo = std::make_unique<AVCodecE2EDemo>(path);
        if (e2eDemo == nullptr) {
            cout << "e2eDemo is null" << endl;
            return 0;
        }
        e2eDemo->Configure();
        e2eDemo->Start();
        e2eDemo->WaitForEOS();
        e2eDemo->Stop();
    } else if (mode == "1") {
        auto e2eDemo = std::make_unique<AVCodecE2EDemoAPI10>(path);
        if (e2eDemo == nullptr) {
            cout << "e2eDemo is null" << endl;
            return 0;
        }
        e2eDemo->Configure();
        e2eDemo->Start();
        e2eDemo->WaitForEOS();
        e2eDemo->Stop();
    }
    cout << "e2eDemo end" << endl;
    return 0;
}

static void OptionPrint()
{
    cout << "Please select a demo scenario number(default Audio Decoder): " << endl;
    cout << "0:Audio Decoder" << endl;
    cout << "1:Audio Encoder" << endl;
    cout << "2:Audio Inner Decoder" << endl;
    cout << "3:Audio Inner Encoder" << endl;
    cout << "4:muxer demo" << endl;
    cout << "6:codeclist" << endl;
    cout << "7:Video Decoder" << endl;
    cout << "8:Video Inner Decoder" << endl;
    cout << "9:demuxer demo" << endl;
    cout << "10:Audio AVBuffer Decoder" << endl;
    cout << "11:Video Decoder DRM" << endl;
    cout << "12:E2E demo" << endl;
    cout << "13:Audio AVBuffer Decoder with DRM" << endl;
}

int main()
{
    OptionPrint();
    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        (void)RunAudioDecoder();
    } else if (mode == "1") {
        (void)RunAudioEncoder();
    } else if (mode == "2") {
        (void)RunAudioInnerDecoder();
    } else if (mode == "3") {
        (void)RunAudioInnerEncoder();
    } else if (mode == "4") {
        (void)AVMuxerDemoCase();
    } else if (mode == "6") {
        (void)RunCodecList();
    } else if (mode == "7") {
        (void)RunVideoDecoder();
    } else if (mode == "8") {
        (void)RunVideoInnerDecoder();
    } else if (mode == "9") {
        (void)AVSourceDemuxerDemoCase();
    } else if (mode == "10") {
        (void)RunAudioAVBufferDecoder();
    } else if (mode == "11") {
        (void)RunVideoDecoderDrm();
    } else if (mode == "12") {
        (void)RunE2EDemo();
    } else if (mode == "13") {
        (void)RunAudioAVBufferDecoderDrm();
    } else {
        cout << "no that selection" << endl;
    }
    return 0;
}
