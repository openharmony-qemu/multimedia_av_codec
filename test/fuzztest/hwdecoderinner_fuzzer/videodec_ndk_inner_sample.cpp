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
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include "iconsumer_surface.h"
#include "meta/meta_key.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "native_buffer_inner.h"
#include "videodec_inner_sample.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;

namespace {
const string MIME_TYPE = "video/avc";
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr int32_t THREE = 3;
constexpr int32_t EIGHT = 8;
constexpr int32_t TEN = 10;
constexpr int32_t SIXTEEN = 16;
constexpr int32_t TWENTY_FOUR = 24;
constexpr uint32_t FRAME_INTERVAL = 1; // 16666
constexpr uint8_t H264_NALU_TYPE = 0x1f;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
constexpr uint8_t SPS = 7;
constexpr uint8_t PPS = 8;
int32_t g_strideSurface = 0;
int32_t g_sliceSurface = 0;
bool g_yuvSurface = false;
VDecNdkInnerFuzzSample *g_decSample = nullptr;

SHA512_CTX g_ctx;
unsigned char g_md[SHA512_DIGEST_LENGTH];

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void clearBufferqueue(std::queue<AVCodecBufferInfo> &q)
{
    std::queue<AVCodecBufferInfo> empty;
    swap(empty, q);
}

void clearFlagqueue(std::queue<AVCodecBufferFlag> &q)
{
    std::queue<AVCodecBufferFlag> empty;
    swap(empty, q);
}
} // namespace

class ConsumerListenerBuffer : public IBufferConsumerListener {
public:
    ConsumerListenerBuffer(sptr<Surface> cs, std::string_view name) : cs(cs)
    {
        outFile_ = std::make_unique<std::ofstream>();
        outFile_->open(name.data(), std::ios::out | std::ios::binary);
    };
    ~ConsumerListenerBuffer()
    {
        if (outFile_ != nullptr) {
            outFile_->close();
        }
    }
    void OnBufferAvailable() override
    {
        sptr<SurfaceBuffer> buffer;
        int32_t flushFence;
        cs->AcquireBuffer(buffer, flushFence, timestamp, damage);
        cs->ReleaseBuffer(buffer, -1);
    }

private:
    int64_t timestamp = 0;
    Rect damage = {};
    sptr<Surface> cs {nullptr};
    std::unique_ptr<std::ofstream> outFile_;
};

VDecInnerCallback::VDecInnerCallback(std::shared_ptr<VDecInnerSignal> signal) : innersignal_(signal) {}

void VDecInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    g_decSample->isRunning_.store(false);
    innersignal_->inCond_.notify_all();
    innersignal_->outCond_.notify_all();
}

void VDecInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int32_t stride = 0;
    int32_t sliceHeight = 0;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, currentWidth);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, currentHeight);
    format.GetIntValue(Media::Tag::VIDEO_STRIDE, stride);
    format.GetIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight);
    g_decSample->defaultWidth = currentWidth;
    g_decSample->defaultHeight = currentHeight;
    g_strideSurface = stride;
    g_sliceSurface = sliceHeight;
}

void VDecInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable index:" << index << endl;
    if (innersignal_ == nullptr) {
        std::cout << "buffer is null 1" << endl;
        return;
    }

    unique_lock<mutex> lock(innersignal_->inMutex_);
    innersignal_->inIdxQueue_.push(index);
    innersignal_->inBufferQueue_.push(buffer);
    innersignal_->inCond_.notify_all();
}

void VDecInnerCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
    AVCodecBufferFlag flag, std::shared_ptr<AVSharedMemory> buffer)
{
    if (g_decSample->sfOutput) {
        g_decSample->vdec_->ReleaseOutputBuffer(index, true);
    } else {
        g_decSample->vdec_->ReleaseOutputBuffer(index, false);
    }
}

VDecNdkInnerFuzzSample::~VDecNdkInnerFuzzSample()
{
    for (int i = 0; i < static_cast<int>(maxSurfNum); i++) {
        if (nativeWindow[i]) {
            OH_NativeWindow_DestroyNativeWindow(nativeWindow[i]);
            nativeWindow[i] = nullptr;
        }
    }
    g_yuvSurface = false;
    if (!afterEosDestoryCodec && vdec_ != nullptr) {
        (void)Stop();
        Release();
    }
}

int64_t VDecNdkInnerFuzzSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

