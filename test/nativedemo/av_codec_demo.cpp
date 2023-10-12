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
#include "avcodec_audio_flac_encoder_demo.h"
#include "codeclist_demo.h"
#include "avcodec_video_decoder_demo.h"
#include "avcodec_video_decoder_inner_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioDemo;
using namespace OHOS::MediaAVCodec::AudioFlacDemo;
using namespace OHOS::MediaAVCodec::AudioAacDemo;
using namespace OHOS::MediaAVCodec::InnerAudioDemo;
using namespace OHOS::MediaAVCodec::VideoDemo;
using namespace OHOS::MediaAVCodec::InnerVideoDemo;
using namespace std;

static int RunAudioDecoder()
{
    cout << "Please select number for format (default AAC Decoder): " << endl;
    cout << "0: AAC" << endl;
    cout << "1: FLAC" << endl;
    cout << "2: MP3" << endl;
    cout << "3: VORBIS" << endl;
    cout << "4: AMRNB" << endl;
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
    } else {
        cout << "no that selection" << endl;
        return 0;
    }
    auto audioDec = std::make_unique<ADecDemo>();
    audioDec->RunCase(audioFormatType);
    cout << "demo audio decoder end" << endl;
    return 0;
}

static int RunAudioEncoder()
{
    cout << "Please select number for format (default AAC Encoder): " << endl;
    cout << "0: AAC" << endl;
    cout << "1: FLAC" << endl;

    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        auto audioEnc = std::make_unique<AEncAacDemo>();
        audioEnc->RunCase();
    } else if (mode == "1") {
        auto audioEnc = std::make_unique<AEncFlacDemo>();
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
    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        auto audioEnc = std::make_unique<AEnInnerDemo>();
        audioEnc->RunCase();
    } else if (mode == "1") {
        auto audioEnc = std::make_unique<AEnInnerDemo>();
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
        (void)AvmuxerDemoCase();
    } else if (mode == "6") {
        (void)RunCodecList();
    } else if (mode == "7") {
        (void)RunVideoDecoder();
    } else if (mode == "8") {
        (void)RunVideoInnerDecoder();
    } else if (mode == "9") {
        (void)AVSourceDemuxerDemoCase();
    } else {
        cout << "no that selection" << endl;
    }
    return 0;
}
