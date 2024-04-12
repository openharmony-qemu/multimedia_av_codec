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
#include "avmuxer_demo_base.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "avcodec_errors.h"

namespace {
    constexpr int MODE_ZERO = 0;
    constexpr int MODE_ONE = 1;
    constexpr int MODE_TWO = 2;
    constexpr int MODE_THREE = 3;
    constexpr int MODE_FOUR = 4;
    constexpr int CONFIG_BUFFER_SZIE = 0x1FFF;
}

namespace OHOS {
namespace MediaAVCodec {
const AudioTrackParam *AVMuxerDemoBase::audioParams_ = nullptr;
const VideoTrackParam *AVMuxerDemoBase::videoParams_ = nullptr;
const VideoTrackParam *AVMuxerDemoBase::coverParams_ = nullptr;
std::string AVMuxerDemoBase::videoType_ = std::string("");
std::string AVMuxerDemoBase::audioType_ = std::string("");
std::string AVMuxerDemoBase::coverType_ = std::string("");
std::string AVMuxerDemoBase::format_ = std::string("");
Plugins::OutputFormat AVMuxerDemoBase::outputFormat_ = Plugins::OutputFormat::DEFAULT;
bool AVMuxerDemoBase::hasSetMode_ = false;
using namespace OHOS::Media;

AVMuxerDemoBase::AVMuxerDemoBase()
{
}

std::shared_ptr<std::ifstream> OpenFile(const std::string &filePath)
{
    auto file = std::make_shared<std::ifstream>();
    file->open(filePath, std::ios::in | std::ios::binary);
    if (file->is_open()) {
        return file;
    }

    return nullptr;
}

void AVMuxerDemoBase::SelectFormatMode()
{
    int num;
    std::cout<<"\nplease select muxer type: 0.mp4 1.m4a 2.amr 3.mp3"<<std::endl;
    std::cin>>num;
    switch (num) {
        case MODE_ZERO:
            format_ = "mp4";
            outputFormat_ = Plugins::OutputFormat::MPEG_4;
            break;
        case MODE_ONE:
            format_ = "m4a";
            outputFormat_ = Plugins::OutputFormat::M4A;
            break;
        case MODE_TWO:
            format_ = "amr";
            outputFormat_ = Plugins::OutputFormat::AMR;
            break;
        case MODE_THREE:
            format_ = "mp3";
            outputFormat_ = Plugins::OutputFormat::MP3;
            break;
        default:
            format_ = "mp4";
            outputFormat_ = Plugins::OutputFormat::MPEG_4;
            break;
    }
}

void AVMuxerDemoBase::SelectAudioMode()
{
    int num;
    std::cout<<"\nplease select audio file: 0.noAudio 1.aac 2.mpeg 3.amrnb 4.amrwb"<<std::endl;
    std::cin>>num;
    switch (num) {
        case MODE_ZERO:
            audioType_ = "noAudio";
            audioParams_ = nullptr;
            break;
        case MODE_ONE:
            audioType_ = "aac";
            audioParams_ = &g_audioAacPar;
            break;
        case MODE_TWO:
            audioType_ = "mpeg";
            audioParams_ = &g_audioMpegPar;
            break;
        case MODE_THREE:
            audioType_ = "amr";
            audioParams_ = &g_audioAmrNbPar;
            break;
        case MODE_FOUR:
            audioType_ = "amr";
            audioParams_ = &g_audioAmrWbPar;
            break;
        default:
            audioType_ = "noAudio";
            audioParams_ = nullptr;
            std::cout<<"do not support audio type index: "<<num<<", set to noAudio"<<std::endl;
            break;
    }
}

void AVMuxerDemoBase::SelectVideoMode()
{
    int num;
    std::cout<<"please select video file:0.noVideo 1.h264 2.mpeg4 3.h265 4.hdr vivid"<<std::endl;
    std::cin>>num;
    switch (num) {
        case MODE_ZERO:
            videoType_ = "noVideo";
            videoParams_ = nullptr;
            break;
        case MODE_ONE:
            videoType_ = "h264";
            videoParams_ = &g_videoH264Par;
            break;
        case MODE_TWO:
            videoType_ = "mpeg4";
            videoParams_ = &g_videoMpeg4Par;
            break;
        case MODE_THREE:
            videoType_ = "h265";
            videoParams_ = &g_videoH265Par;
            break;
        case MODE_FOUR:
            videoType_ = "hdr-vivid";
            videoParams_ = &g_videoHdrPar;
            break;
        default:
            videoType_ = "noVideo";
            videoParams_ = nullptr;
            std::cout<<"do not support video type index: "<<", set to noVideo"<<num<<std::endl;
            break;
    }
}

void AVMuxerDemoBase::SelectCoverMode()
{
    int num;
    std::cout<<"please select cover file:0.NoCover 1.jpg 2.png 3.bmp"<<std::endl;
    std::cin>>num;
    switch (num) {
        case MODE_ZERO:
            coverType_ = "noCover";
            coverParams_ = nullptr;
            break;
        case MODE_ONE:
            coverType_ = "jpg";
            coverParams_ = &g_jpegCoverPar;
            break;
        case MODE_TWO:
            coverType_ = "png";
            coverParams_ = &g_pngCoverPar;
            break;
        case MODE_THREE:
            coverType_ = "bmp";
            coverParams_ = &g_bmpCoverPar;
            break;
        default:
            coverType_ = "noCover";
            coverParams_ = nullptr;
            std::cout<<"do not support cover type index: "<<", set to noCover"<<num<<std::endl;
            break;
    }
}

int AVMuxerDemoBase::SelectMode()
{
    if (hasSetMode_) {
        return 0;
    }
    SelectFormatMode();
    SelectAudioMode();
    SelectVideoMode();
    SelectCoverMode();

    hasSetMode_ = true;
    return 0;
}

int AVMuxerDemoBase::SelectModeAndOpenFile()
{
    if (SelectMode() != 0) {
        return -1;
    }

    if (audioParams_ != nullptr) {
        audioFile_ = OpenFile(audioParams_->fileName);
        if (audioFile_ == nullptr) {
            std::cout<<"open audio file failed! file name:"<<audioParams_->fileName<<std::endl;
            return -1;
        }
        std::cout<<"open audio file success! file name:"<<audioParams_->fileName<<std::endl;
    }

    if (videoParams_ != nullptr) {
        videoFile_ = OpenFile(videoParams_->fileName);
        if (videoFile_ == nullptr) {
            std::cout<<"open video file failed! file name:"<<videoParams_->fileName<<std::endl;
            Reset();
            return -1;
        }
        std::cout<<"video file success! file name:"<<videoParams_->fileName<<std::endl;
    }

    if (coverParams_ != nullptr) {
        coverFile_ = OpenFile(coverParams_->fileName);
        if (coverFile_ == nullptr) {
            std::cout<<"open cover file failed! file name:"<<coverParams_->fileName<<std::endl;
            Reset();
            return -1;
        }
        std::cout<<"cover file success! file name:"<<coverParams_->fileName<<std::endl;
    }
    return 0;
}

void AVMuxerDemoBase::Reset()
{
    if (outFd_ > 0) {
        close(outFd_);
        outFd_ = -1;
    }
    if (audioFile_ != nullptr) {
        audioFile_->close();
        audioFile_ = nullptr;
    }
    if (videoFile_ != nullptr) {
        videoFile_->close();
        videoFile_ = nullptr;
    }
    if (coverFile_ != nullptr) {
        coverFile_->close();
        coverFile_ = nullptr;
    }
}

void AVMuxerDemoBase::RunCase()
{
    if (SelectModeAndOpenFile() != 0) {
        return;
    }

    DoRunMuxer();

    Reset();
}

void AVMuxerDemoBase::RunMultiThreadCase()
{
    std::cout<<"==== start AVMuxerDemoBase::RunMultiThreadCase ==="<<std::endl;
    if (SelectModeAndOpenFile() != 0) {
        return;
    }

    DoRunMultiThreadCase();

    Reset();
}

void AVMuxerDemoBase::WriteSingleTrackSample(uint32_t trackId, std::shared_ptr<std::ifstream> file)
{
    if (file == nullptr) {
        std::cout<<"AVMuxerDemoBase::WriteTrackSample file is nullptr"<<std::endl;
        return;
    }
    std::shared_ptr<AVBuffer> buffer = nullptr;
    bool ret = ReadSampleDataInfo(file, buffer);
    while (ret) {
        if (DoWriteSample(trackId, buffer) != AVCS_ERR_OK) {
            std::cout<<"WriteSample failed"<<std::endl;
            break;
        }
        ret = ReadSampleDataInfo(file, buffer);
    }
}

void AVMuxerDemoBase::WriteSingleTrackSampleByBufferQueue(sptr<AVBufferQueueProducer> bufferQueue,
    std::shared_ptr<std::ifstream> file)
{
    if (file == nullptr) {
        std::cout<<"AVMuxerDemoBase::WriteSingleTrackSampleByBufferQueue file is nullptr"<<std::endl;
        return;
    }
    std::shared_ptr<AVBuffer> buffer = nullptr;
    if (bufferQueue == nullptr) {
        std::cout<<"AVMuxerDemoBase::WriteSingleTrackSampleByBufferQueue buffer queue is nullptr"<<std::endl;
        return;
    }
    bool ret = ReadSampleDataInfoByBufferQueue(file, buffer, bufferQueue);
    while (ret) {
        if (bufferQueue->PushBuffer(buffer, true) != Status::OK) {
            std::cout<<"BufferQueue PushBuffer failed"<<std::endl;
            break;
        }
        ret = ReadSampleDataInfoByBufferQueue(file, buffer, bufferQueue);
    }
}

bool AVMuxerDemoBase::ReadSampleDataInfo(std::shared_ptr<std::ifstream> file,
    std::shared_ptr<AVBuffer> &buffer)
{
    int64_t pts = 0;
    uint32_t flags = 0;
    int32_t size = 0;
    if (file->eof()) {
        return false;
    }
    file->read(reinterpret_cast<char*>(&pts), sizeof(pts));

    if (file->eof()) {
        return false;
    }
    file->read(reinterpret_cast<char*>(&flags), sizeof(flags));

    if (file->eof()) {
        return false;
    }
    file->read(reinterpret_cast<char*>(&size), sizeof(size));

    if (file->eof()) {
        return false;
    }
    if (buffer == nullptr || buffer->memory_ == nullptr || buffer->memory_->GetCapacity() < size) {
        auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        buffer = AVBuffer::CreateAVBuffer(alloc, size);
    }
    file->read(reinterpret_cast<char*>(buffer->memory_->GetAddr()), size);
    buffer->pts_ = pts;
    buffer->flag_ = flags;
    buffer->memory_->SetSize(size);
    return true;
}

bool AVMuxerDemoBase::ReadSampleDataInfoByBufferQueue(std::shared_ptr<std::ifstream> file,
    std::shared_ptr<AVBuffer> &buffer, sptr<AVBufferQueueProducer> bufferQueue)
{
    int64_t pts = 0;
    uint32_t flags = 0;
    int32_t size = 0;
    if (file->eof()) {
        return false;
    }
    file->read(reinterpret_cast<char*>(&pts), sizeof(pts));

    if (file->eof()) {
        return false;
    }
    file->read(reinterpret_cast<char*>(&flags), sizeof(flags));

    if (file->eof()) {
        return false;
    }
    file->read(reinterpret_cast<char*>(&size), sizeof(size));

    if (file->eof()) {
        return false;
    }
    if (bufferQueue == nullptr) {
        return false;
    }
    AVBufferConfig config;
    config.size = size;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    bufferQueue->RequestBuffer(buffer, config, -1);
    file->read(reinterpret_cast<char*>(buffer->memory_->GetAddr()), size);
    buffer->pts_ = pts;
    buffer->flag_ = flags;
    buffer->memory_->SetSize(size);
    return true;
}

void AVMuxerDemoBase::WriteAvTrackSample()
{
    if (audioFile_ == nullptr || videoFile_ == nullptr) {
        return;
    }
    std::shared_ptr<AVBuffer> audioBuffer = nullptr;
    std::shared_ptr<AVBuffer> videoBuffer = nullptr;
    bool audioRet = ReadSampleDataInfo(audioFile_, audioBuffer);
    bool videoRet = ReadSampleDataInfo(videoFile_, videoBuffer);
    bool isOver = false;
    while (!isOver && (audioRet || videoRet)) {
        int ret = AVCS_ERR_OK;
        if (audioRet && videoRet && audioBuffer->pts_ <= videoBuffer->pts_) {
            ret = DoWriteSample(audioTrackId_, audioBuffer);
            audioRet = ReadSampleDataInfo(audioFile_, audioBuffer);
        } else if (audioRet && videoRet) {
            ret = DoWriteSample(videoTrackId_, videoBuffer);
            videoRet = ReadSampleDataInfo(videoFile_, videoBuffer);
        } else if (audioRet) {
            ret = DoWriteSample(audioTrackId_, audioBuffer);
            isOver = true;
        } else {
            ret = DoWriteSample(videoTrackId_, videoBuffer);
            isOver = true;
        }
        if (ret != AVCS_ERR_OK) {
            std::cout<<"WriteSample failed"<<std::endl;
            break;
        }
    }
}

void AVMuxerDemoBase::WriteAvTrackSampleByBufferQueue()
{
    if (audioFile_ == nullptr || videoFile_ == nullptr) {
        std::cout<<"WriteAvTrackSampleByBufferQueue file is null"<<std::endl;
        return;
    }
    if (audioBufferQueue_ == nullptr || videoBufferQueue_ == nullptr) {
        std::cout<<"WriteAvTrackSampleByBufferQueue buffer queue is null"<<std::endl;
        return;
    }
    std::shared_ptr<AVBuffer> audioBuffer = nullptr;
    std::shared_ptr<AVBuffer> videoBuffer = nullptr;
    bool audioRet = ReadSampleDataInfoByBufferQueue(audioFile_, audioBuffer, audioBufferQueue_);
    bool videoRet = ReadSampleDataInfoByBufferQueue(videoFile_, videoBuffer, videoBufferQueue_);
    bool isOver = false;
    Status ret = Status::OK;
    while (!isOver && (audioRet || videoRet)) {
        if (audioRet && videoRet && audioBuffer->pts_ <= videoBuffer->pts_) {
            ret = audioBufferQueue_->PushBuffer(audioBuffer, true);
            audioRet = ReadSampleDataInfoByBufferQueue(audioFile_, audioBuffer, audioBufferQueue_);
        } else if (audioRet && videoRet) {
            ret = videoBufferQueue_->PushBuffer(videoBuffer, true);
            videoRet = ReadSampleDataInfoByBufferQueue(videoFile_, videoBuffer, videoBufferQueue_);
        } else if (audioRet) {
            ret = audioBufferQueue_->PushBuffer(audioBuffer, true);
            isOver = true;
        } else {
            ret = videoBufferQueue_->PushBuffer(videoBuffer, true);
            isOver = true;
        }
        if (ret != Status::OK) {
            std::cout<<"BufferQueue PushBuffer failed"<<std::endl;
            break;
        }
    }
}

void AVMuxerDemoBase::WriteTrackSample()
{
    if (audioFile_ != nullptr && videoFile_ != nullptr && audioTrackId_ >= 0 && videoTrackId_ >= 0) {
        std::cout<<"AVMuxerDemoBase::WriteTrackSample write AUDIO and VIDEO sample"<<std::endl;
        std::cout<<"audio trackId:"<<audioTrackId_<<" video trackId:"<<videoTrackId_<<std::endl;
        WriteAvTrackSample();
    } else if (audioFile_ != nullptr && audioTrackId_ >= 0) {
        std::cout<<"AVMuxerDemoBase::WriteTrackSample write AUDIO sample"<<std::endl;
        WriteSingleTrackSample(audioTrackId_, audioFile_);
    } else if (videoFile_ != nullptr && videoTrackId_ >= 0) {
        std::cout<<"AVMuxerDemoBase::WriteTrackSample write VIDEO sample"<<std::endl;
        WriteSingleTrackSample(videoTrackId_, videoFile_);
    } else {
        std::cout<<"AVMuxerDemoBase::WriteTrackSample don't write AUDIO and VIDEO track!!"<<std::endl;
    }
}

void AVMuxerDemoBase::WriteTrackSampleByBufferQueue()
{
    if (audioFile_ != nullptr && videoFile_ != nullptr && audioTrackId_ >= 0 && videoTrackId_ >= 0) {
        std::cout<<"AVMuxerDemoBase::WriteTrackSampleByBufferQueue write AUDIO and VIDEO sample"<<std::endl;
        std::cout<<"audio trackId:"<<audioTrackId_<<" video trackId:"<<videoTrackId_<<std::endl;
        WriteAvTrackSampleByBufferQueue();
    } else if (audioFile_ != nullptr && audioTrackId_ >= 0) {
        std::cout<<"AVMuxerDemoBase::WriteTrackSampleByBufferQueue write AUDIO sample"<<std::endl;
        WriteSingleTrackSampleByBufferQueue(audioBufferQueue_, audioFile_);
    } else if (videoFile_ != nullptr && videoTrackId_ >= 0) {
        std::cout<<"AVMuxerDemoBase::WriteTrackSampleByBufferQueue write VIDEO sample"<<std::endl;
        WriteSingleTrackSampleByBufferQueue(videoBufferQueue_, videoFile_);
    } else {
        std::cout<<"AVMuxerDemoBase::WriteTrackSampleByBufferQueue don't write AUDIO and VIDEO track!!"<<std::endl;
    }
}

void AVMuxerDemoBase::MulThdWriteTrackSample(AVMuxerDemoBase *muxerBase, uint32_t trackId,
    std::shared_ptr<std::ifstream> file)
{
    muxerBase->WriteSingleTrackSample(trackId, file);
}

void AVMuxerDemoBase::MulThdWriteTrackSampleByBufferQueue(AVMuxerDemoBase *muxerBase,
    sptr<AVBufferQueueProducer> bufferQueue, std::shared_ptr<std::ifstream> file)
{
    muxerBase->WriteSingleTrackSampleByBufferQueue(bufferQueue, file);
}

void AVMuxerDemoBase::WriteCoverSample()
{
    if (coverParams_ == nullptr) {
        return;
    }
    std::cout<<"AVMuxerDemoBase::WriteCoverSample"<<std::endl;
    if (coverFile_ == nullptr) {
        std::cout<<"AVMuxerDemoBase::WriteCoverSample coverFile_ is nullptr!"<<std::endl;
        return;
    }

    coverFile_->seekg(0, std::ios::end);
    int32_t size = coverFile_->tellg();
    coverFile_->seekg(0, std::ios::beg);
    if (size <= 0) {
        std::cout<<"AVMuxerDemoBase::WriteCoverSample coverFile_ size is 0!"<<std::endl;
        return;
    }

    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(alloc, size);
    coverFile_->read(reinterpret_cast<char*>(avMemBuffer->memory_->GetAddr()), size);
    avMemBuffer->pts_ = 0;
    avMemBuffer->memory_->SetSize(size);
    if (DoWriteSample(coverTrackId_, avMemBuffer) != AVCS_ERR_OK) {
        std::cout<<"WriteCoverSample error"<<std::endl;
    }
}

int AVMuxerDemoBase::AddVideoTrack(const VideoTrackParam *param)
{
    if (param == nullptr) {
        std::cout<<"AVMuxerDemoBase::AddVideoTrack video is not select!"<<std::endl;
        return -1;
    }
    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(param->mimeType);
    videoParams->Set<Tag::VIDEO_WIDTH>(param->width);
    videoParams->Set<Tag::VIDEO_HEIGHT>(param->height);
    videoParams->Set<Tag::VIDEO_FRAME_RATE>(param->frameRate);
    videoParams->Set<Tag::VIDEO_DELAY>(param->videoDelay);
    if (param == &g_videoHdrPar) {
        videoParams->Set<Tag::VIDEO_COLOR_PRIMARIES>(static_cast<Plugins::ColorPrimary>(param->colorPrimaries));
        videoParams->Set<Tag::VIDEO_COLOR_TRC>(static_cast<Plugins::TransferCharacteristic>(param->colorTransfer));
        videoParams->Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(
            static_cast<Plugins::MatrixCoefficient>(param->colorMatrixCoeff));
        videoParams->Set<Tag::VIDEO_COLOR_RANGE>(static_cast<bool>(param->colorRange));
        videoParams->Set<Tag::VIDEO_IS_HDR_VIVID>(static_cast<bool>(param->isHdrVivid));
    }
    int extSize = 0;
    videoFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0 && extSize < CONFIG_BUFFER_SZIE) {
        std::vector<uint8_t> buffer(extSize);
        videoFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
        videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    } else {
        std::cout<<"AVMuxerDemoBase::AddVideoTrack DoAddTrack failed!"<<std::endl;
    }