void VDecNdkInnerFuzzSample::CreateSurface()
{
    cs[0] = Surface::CreateSurfaceAsConsumer();
    if (cs[0] == nullptr) {
        cout << "Create the surface consummer fail" << endl;
        return;
    }
    GSError err = cs[0]->SetDefaultUsage(BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_DECODER | BUFFER_USAGE_CPU_READ);
    if (err == GSERROR_OK) {
        cout << "set consumer usage succ" << endl;
    } else {
        cout << "set consumer usage failed" << endl;
    }
    sptr<IBufferConsumerListener> listener = new ConsumerListenerBuffer(cs[0], outDir);
    cs[0]->RegisterConsumerListener(listener);
    auto p = cs[0]->GetProducer();
    ps[0] = Surface::CreateSurfaceAsProducer(p);
    nativeWindow[0] = CreateNativeWindowFromSurface(&ps[0]);
    if (autoSwitchSurface)  {
        cs[1] = Surface::CreateSurfaceAsConsumer();
        sptr<IBufferConsumerListener> listener2 = new ConsumerListenerBuffer(cs[1], outDir2);
        cs[1]->RegisterConsumerListener(listener2);
        auto p2 = cs[1]->GetProducer();
        ps[1] = Surface::CreateSurfaceAsProducer(p2);
        nativeWindow[1] = CreateNativeWindowFromSurface(&ps[1]);
    }
}

int32_t VDecNdkInnerFuzzSample::CreateByMime(const std::string &mime)
{
    vdec_ = VideoDecoderFactory::CreateByMime(mime);
    return vdec_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VDecNdkInnerFuzzSample::CreateByName(const std::string &name)
{
    vdec_ = VideoDecoderFactory::CreateByName(name);
    g_decSample = this;
    return vdec_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VDecNdkInnerFuzzSample::Configure()
{
    if (autoSwitchSurface) {
        switchSurfaceFlag = (switchSurfaceFlag == 1) ? 0 : 1;
        if (ps[switchSurfaceFlag] != nullptr) {
            if (vdec_->SetOutputSurface(ps[switchSurfaceFlag]) != AVCS_ERR_INVALID_STATE) {
                errCount++;
            }
        }
    }
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, defaultWidth);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, defaultHeight);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, DEFAULT_FORMAT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, defaultFrameRate);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, defaultColorspace);
    return vdec_->Configure(format);
}

int32_t VDecNdkInnerFuzzSample::Prepare()
{
    return vdec_->Prepare();
}

int32_t VDecNdkInnerFuzzSample::Start()
{
    int32_t ret = vdec_->Start();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning_.store(false);
        ReleaseInFile();
        Release();
    }
    return ret;
}

void VDecNdkInnerFuzzSample::GetStride()
{
    Format format;
    int32_t ret = vdec_->GetOutputFormat(format);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to get output format" << endl;
        return;
    }
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int32_t stride = 0;
    int32_t sliceHeight = 0;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, currentWidth);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, currentHeight);
    format.GetIntValue(Media::Tag::VIDEO_STRIDE, stride);
    format.GetIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight);
    g_decSample->defaultWidth = currentWidth;
    g_decSample->defaultHeight = currentHeight;
    g_strideSurface = stride;
    g_sliceSurface = sliceHeight;
}

int32_t VDecNdkInnerFuzzSample::Stop()
{
    StopInloop();
    if (signal_ != nullptr) {
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->infoQueue_);
        clearFlagqueue(signal_->flagQueue_);
    }
    ReleaseInFile();

    return vdec_->Stop();
}

int32_t VDecNdkInnerFuzzSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();

    return vdec_->Flush();
}

int32_t VDecNdkInnerFuzzSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();

    return vdec_->Reset();
}

int32_t VDecNdkInnerFuzzSample::Release()
{
    int32_t ret = 0;
    if (vdec_ != nullptr) {
        ret = vdec_->Release();
        vdec_ = nullptr;
    }
    if (signal_ != nullptr) {
        signal_ = nullptr;
    }
    return ret;
}

int32_t VDecNdkInnerFuzzSample::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    return vdec_->QueueInputBuffer(index, info, flag);
}

int32_t VDecNdkInnerFuzzSample::GetOutputFormat(Format &format)
{
    return vdec_->GetOutputFormat(format);
}

int32_t VDecNdkInnerFuzzSample::ReleaseOutputBuffer(uint32_t index)
{
    return vdec_->ReleaseOutputBuffer(index, false);
}

