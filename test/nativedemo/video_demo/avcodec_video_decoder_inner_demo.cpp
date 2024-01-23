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

#include <iostream>
#include <unistd.h>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "iconsumer_surface.h"
#include "demo_log.h"
#include "media_description.h"
#include "securec.h"
#include "ui/rs_surface_node.h"
#include "wm/window.h"
#include "window_option.h"
#include "avcodec_video_decoder_inner_demo.h"


using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::InnerVideoDemo;
using namespace std;
namespace {
constexpr uint32_t DEFAULT_WIDTH = 320;
constexpr uint32_t DEFAULT_HEIGHT = 240;

constexpr string_view inputFilePath = "/data/test/media/out_320_240_10s.h264";
constexpr string_view outputFilePath = "/data/test/media/out_320_240_10s.yuv";
constexpr string_view outputSurfacePath = "/data/test/media/out_320_240_10s.rgba";
uint32_t outFrameCount = 0;
constexpr uint32_t SLEEP_TIME = 1;
} // namespace

TestConsumerListener::TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs_(cs)
{
    outFile_ = std::make_unique<std::ofstream>();
    outFile_->open(name.data(), std::ios::out | std::ios::binary);
}

TestConsumerListener::~TestConsumerListener()
{
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

void VDecInnerDemo::RunCase(std::string &mode)
{
    mode_ = mode;

    DEMO_CHECK_AND_RETURN_LOG(CreateDec() == AVCS_ERR_OK, "Fatal: CreateDec fail");

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");

    if (mode_ != "0") {
        sptr<Surface> ps = GetSurface(mode_);
        DEMO_CHECK_AND_RETURN_LOG(SetOutputSurface(ps) == AVCS_ERR_OK, "Fatal: SetSurface fail");
    }

    DEMO_CHECK_AND_RETURN_LOG(Start() == AVCS_ERR_OK, "Fatal: Start fail");
    while (isRunning_.load()) {
        sleep(SLEEP_TIME); // start run 1s
    }
    DEMO_CHECK_AND_RETURN_LOG(Stop() == AVCS_ERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == AVCS_ERR_OK, "Fatal: Release fail");
}

VDecInnerDemo::VDecInnerDemo()
{
    codec_ = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (codec_ == nullptr) {
        std::cout << "find h264 fail" << std::endl;
        exit(1);
    }

    parser_ = av_parser_init(codec_->id);
    if (parser_ == nullptr) {
        std::cout << "parser init fail" << std::endl;
        exit(1);
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (codec_ctx_ == nullptr) {
        std::cout << "alloc context fail" << std::endl;
        exit(1);
    }

    if (avcodec_open2(codec_ctx_, codec_, NULL) < 0) {
        std::cout << "codec open fail" << std::endl;
        exit(1);
    }

    pkt_ = av_packet_alloc();
    if (pkt_ == nullptr) {
        std::cout << "alloc pkt_ fail" << std::endl;
        exit(1);
    }
    pkt_->data = NULL;
    pkt_->size = 0;
}

VDecInnerDemo::~VDecInnerDemo()
{
    avcodec_free_context(&codec_ctx_);
    av_parser_close(parser_);
    av_packet_free(&pkt_);

    if (inputFile_ != nullptr) {
        inputFile_->close();
    }

    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

sptr<Surface> VDecInnerDemo::GetSurface(std::string &mode)
{
    sptr<Surface> ps = nullptr;
    if (mode == "1") {
        sptr<Surface> cs = Surface::CreateSurfaceAsConsumer();
        sptr<IBufferConsumerListener> listener = new InnerVideoDemo::TestConsumerListener(cs, outputSurfacePath);
        cs->RegisterConsumerListener(listener);
        auto p = cs->GetProducer();
        ps = Surface::CreateSurfaceAsProducer(p);
    } else if (mode == "2") {
        sptr<Rosen::Window> window = nullptr;
        sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
        option->SetWindowRect({0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT});
        option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_FLOAT);
        option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
        window = Rosen::Window::Create("avcodec_unittest", option);
        DEMO_CHECK_AND_RETURN_RET_LOG(window != nullptr && window->GetSurfaceNode() != nullptr, nullptr,
                                      "Fatal: Create window fail");
        window->Show();
        ps = window->GetSurfaceNode()->GetSurface();
    }

    return ps;
}

int32_t VDecInnerDemo::CreateDec()
{
    videoDec_ = VideoDecoderFactory::CreateByName(AVCodecCodecName::VIDEO_DECODER_AVC_NAME.data());
    DEMO_CHECK_AND_RETURN_RET_LOG(videoDec_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = make_shared<VDecSignal>();

    cb_ = make_unique<VDecDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(videoDec_->SetCallback(cb_) == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                                  "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t VDecInnerDemo::Configure(const Format &format)
{
    return videoDec_->Configure(format);
}

int32_t VDecInnerDemo::SetOutputSurface(sptr<Surface> surface)
{
    return videoDec_->SetOutputSurface(surface);
}

int32_t VDecInnerDemo::Start()
{
    inputFile_ = std::make_unique<std::ifstream>();
    DEMO_CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    inputFile_->open(inputFilePath.data(), std::ios::in | std::ios::binary);

    if (mode_ == "0") {
        outFile_ = std::make_unique<std::ofstream>();
        DEMO_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
        outFile_->open(outputFilePath.data(), std::ios::out | std::ios::binary);
    }

    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VDecInnerDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&VDecInnerDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return videoDec_->Start();
}

int32_t VDecInnerDemo::Stop()
{
    isRunning_.store(false);

    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inQueue_.push(0);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outQueue_.push(0);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
    }

    return videoDec_->Stop();
}

int32_t VDecInnerDemo::Flush()
{
    return videoDec_->Flush();
}

int32_t VDecInnerDemo::Reset()
{
    return videoDec_->Reset();
}

int32_t VDecInnerDemo::Release()
{
    return videoDec_->Release();
}

void VDecInnerDemo::HandleInputEOS(const uint32_t &index)
{
    AVCodecBufferInfo attr;
    AVCodecBufferFlag flag;
    attr.presentationTimeUs = 0;
    attr.size = 0;
    attr.offset = 0;
    flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS;
    (void)videoDec_->QueueInputBuffer(index, attr, flag);
    std::cout << "end buffer" << std::endl;
}

int32_t VDecInnerDemo::HandleNormalInput(const uint32_t &index, const int64_t &pts, const size_t &size)
{
    AVCodecBufferInfo attr;
    AVCodecBufferFlag flag;
    attr.presentationTimeUs = pts;
    attr.size = size;
    attr.offset = 0;
    flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    auto result = videoDec_->QueueInputBuffer(index, attr, flag);
    return result;
}

int32_t VDecInnerDemo::ExtractPacket()
{
    int32_t len = 0;
    int32_t ret = 0;

    if (data_ == nullptr) {
        data_ = inbuf_;
        (void)inputFile_->read(reinterpret_cast<char *>(inbuf_), VIDEO_INBUF_SIZE);
        data_size_ = inputFile_->gcount();
    }

    if ((data_size_ < VIDEO_REFILL_THRESH) && !file_end_) {
        memmove_s(inbuf_, data_size_, data_, data_size_);
        data_ = inbuf_;
        (void)inputFile_->read(reinterpret_cast<char *>(data_ + data_size_), VIDEO_INBUF_SIZE - data_size_);
        len = inputFile_->gcount();
        if (len > 0) {
            data_size_ += len;
        } else if (len == 0 && data_size_ == 0) {
            file_end_ = true;
            cout << "extract file end" << endl;
        }
    }

    if (data_size_ > 0) {
        ret = av_parser_parse2(parser_, codec_ctx_, &pkt_->data, &pkt_->size, data_, data_size_, AV_NOPTS_VALUE,
                               AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            cout << "av_parser_parser2 Error!" << endl;
        }
        data_ += ret;
        data_size_ -= ret;
        if (pkt_->size) {
            return AVCS_ERR_OK;
        } else {
            return AVCS_ERR_UNKNOWN;
        }
    }
    return AVCS_ERR_UNKNOWN;
}

void VDecInnerDemo::InputFunc()
{
    while (isRunning_.load()) {
        std::unique_lock<std::mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return signal_->inQueue_.size() > 0; });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        std::shared_ptr<AVSharedMemory> buffer = signal_->inBufferQueue_.front();
        lock.unlock();

        if (buffer == nullptr) {
            isRunning_.store(false);
            std::cout << "buffer is null:" << index << std::endl;
            break;
        }

        if (!file_end_ && (ExtractPacket() != AVCS_ERR_OK || pkt_->size == 0)) {
            continue;
        }

        if (file_end_) {
            HandleInputEOS(index);
            av_packet_unref(pkt_);
            break;
        }

        if (memcpy_s(buffer->GetBase(), buffer->GetSize(), pkt_->data, pkt_->size) != EOK) {
            cout << "Fatal: memcpy fail" << endl;
            break;
        }
        auto result = HandleNormalInput(index, pkt_->pts, pkt_->size);
        av_packet_unref(pkt_);

        if (result != AVCS_ERR_OK) {
            std::cout << "QueueInputBuffer error:\n";
            isRunning_ = false;
            break;
        }
        lock.lock();
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
}

void VDecInnerDemo::OutputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return signal_->outQueue_.size() > 0; });
        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        auto attr = signal_->infoQueue_.front();
        auto flag = signal_->flagQueue_.front();
        auto buffer = signal_->outBufferQueue_.front();
        lock.unlock();
        if (flag == AVCODEC_BUFFER_FLAG_EOS) {
            cout << "decode eos, write frame:" << outFrameCount << endl;
            isRunning_.store(false);
        }

        if (outFile_ != nullptr && attr.size != 0 && buffer != nullptr && buffer->GetBase() != nullptr) {
            cout << "OutputFunc write file,buffer index" << index << ", data size = :" << attr.size << endl;
            outFile_->write(reinterpret_cast<char *>(buffer->GetBase()), attr.size);
        }

        if (mode_ == "0" && videoDec_->ReleaseOutputBuffer(index, false) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        if (mode_ != "0" && videoDec_->ReleaseOutputBuffer(index, true) != AVCS_ERR_OK) {
            cout << "Fatal: RenderOutputBuffer fail" << endl;
            break;
        }
        lock.lock();
        signal_->outQueue_.pop();
        signal_->infoQueue_.pop();
        signal_->flagQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
}

VDecDemoCallback::VDecDemoCallback(shared_ptr<VDecSignal> signal) : signal_(signal) {}

void VDecDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VDecDemoCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
    cout << "OnOutputFormatChanged received" << endl;
}

void VDecDemoCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(buffer);
    signal_->inCond_.notify_all();
}

void VDecDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                               std::shared_ptr<AVSharedMemory> buffer)
{
    (void)info;
    (void)flag;
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->outBufferQueue_.push(buffer);
    signal_->infoQueue_.push(info);
    signal_->flagQueue_.push(flag);
    if (info.size > 0) {
        outFrameCount++;
    }
    signal_->outCond_.notify_all();
}