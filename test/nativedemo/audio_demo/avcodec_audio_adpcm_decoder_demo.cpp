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

#include <cstdlib>
#include <iostream>
#include "avcodec_audio_adpcm_decoder_demo.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace {
const string TEST_FILE_PATH = "/data/test/media/";
const vector<string> TEST_DECODER = {
    "OH.Media.Codec.Decoder.Audio.ADPCM.MS",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.QT",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.WAV",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.DK3",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.DK4",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.WS",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.SMJPEG",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.DAT4",
    "OH.Media.Codec.Decoder.Audio.ADPCM.MTAF",
    "OH.Media.Codec.Decoder.Audio.ADPCM.ADX",
    "OH.Media.Codec.Decoder.Audio.ADPCM.AFC",
    "OH.Media.Codec.Decoder.Audio.ADPCM.AICA",
    "OH.Media.Codec.Decoder.Audio.ADPCM.CT",
    "OH.Media.Codec.Decoder.Audio.ADPCM.DTK",
    "OH.Media.Codec.Decoder.Audio.ADPCM.G722",
    "OH.Media.Codec.Decoder.Audio.ADPCM.G726",
    "OH.Media.Codec.Decoder.Audio.ADPCM.G726LE",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.AMV",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.APC",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.ISS",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.OKI",
    "OH.Media.Codec.Decoder.Audio.ADPCM.IMA.RAD",
    "OH.Media.Codec.Decoder.Audio.ADPCM.PSX",
    "OH.Media.Codec.Decoder.Audio.ADPCM.SBPRO2",
    "OH.Media.Codec.Decoder.Audio.ADPCM.SBPRO3",
    "OH.Media.Codec.Decoder.Audio.ADPCM.SBPRO4",
    "OH.Media.Codec.Decoder.Audio.ADPCM.THP",
    "OH.Media.Codec.Decoder.Audio.ADPCM.THP.LE",
    "OH.Media.Codec.Decoder.Audio.ADPCM.XA",
    "OH.Media.Codec.Decoder.Audio.ADPCM.YAMAHA"
};
}

bool AudioAdpcmDecoderDemo::ReadBuffer(std::shared_ptr<AVBuffer> buffer)
{
    int64_t size;
    int64_t pts;
    inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_.eof() || inputFile_.gcount() == 0 || size == 0) {
        buffer->memory_->SetSize(1);
        buffer->flag_ = 1;
        cout << "Set EOS" << endl;
        decoder_->QueueInputBuffer(buffer);
        return false;
    }

    if (inputFile_.gcount() != sizeof(size)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }

    inputFile_.read(reinterpret_cast<char *>(&buffer->pts_), sizeof(buffer->pts_));
    if (inputFile_.gcount() != sizeof(pts)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }

    inputFile_.read(reinterpret_cast<char *>(buffer->memory_->GetAddr()), size);
    buffer->memory_->SetSize(size);
    if (inputFile_.gcount() != size) {
        cout << "Fatal: read buffer fail" << endl;
        return false;
    }
    cout << "read adpcm buffer size:" << size << endl;
    decoder_->QueueInputBuffer(buffer);

    return true;
}

Status AudioAdpcmDecoderDemo::ProcessDecoder()
{
    outputFile_.open(TEST_FILE_PATH + "adpcm_demo_out.pcm", ios::binary);
    if (!outputFile_.is_open()) {
        cout << "output file open fail" << endl;
        return Status::ERROR_UNKNOWN;
    }
    shared_ptr<Meta> config = make_shared<Meta>();
    decoder_->GetParameter(config);
    config->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(outputBufferCapacity_);
    config->Get<Tag::AUDIO_MAX_INPUT_SIZE>(inputBufferCapacity_);
    Status ret = Status::OK;
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto inBuffer = AVBuffer::CreateAVBuffer(avAllocator, inputBufferCapacity_);
    auto outBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputBufferCapacity_);
    bool isContinue = true;
    while (isContinue) {
        isContinue = ReadBuffer(inBuffer);
        Status outRet;
        do {
            outRet = decoder_->QueueOutputBuffer(outBuffer);
            if (outRet == Status::OK || outRet == Status::ERROR_AGAIN) {
                outputFile_.write(reinterpret_cast<char *>(outBuffer->memory_->GetAddr()),
                                  outBuffer->memory_->GetSize());
            }
        } while (outRet == Status::ERROR_AGAIN);
    }
    return ret;
}

bool AudioAdpcmDecoderDemo::InitDecoder(string name)
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(name);
    if (plugin == nullptr) {
        cout << "init decoder fail" << endl;
        return false;
    }
    decoder_ = reinterpret_pointer_cast<CodecPlugin>(plugin);
    decoder_->SetDataCallback(this);
    meta_ = make_shared<Meta>();
    return true;
}

void AudioAdpcmDecoderDemo::ReleaseDecoder()
{
    if (decoder_ != nullptr) {
        decoder_->Release();
        decoder_ = nullptr;
    }
}

void AudioAdpcmDecoderDemo::RunCase()
{
    cout << "choose a decoder name:" << endl;
    for (size_t i = 0; i < TEST_DECODER.size(); i++) {
        cout << i << ": " << TEST_DECODER[i] << endl;
    }
    string choose;
    (void)getline(cin, choose);
    size_t selected = atoi(choose.c_str());
    if (selected >= TEST_DECODER.size()) {
        cout << "not exist name" << endl;
        return;
    }
    if (!InitDecoder(TEST_DECODER[selected])) {
        return;
    }
    inputFile_.open(TEST_FILE_PATH + "adpcm_demo.dat", ios::binary);
    if (!inputFile_.is_open()) {
        cout << "input file open fail" << endl;
        return;
    }
    int32_t channel;
    int32_t sampleRate;
    int32_t extraDataSize;
    vector<uint8_t> extraData;
    inputFile_.read(reinterpret_cast<char *>(&channel), sizeof(channel));
    if (inputFile_.gcount() != sizeof(channel)) {
        cout << "Fatal: read channel fail" << endl;
        return;
    }
    inputFile_.read(reinterpret_cast<char *>(&sampleRate), sizeof(sampleRate));
    if (inputFile_.gcount() != sizeof(sampleRate)) {
        cout << "Fatal: read sampleRate fail" << endl;
        return;
    }
    inputFile_.read(reinterpret_cast<char *>(&extraDataSize), sizeof(extraDataSize));
    if (inputFile_.gcount() != sizeof(extraDataSize)) {
        cout << "Fatal: read extraDataSize fail" << endl;
        return;
    }
    if (extraDataSize > 0) {
        extraData.resize(extraDataSize);
        inputFile_.read(reinterpret_cast<char *>(extraData.data()), extraDataSize);
        if (inputFile_.gcount() != extraDataSize) {
            cout << "Fatal: read extraData fail" << endl;
            return;
        }
        meta_->Set<Tag::MEDIA_CODEC_CONFIG>(extraData);
    }
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(channel);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    decoder_->SetParameter(meta_);
    ProcessDecoder();
    ReleaseDecoder();
}