int32_t VDecNdkInnerFuzzSample::SetParameter(const Format &format)
{
    return vdec_->SetParameter(format);
}

int32_t VDecNdkInnerFuzzSample::SetCallback()
{
    signal_ = make_shared<VDecInnerSignal>();
    if (signal_ == nullptr) {
        cout << "Failed to new VEncInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    cb_ = make_shared<VDecInnerCallback>(signal_);
    return vdec_->SetCallback(cb_);
}

int32_t VDecNdkInnerFuzzSample::StartVideoDecoder()
{
    isRunning_.store(true);
    if (prepareFlag) {
        int res = Prepare();
        if (res != AVCS_ERR_OK) {
            cout << "prepare failed:" << res << endl;
            isRunning_.store(false);
            ReleaseInFile();
            Release();
            return res;
        }
    }
    int32_t ret = vdec_->Start();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning_.store(false);
        ReleaseInFile();
        Release();
        return ret;
    }

    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning_.store(false);
        vdec_->Stop();
        return AVCS_ERR_UNKNOWN;
    }

    inFile_->open(inpDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        OpenFileFail();
        return AVCS_ERR_UNKNOWN;
    }

    inputLoop_ = make_unique<thread>(&VDecNdkInnerFuzzSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        vdec_->Stop();
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }
    
    outputLoop_ = make_unique<thread>(&VDecNdkInnerFuzzSample::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        isRunning_.store(false);
        vdec_->Stop();
        ReleaseInFile();
        StopInloop();
        Release();
        return AVCS_ERR_UNKNOWN;
    }

    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerFuzzSample::RunVideoDecoder(const std::string &codeName)
{
    sfOutput = false;
    int32_t ret = CreateByName(codeName);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to create video decoder" << endl;
        return ret;
    }

    ret = Configure();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return ret;
    }

    ret = SetCallback();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return ret;
    }

    ret = StartVideoDecoder();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return ret;
    }
    return ret;
}

int32_t VDecNdkInnerFuzzSample::RunVideoDecSurface(const std::string &codeName)
{
    sfOutput = true;
    int ret = AVCS_ERR_OK;
    CreateSurface();
    if (!nativeWindow[0]) {
        cout << "Failed to create surface" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    ret = CreateByName(codeName);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to create video decoder" << endl;
        return ret;
    }

    ret = Configure();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return ret;
    }

    ret = SetCallback();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return ret;
    }

    if (ps[0] == nullptr) {
        cout << "ps[0] is nullptr" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    ret = vdec_->SetOutputSurface(ps[0]);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to set surface" << endl;
        return ret;
    }
    ret = StartVideoDecoder();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return ret;
    }
    return ret;
}

int32_t VDecNdkInnerFuzzSample::PushData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index)
{
    static uint32_t repeatCount = 0;

    if (beforeEosInput && frameCount > TEN) {
        SetEOS(index);
        return 1;
    }

    if (beforeEosInputInput && frameCount > TEN) {
        beforeEosInputInput = false;
    }

    char ch[4] = {};
    (void)inFile_->read(ch, START_CODE_SIZE);
    if (repeatRun && inFile_->eof()) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        cout << "repeat run " << repeatCount << endl;
        repeatCount++;
        return 0;
    }

    if (inFile_->eof()) {
        SetEOS(index);
        return 1;
    }

    uint32_t bufferSize = static_cast<uint32_t>(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << EIGHT) |
        ((ch[1] & 0xFF) << SIXTEEN) | ((ch[0] & 0xFF) << TWENTY_FOUR));
    if (bufferSize >= ((defaultWidth * defaultHeight * THREE) >> 1)) {
        cout << "read bufferSize abnormal. buffersize = " << bufferSize << endl;
        return 1;
    }

    return SendData(bufferSize, index, buffer);
}

