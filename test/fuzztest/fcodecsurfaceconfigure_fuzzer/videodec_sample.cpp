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
#include "videodec_sample.h"
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int32_t DEFAULT_W = 1920;
constexpr int32_t DEFAULT_H = 1080;
constexpr uint32_t SURFACE_SWITCH_FRAME = 9;
const uint32_t FC_H264[] = {139107, 1114, 474, 253, 282,    146,  197, 90,  108, 3214, 301, 77, 51, 43,
                            234,    210,  143, 108, 139107, 1114, 474, 253, 282, 146,  197, 90, 108};
constexpr uint32_t FC_LENGTH_H264 = sizeof(FC_H264) / sizeof(uint32_t);
constexpr string_view formatChangeInputFilePath = "/data/test/media/format_change_testseq.h264";
constexpr string_view outputSurfacePath = "/data/test/media/out.rgba";
} // namespace

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
    cout << "Error received, errorCode:" << errorCode << endl;
}

static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
    cout << "OnOutputFormatChanged received" << endl;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                    void *userData)
{
    (void)codec;
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    if (attr) {
        unique_lock<mutex> lock(signal->outMutex_);
        signal->outQueue_.push(index);
        signal->outBufferQueue_.push(data);
        signal->attrQueue_.push(*attr);
        signal->outCond_.notify_all();
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
}

TestConsumerListener::TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs_(cs)
{
    outDir_ = std::make_unique<std::ofstream>();
    outDir_->open(name.data(), std::ios::out | std::ios::binary);
}

TestConsumerListener::~TestConsumerListener()
{
    if (outDir_ != nullptr) {
        outDir_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;
    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);
    (void)outDir_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

static sptr<Surface> GetSurface(std::string outputPath)
{
    sptr<Surface> cs = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs, outputPath);
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    sptr<Surface> ps = Surface::CreateSurfaceAsProducer(p);
    return ps;
}

VDecFuzzSample::~VDecFuzzSample()
{
    if (videoDec_ != nullptr) {
        OH_VideoDecoder_Destroy(videoDec_);
    }
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
    }
}

int32_t VDecFuzzSample::SetParameter()
{
    if (videoDec_ == nullptr) {
        cout << "codec is nullptr" << endl;
        return AV_ERR_UNKNOWN;
    }
    if (!isSurfaceMode) {
        return AV_ERR_OK;
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "set parameter failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, 0);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_SCALING_MODE, 0);
    int32_t ret = OH_VideoDecoder_SetParameter(videoDec_, format);
    if (ret != AV_ERR_OK) {
        cout << "set parameter failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    cout << "set parameter success" << endl;
    return AV_ERR_OK;
}

int32_t VDecFuzzSample::ProceFunc()
{
    videoDec_ = OH_VideoDecoder_CreateByName("OH.Media.Codec.Decoder.Video.AVC");
    if (videoDec_ == nullptr) {
        cout << "create codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    signal_ = make_shared<VDecSignal>();
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_VideoDecoder_SetCallback(videoDec_, cb_, signal_.get());
    if (ret != AV_ERR_OK) {
        return AV_ERR_UNKNOWN;
    }
    format_ = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_W);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_H);
    ret = OH_VideoDecoder_Configure(videoDec_, format_);
    if (ret != AV_ERR_OK) {
        cout << "configure codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    if (isSurfaceMode) {
        cout << "surface mode, create output surface" << endl;
        surface_ = GetSurface(std::string(outputSurfacePath));
        OHNativeWindow *nativeWindow = CreateNativeWindowFromSurface(&surface_);
        ret = OH_VideoDecoder_SetSurface(videoDec_, nativeWindow);
        if (ret != AV_ERR_OK) {
            return AV_ERR_UNKNOWN;
        }
    } else {
        cout << "buffer mode" << endl;
    }
    ret = OH_VideoDecoder_Start(videoDec_);
    if (ret != AV_ERR_OK) {
        cout << "start codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    ret = SetParameter();
    if (ret != AV_ERR_OK) {
        return AV_ERR_UNKNOWN;
    }
    isRunning_.store(true);
    return AV_ERR_OK;
}

void VDecFuzzSample::PrepareResource()
{
    testFile_ = std::make_unique<std::ifstream>();
    testFile_->open(formatChangeInputFilePath, std::ios::in | std::ios::binary);
    if (!testFile_->is_open()) {
        cout << "open input file failed" << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(videoDec_);
        testFile_->close();
        testFile_.reset();
        testFile_ = nullptr;
        return;
    }
}

void VDecFuzzSample::FormatChangeInputFunc()
{
    PrepareResource();
    while (!inEnd_) {
        if (!isRunning_.load()) {
            return;
        }
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || isRunning_.load()); });
        if (!isRunning_.load()) {
            return;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
        if (frameCount_ < FC_LENGTH_H264) {
            info.size = FC_H264[frameCount_];
            char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * info.size + 1));
            if (fileBuffer == nullptr) {
                return;
            }
            (void)testFile_->read(fileBuffer, info.size);
            if (memcpy_s(OH_AVMemory_GetAddr(buffer), OH_AVMemory_GetSize(buffer), fileBuffer, info.size) != EOK) {
                free(fileBuffer);
                isRunning_.store(false);
                inEnd_ = true;
            }
            free(fileBuffer);
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            if (isFirstFrame_) {
                info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                isFirstFrame_ = false;
            }
            OH_VideoDecoder_PushInputData(videoDec_, index, info);
            int32_t ret = SwitchSurface();
            if (ret != AV_ERR_OK) {
                isRunning_.store(false);
                inEnd_ = true;
            }
            frameCount_++;
        } else {
            OH_VideoDecoder_PushInputData(videoDec_, index, info);
            inEnd_ = true;
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
    if (testFile_ != nullptr) {
        testFile_->close();
    }
}