    if (DoAddTrack(videoTrackId_, videoParams) != AVCS_ERR_OK) {
        return -1;
    }
    std::cout << "AVMuxerDemoBase::AddVideoTrack video trackId is: " << videoTrackId_ << std::endl;
    videoBufferQueue_ = DoGetInputBufferQueue(videoTrackId_);
    return 0;
}

int AVMuxerDemoBase::AddAudioTrack(const AudioTrackParam *param)
{
    if (param == nullptr) {
        std::cout<<"AVMuxerDemoBase::AddAudioTrack audio is not select!"<<std::endl;
        return -1;
    }
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    audioParams->Set<Tag::MIME_TYPE>(param->mimeType);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(param->sampleRate);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(param->channels);
    audioParams->Set<Tag::AUDIO_SAMPLE_PER_FRAME>(param->frameSize);
    if (param == &g_audioAacPar) {
        audioParams->Set<Tag::MEDIA_PROFILE>(Plugins::AACProfile::AAC_PROFILE_LC);
    }

    int extSize = 0;
    audioFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0 && extSize < CONFIG_BUFFER_SZIE) {
        std::vector<uint8_t> buffer(extSize);
        audioFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    } else {
        std::cout<<"AVMuxerDemoBase::AddAudioTrack error extSize:"<<extSize<<std::endl;
    }