int32_t VDecNdkInnerFuzzSample::SendData(uint32_t bufferSize, uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

    uint8_t *fileBuffer = new uint8_t[bufferSize + START_CODE_SIZE];
    if (fileBuffer == nullptr) {
        delete[] fileBuffer;
        return 0;
    }
    if (memcpy_s(fileBuffer, bufferSize + START_CODE_SIZE, START_CODE, START_CODE_SIZE) != EOK) {
        cout << "Fatal: memory copy failed" << endl;
    }

    (void)inFile_->read((char *)fileBuffer + START_CODE_SIZE, bufferSize);
    if ((fileBuffer[START_CODE_SIZE] & H264_NALU_TYPE) == SPS ||
        (fileBuffer[START_CODE_SIZE] & H264_NALU_TYPE) == PPS) {
        flag = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    } else {
        flag = AVCODEC_BUFFER_FLAG_NONE;
    }

    int32_t size = buffer->GetSize();
    if (size < static_cast<int32_t>(bufferSize + START_CODE_SIZE)) {
        delete[] fileBuffer;
        cout << "buffer size not enough." << endl;
        return 0;
    }

    uint8_t *avBuffer = buffer->GetBase();
    if (avBuffer == nullptr) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        delete[] fileBuffer;
        return 0;
    }
    if (memcpy_s(avBuffer, size, fileBuffer, bufferSize + START_CODE_SIZE) != EOK) {
        delete[] fileBuffer;
        return 0;
    }

    info.presentationTimeUs = GetSystemTimeUs();
    info.size = bufferSize + START_CODE_SIZE;
    info.offset = 0;
    int32_t result = vdec_->QueueInputBuffer(index, info, flag);
    if (result != AVCS_ERR_OK) {
        errCount = errCount + 1;
    }
    frameCount = frameCount + 1;
    SwitchSurface();
    delete[] fileBuffer;
    return 0;
}

void VDecNdkInnerFuzzSample::SwitchSurface()
{
    if (autoSwitchSurface && (frameCount % static_cast<int32_t>(defaultFrameRate) == 0)) {
        switchSurfaceFlag = (switchSurfaceFlag == 1) ? 0 : 1;
        if (ps[switchSurfaceFlag] != nullptr) {
            vdec_->SetOutputSurface(ps[switchSurfaceFlag]) == AVCS_ERR_OK ? (0) : (errCount++);
        }
    }
}

int32_t VDecNdkInnerFuzzSample::StateEOS()
{
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return signal_->inIdxQueue_.size() > 0; });
    uint32_t index = signal_->inIdxQueue_.front();
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    lock.unlock();

    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 0;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;

    return vdec_->QueueInputBuffer(index, info, flag);
}

void VDecNdkInnerFuzzSample::RepeatStartBeforeEOS()
{
    if (repeatStartFlushBeforeEos > 0) {
        repeatStartFlushBeforeEos--;
        vdec_->Flush();
        FlushBuffer();
        vdec_->Start();
    }

    if (repeatStartStopBeforeEos > 0) {
        repeatStartStopBeforeEos--;
        vdec_->Stop();
        FlushBuffer();
        vdec_->Start();
    }
}

void VDecNdkInnerFuzzSample::SetEOS(uint32_t index)
{
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 0;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;

    int32_t res = vdec_->QueueInputBuffer(index, info, flag);
    cout << "QueueInputBuffer EOS res: " << res << endl;
}

void VDecNdkInnerFuzzSample::WaitForEOS()
{
    if (!afterEosDestoryCodec && inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }
        
    if (outputLoop_ && outputLoop_->joinable()) {
        outputLoop_->join();
    }
}

void VDecNdkInnerFuzzSample::OpenFileFail()
{
    cout << "file open fail" << endl;
    isRunning_.store(false);
    vdec_->Stop();
    inFile_->close();
    inFile_.reset();
    inFile_ = nullptr;
}

int32_t VDecNdkInnerFuzzSample::InputFuncFUZZ(const uint8_t *data, size_t size)
{
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!isRunning_.load()) {
        return AVCS_ERR_NO_MEMORY;
    }
    signal_->inCond_.wait(lock, [this]() {
        if (!isRunning_.load()) {
            return true;
        }
        return signal_->inIdxQueue_.size() > 0;
    });

    if (!isRunning_.load()) {
        return AVCS_ERR_NO_MEMORY;
    }

    uint32_t index = signal_->inIdxQueue_.front();
    auto buffer = signal_->inBufferQueue_.front();
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    lock.unlock();
    int32_t bufferSize = buffer->GetSize();
    uint8_t *bufferAddr = buffer->GetBase();
    if (size > bufferSize - START_CODE_SIZE) {
        vdec_->QueueInputBuffer(index);
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr, bufferSize, START_CODE, START_CODE_SIZE) != EOK) {
        vdec_->QueueInputBuffer(index);
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr + START_CODE_SIZE, bufferSize - START_CODE_SIZE, data, size) != EOK) {
        vdec_->QueueInputBuffer(index);
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.presentationTimeUs = GetSystemTimeUs();
    info.size = bufferSize + START_CODE_SIZE;
    info.offset = 0;
    return vdec_->QueueInputBuffer(index, info, flag);
}