void VDecFuzzSample::OutputFunc()
{
    while (!outEnd_) {
        if (!isRunning_.load()) {
            cout << "stop, exit" << endl;
            outEnd_ = true;
        }
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            outEnd_ = true;
        }
        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *data = signal_->outBufferQueue_.front();
        if (outFile_ != nullptr && attr.size != 0 && data != nullptr && OH_AVMemory_GetAddr(data) != nullptr) {
            outFile_->write(reinterpret_cast<char *>(OH_AVMemory_GetAddr(data)), attr.size);
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos, write frame:" << frameCount_ << endl;
            isRunning_.store(false);
        }
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        if (surface_) {
            OH_VideoDecoder_RenderOutputData(videoDec_, index);
        } else {
            OH_VideoDecoder_FreeOutputData(videoDec_, index);
        }
    }
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

int32_t VDecFuzzSample::SwitchSurface()
{
    if (videoDec_ == nullptr) {
        cout << "codec is nullptr" << endl;
        return AV_ERR_UNKNOWN;
    }
    if (!isSurfaceMode) {
        return AV_ERR_OK;
    }
    if (surface_ == nullptr) {
        return AV_ERR_OK;
    }
    if (frameCount_ != SURFACE_SWITCH_FRAME) {
        return AV_ERR_OK;
    }
    cout << "create new surface" << endl;
    surface_ = GetSurface(std::string("/data/test/media/out_new.rgba"));
    OHNativeWindow *nativeWindow = CreateNativeWindowFromSurface(&surface_);
    int32_t ret = OH_VideoDecoder_SetSurface(videoDec_, nativeWindow);
    if (ret != AV_ERR_OK) {
        cout << "switch surface failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    cout << "switch surface success" << endl;
    return AV_ERR_OK;
}

int32_t VDecFuzzSample::CheckCodecStatus()
{
    if (videoDec_ == nullptr) {
        cout << "codec is nullptr" << endl;
        return AV_ERR_UNKNOWN;
    }
    int32_t ret = OH_VideoDecoder_Flush(videoDec_);
    if (ret != AV_ERR_OK) {
        cout << "flush codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "create format failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    format = OH_VideoDecoder_GetOutputDescription(videoDec_);
    if (format == nullptr) {
        cout << "get output description failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    ret = OH_VideoDecoder_Start(videoDec_);
    if (ret != AV_ERR_OK) {
        cout << "start codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    ret = OH_VideoDecoder_Stop(videoDec_);
    if (ret != AV_ERR_OK) {
        cout << "stop codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    ret = OH_VideoDecoder_Start(videoDec_);
    if (ret != AV_ERR_OK) {
        cout << "start codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    ret = OH_VideoDecoder_Reset(videoDec_);
    if (ret != AV_ERR_OK) {
        cout << "reset codec failed" << endl;
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

void VDecFuzzSample::RunVideoDec()
{
    int32_t ret = ProceFunc();
    if (ret != AV_ERR_OK) {
        isRunning_.store(false);
        return;
    }

    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VDecFuzzSample::FormatChangeInputFunc, this);

    outputLoop_ = make_unique<thread>(&VDecFuzzSample::OutputFunc, this);

    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    ret = CheckCodecStatus();
    if (ret != AV_ERR_OK) {
        cout << "codec status switch failed" << endl;
        return;
    }
}