    if (DoAddTrack(audioTrackId_, audioParams) != 0) {
        std::cout<<"AVMuxerDemoBase::AddAudioTrack DoAddTrack failed!"<<std::endl;
        return -1;
    }
    std::cout << "AVMuxerDemoBase::AddAudioTrack audio trackId is: " << audioTrackId_ << std::endl;
    audioBufferQueue_ = DoGetInputBufferQueue(audioTrackId_);
    return 0;
}

int AVMuxerDemoBase::AddCoverTrack(const VideoTrackParam *param)
{
    if (param == nullptr) {
        std::cout<<"AVMuxerDemoBase::AddCoverTrack cover is not select!"<<std::endl;
        return -1;
    }
    std::shared_ptr<Meta> coverParams = std::make_shared<Meta>();
    coverParams->Set<Tag::MIME_TYPE>(param->mimeType);
    coverParams->Set<Tag::VIDEO_WIDTH>(param->width);
    coverParams->Set<Tag::VIDEO_HEIGHT>(param->height);

    if (DoAddTrack(coverTrackId_, coverParams) != AVCS_ERR_OK) {
        return -1;
    }
    std::cout << "AVMuxerDemoBase::AddCoverTrack video trackId is: " << coverTrackId_ << std::endl;
    coverBufferQueue_ = DoGetInputBufferQueue(coverTrackId_);
    return 0;
}

std::string AVMuxerDemoBase::GetOutputFileName(std::string header)
{
    std::string outputFileName = header;
    if (!audioType_.empty()) {
        outputFileName += "_" + audioType_;
    }
    if (!videoType_.empty()) {
        outputFileName += "_" + videoType_;
    }
    if (!coverType_.empty()) {
        outputFileName += "_" + coverType_;
    }
    outputFileName += "." + format_;
    return outputFileName;
}
} // MediaAVCodec
} // OHOS