void VDecNdkInnerFuzzSample::InputFunc()
{
    if (outputYuvSurface) {
        g_yuvSurface = true;
    }
    errCount = 0;
    bool flags = true;
    while (flags) {
        if (!isRunning_.load()) {
            break;
        }
        RepeatStartBeforeEOS();

        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                return true;
            }
            return signal_->inIdxQueue_.size() > 0;
        });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        lock.unlock();

        if (!inFile_->eof()) {
            int32_t ret = PushData(buffer, index);
            if (ret == 1) {
                break;
            }
        }

        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
        if (errCount > 0) {
            isRunning_.store(false);
            flags = false;
            break;
        }
    }
}

void VDecNdkInnerFuzzSample::OutputFunc()
{
    SHA512_Init(&g_ctx);
    bool flags = true;
    while (flags) {
        if (!isRunning_.load()) {
            flags =false;
            break;
        }

        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                return true;
            }
            return signal_->outIdxQueue_.size() > 0;
        });

        if (!isRunning_.load()) {
            flags = false;
            break;
        }

        std::shared_ptr<AVSharedMemory> buffer = signal_->outBufferQueue_.front();
        AVCodecBufferFlag flag = signal_->flagQueue_.front();
        uint32_t index = signal_->outIdxQueue_.front();
        
        signal_->outBufferQueue_.pop();
        signal_->outIdxQueue_.pop();
        signal_->infoQueue_.pop();
        signal_->flagQueue_.pop();
        lock.unlock();

        if (flag == AVCODEC_BUFFER_FLAG_EOS) {
            SHA512_Final(g_md, &g_ctx);
            OPENSSL_cleanse(&g_ctx, sizeof(g_ctx));
            MdCompare(g_md, SHA512_DIGEST_LENGTH, fileSourcesha256);
            if (afterEosDestoryCodec) {
                (void)Stop();
                Release();
            }
            flags = false;
            break;
        }
        ProcessOutputData(buffer, index);
        if (errCount > 0) {
            isRunning_.store(false);
            break;
        }
    }
}

void VDecNdkInnerFuzzSample::ProcessOutputData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index)
{
    if (!sfOutput) {
        uint32_t size = buffer->GetSize();
        if (size >= ((defaultWidth * defaultHeight * THREE) >> 1)) {
            uint8_t *cropBuffer = new uint8_t[size];
            if (memcpy_s(cropBuffer, size, buffer->GetBase(),
				            defaultWidth * defaultHeight) != EOK) {
                cout << "Fatal: memory copy failed Y" << endl;
            }
            // copy UV
            uint32_t uvSize = size - defaultWidth * defaultHeight;
            if (memcpy_s(cropBuffer + defaultWidth * defaultHeight, uvSize,
				            buffer->GetBase() + defaultWidth * defaultHeight, uvSize) != EOK) {
                cout << "Fatal: memory copy failed UV" << endl;
            }
            SHA512_Update(&g_ctx, cropBuffer, (defaultWidth * defaultHeight * THREE) >> 1);
            delete[] cropBuffer;
        }

        if (vdec_->ReleaseOutputBuffer(index, false) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
    } else {
        if (vdec_->ReleaseOutputBuffer(index, true) != AVCS_ERR_OK) {
            cout << "Fatal: RenderOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
    }
}

void VDecNdkInnerFuzzSample::FlushBuffer()
{
    std::queue<std::shared_ptr<AVSharedMemory>> empty;
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    swap(empty, signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();

    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
}

void VDecNdkInnerFuzzSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_.reset();
    }
}

void VDecNdkInnerFuzzSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->infoQueue_);
        clearFlagqueue(signal_->flagQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}

void VDecNdkInnerFuzzSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

bool VDecNdkInnerFuzzSample::MdCompare(unsigned char *buffer, int len, const char *source[])
{
    bool result = true;
    for (int i = 0; i < len; i++) {
        char std[SHA512_DIGEST_LENGTH] = {0};
        int re = strcmp(source[i], std);
        if (re != 0) {
            result = false;
            break;
        }
    }
    return result;
}

void VDecNdkInnerFuzzSample::SetRunning()
{
    isRunning_.store(false);
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
}

int32_t VDecNdkInnerFuzzSample::SetOutputSurface()
{
    CreateSurface();
    return vdec_->SetOutputSurface(ps[0]);
}