/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include "iconsumer_surface.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "native_buffer_inner.h"
#include "serverdec_sample.h"
#include "hevc_decoder_api.h"
#include "fcodec_api.cpp"

#define THREE 3 // 3
#define EIGHT 8 // 8
#define TEN 10 // 10
#define SIXTEEN 16 // 16
#define TWENTY_FOUR 24 // 24

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;

using namespace OHOS::Media;
class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs(cs), name(name) {};
    ~TestConsumerListener() {}
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
    std::string_view name;
};

namespace {
const string MIME_TYPE = "video/hevc";
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t FRAME_INTERVAL = 1;
constexpr uint8_t H264_NALU_TYPE = 0x1f;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
constexpr uint8_t SPS = 7;
constexpr uint8_t PPS = 8;

SHA512_CTX g_ctx;
unsigned char g_md[SHA512_DIGEST_LENGTH];

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

} // namespace

VDecInnerCallback::VDecInnerCallback(std::shared_ptr<VDecInnerSignal> signal) : innersignal_(signal) {}

void VDecInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VDecInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
}

void VDecInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
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

void VDecInnerCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    cout << "OnOutputBufferAvailable index:" << index << endl;

    unique_lock<mutex> lock(innersignal_->outMutex_);
    innersignal_->outIdxQueue_.push(index);
    innersignal_->outBufferQueue_.push(buffer);
    innersignal_->outCond_.notify_all();
}

VDecNdkInnerSample::~VDecNdkInnerSample()
{
    Release();
}

int64_t VDecNdkInnerSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec * NANOS_IN_SECOND + now.tv_nsec);
    return nanoTime / NANOS_IN_MICRO;
}

void VDecNdkInnerSample::CreateSurface()
{
    cs[0] = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs[0], outputDir);
    cs[0]->RegisterConsumerListener(listener);
    auto p = cs[0]->GetProducer();
    ps[0] = Surface::CreateSurfaceAsProducer(p);
    nativeWindow[0] = CreateNativeWindowFromSurface(&ps[0]);
}

int32_t VDecNdkInnerSample::Configure()
{
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, defaultWidth);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, defaultHeight);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, defaultFrameRate);

    return vdec_->Configure(format);
}

int32_t VDecNdkInnerSample::Prepare()
{
    return vdec_->Prepare();
}

int32_t VDecNdkInnerSample::Start()
{
    return vdec_->Start();
}

int32_t VDecNdkInnerSample::Stop()
{
    StopInloop();
    clearIntqueue(signal_->outIdxQueue_);
    ReleaseInFile();

    return vdec_->Stop();
}

int32_t VDecNdkInnerSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();

    return vdec_->Flush();
}

int32_t VDecNdkInnerSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();

    return vdec_->Reset();
}

int32_t VDecNdkInnerSample::Release()
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

int32_t VDecNdkInnerSample::QueueInputBuffer(uint32_t index)
{
    return vdec_->QueueInputBuffer(index);
}

int32_t VDecNdkInnerSample::GetOutputFormat(Format &format)
{
    return vdec_->GetOutputFormat(format);
}

int32_t VDecNdkInnerSample::ReleaseOutputBuffer(uint32_t index)
{
    return vdec_->ReleaseOutputBuffer(index);
}

int32_t VDecNdkInnerSample::SetParameter(const Format &format)
{
    return vdec_->SetParameter(format);
}

int32_t VDecNdkInnerSample::SetCallback()
{
    signal_ = make_shared<VDecInnerSignal>();
    if (signal_ == nullptr) {
        cout << "Failed to new VDecInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    cb_ = make_shared<VDecInnerCallback>(signal_);
    return vdec_->SetCallback(cb_);
}


int32_t VDecNdkInnerSample::StartErrorVideoDecoder() // 内存回收异常操作
{
    isRunning_.store(true);
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

    inFile_->open(inputDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        OpenFileFail();
        return AVCS_ERR_UNKNOWN;
    }

    inputLoop_ = make_unique<thread>(&VDecNdkInnerSample::InputErrorFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        vdec_->Stop();
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&VDecNdkInnerSample::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning_.store(false);
        vdec_->Stop();
        vdec_->NotifyMemoryRecycle();
        sleep(1);
        vdec_->NotifyMemoryRecycle();
        sleep(1);
        vdec_->NotifyMemoryWriteBack();
        sleep(1);
        vdec_->NotifyMemoryWriteBack();
        sleep(1);
        ReleaseInFile();
        StopInloop();
        Release();
        return AVCS_ERR_UNKNOWN;
    }

    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::StartVideoDecoder()
{
    isRunning_.store(true);
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

    inFile_->open(inputDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        OpenFileFail();
        return AVCS_ERR_UNKNOWN;
    }

    inputLoop_ = make_unique<thread>(&VDecNdkInnerSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        vdec_->Stop();
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&VDecNdkInnerSample::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning_.store(false);
        vdec_->Stop();
        vdec_->NotifyMemoryRecycle();
        sleep(1);
        vdec_->NotifyMemoryRecycle();
        sleep(1);
        vdec_->NotifyMemoryWriteBack();
        sleep(1);
        vdec_->NotifyMemoryWriteBack();
        sleep(1);
        ReleaseInFile();
        StopInloop();
        Release();
        return AVCS_ERR_UNKNOWN;
    }

    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::RunHEVCVideoDecoder(const std::string &codeName)
{
    int32_t ret = AVCS_ERR_OK;
    CreateSurface();
    if (!nativeWindow[0]) {
        cout << "Failed to create surface" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    CreateHevcDecoderByName("OH.Media.Codec.Decoder.Video.HEVC", vdec_);
    if (vdec_ == nullptr) {
        cout << "Failed to Create video decoder" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    Media::Meta codecInfo;
    ret = vdec_->Init(codecInfo);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to init video decoder" << endl;
        Release();
        return ret;
    }
    ret = Configure();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return ret;
    }
    vdec_->SetOutputSurface(ps[0]);
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

int32_t VDecNdkInnerSample::RunFcodecVideoDecoder(const std::string &codeName)
{
    int32_t ret = AVCS_ERR_OK;
    CreateSurface();
    if (!nativeWindow[0]) {
        cout << "Failed to create surface" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    Codec::CreateFCodecByName("OH.Media.Codec.Decoder.Video.AVC", vdec_);
    if (vdec_ == nullptr) {
        cout << "Failed to Create video decoder" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    Media::Meta codecInfo;
    ret = vdec_->Init(codecInfo);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to init video decoder" << endl;
        Release();
        return ret;
    }
    ret = Configure();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return ret;
    }
    vdec_->SetOutputSurface(ps[0]);
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

int32_t VDecNdkInnerSample::RunHEVCErrorVideoDecoder(const std::string &codeName)
{
    int32_t ret = AVCS_ERR_OK;
    CreateSurface();
    if (!nativeWindow[0]) {
        cout << "Failed to create surface" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    CreateHevcDecoderByName("OH.Media.Codec.Decoder.Video.HEVC", vdec_);
    if (vdec_ == nullptr) {
        cout << "Failed to Create video decoder" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    Media::Meta codecInfo;
    ret = vdec_->Init(codecInfo);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to init video decoder" << endl;
        Release();
        return ret;
    }
    ret = Configure();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return ret;
    }
    vdec_->SetOutputSurface(ps[0]);
    ret = SetCallback();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return ret;
    }

    ret = StartErrorVideoDecoder();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return ret;
    }
    return ret;
}

int32_t VDecNdkInnerSample::RunFcodecErrorVideoDecoder(const std::string &codeName)
{
    int32_t ret = AVCS_ERR_OK;
    CreateSurface();
    if (!nativeWindow[0]) {
        cout << "Failed to create surface" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    Codec::CreateFCodecByName("OH.Media.Codec.Decoder.Video.AVC", vdec_);
    if (vdec_ == nullptr) {
        cout << "Failed to Create video decoder" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    Media::Meta codecInfo;
    ret = vdec_->Init(codecInfo);
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to init video decoder" << endl;
        Release();
        return ret;
    }
    ret = Configure();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return ret;
    }
    vdec_->SetOutputSurface(ps[0]);
    ret = SetCallback();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return ret;
    }

    ret = StartErrorVideoDecoder();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return ret;
    }
    return ret;
}

int32_t VDecNdkInnerSample::PushData(std::shared_ptr<AVBuffer> buffer, uint32_t index)
{
    static uint32_t repeatCount = 0;

    if (beforeEosInput && frameCount > TEN) {
        SetEOS(index, buffer);
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
        SetEOS(index, buffer);
        return 1;
    }

    uint32_t bufferSize = static_cast<uint32_t>(((static_cast<unsigned char>(ch[3]) & 0xFF)) |
                                                ((static_cast<unsigned char>(ch[2]) & 0xFF) << EIGHT) |
                                                ((static_cast<unsigned char>(ch[1]) & 0xFF) << SIXTEEN) |
                                                ((static_cast<unsigned char>(ch[0]) & 0xFF) << TWENTY_FOUR));
    if (bufferSize >= ((defaultWidth * defaultHeight * THREE) >> 1)) {
        cout << "read bufferSize abnormal. buffersize = " << bufferSize << endl;
        return 1;
    }

    return SendData(bufferSize, index, buffer);
}

int32_t VDecNdkInnerSample::SendData(uint32_t bufferSize, uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    uint8_t *fileBuffer = new uint8_t[bufferSize + START_CODE_SIZE];
    if (fileBuffer == nullptr) {
        cout << "Fatal: no memory" << endl;
        delete[] fileBuffer;
        return 0;
    }
    if (memcpy_s(fileBuffer, bufferSize + START_CODE_SIZE, START_CODE, START_CODE_SIZE) != EOK) {
        cout << "Fatal: memory copy failed" << endl;
    }

    (void)inFile_->read((char *)fileBuffer + START_CODE_SIZE, bufferSize);
    if ((fileBuffer[START_CODE_SIZE] & H264_NALU_TYPE) == SPS ||
        (fileBuffer[START_CODE_SIZE] & H264_NALU_TYPE) == PPS) {
        buffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    } else {
        buffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
    }

    int32_t size = buffer->memory_->GetCapacity();
    if (size < static_cast<int32_t>(bufferSize + START_CODE_SIZE)) {
        delete[] fileBuffer;
        cout << "buffer size not enough." << endl;
        return 0;
    }

    uint8_t *avBuffer = buffer->memory_->GetAddr();
    if (avBuffer == nullptr) {
        cout << "avBuffer == nullptr" << endl;
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        delete[] fileBuffer;
        return 0;
    }
    if (memcpy_s(avBuffer, size, fileBuffer, bufferSize + START_CODE_SIZE) != EOK) {
        delete[] fileBuffer;
        cout << "Fatal: memcpy fail" << endl;
        return 0;
    }
    buffer->pts_ = GetSystemTimeUs();
    buffer->memory_->SetOffset(0);
    buffer->memory_->SetSize(bufferSize + START_CODE_SIZE);
    int32_t result = vdec_->QueueInputBuffer(index);
    if (result != AVCS_ERR_OK) {
        errCount = errCount + 1;
        cout << "push input data failed, error:" << result << endl;
    }

    delete[] fileBuffer;
    frameCount = frameCount + 1;
    return 0;
}

int32_t VDecNdkInnerSample::StateEOS()
{
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return signal_->inIdxQueue_.size() > 0; });
    uint32_t index = signal_->inIdxQueue_.front();
    auto buffer = signal_->inBufferQueue_.front();
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    lock.unlock();

    buffer->pts_ = 0;
    buffer->memory_->SetOffset(0);
    buffer->memory_->SetSize(0);
    buffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
    return vdec_->QueueInputBuffer(index);
}

void VDecNdkInnerSample::RepeatStartBeforeEOS()
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

void VDecNdkInnerSample::SetEOS(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    buffer->pts_ = 0;
    buffer->memory_->SetOffset(0);
    buffer->memory_->SetSize(0);
    buffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;

    int32_t res = vdec_->QueueInputBuffer(index);
    cout << "QueueInputBuffer EOS res: " << res << endl;
}

void VDecNdkInnerSample::WaitForEOS()
{
    if (!afterEosDestoryCodec && inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }

    if (outputLoop_ && outputLoop_->joinable()) {
        outputLoop_->join();
    }
}

void VDecNdkInnerSample::OpenFileFail()
{
    cout << "file open fail" << endl;
    isRunning_.store(false);
    vdec_->Stop();
    inFile_->close();
    inFile_.reset();
    inFile_ = nullptr;
}

void VDecNdkInnerSample::InputFunc()
{
    errCount = 0;
    while (isRunning_.load()) {
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
            vdec_->NotifyMemoryRecycle();
            usleep(100); // 100为100ms
            vdec_->NotifyMemoryWriteBack();
            usleep(100); // 100为100ms
            int32_t ret = PushData(buffer, index);
            usleep(100); // 100为100ms
            vdec_->NotifyMemoryRecycle();
            usleep(100); // 100为300ms
            vdec_->NotifyMemoryRecycle();
            usleep(100); // 100为100ms
            vdec_->NotifyMemoryWriteBack();
            usleep(100); // 100为100ms
            vdec_->NotifyMemoryWriteBack();
            usleep(100); // 100为100ms
            if (ret == 1) {
                break;
            }
        }

        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

void VDecNdkInnerSample::InputErrorFunc()
{
    errCount = 0;
    while (isRunning_.load()) {
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
            vdec_->NotifyMemoryWriteBack();
            usleep(100); // 100为100ms
            int32_t ret = PushData(buffer, index);
            usleep(100); // 100为100ms
            vdec_->NotifyMemoryRecycle();
            usleep(100); // 100为100ms
            vdec_->NotifyMemoryRecycle();
            usleep(100); // 100为100ms
            vdec_->NotifyMemoryWriteBack();
            usleep(100); // 100为100ms
            if (ret == 1) {
                break;
            }
        }

        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

void VDecNdkInnerSample::OutputFunc()
{
    SHA512_Init(&g_ctx);
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() {
            return signal_->outIdxQueue_.size() > 0;
        });
        if (!isRunning_.load()) {
            break;
        }

        std::shared_ptr<AVBuffer> buffer = signal_->outBufferQueue_.front();
        uint32_t index = signal_->outIdxQueue_.front();

        signal_->outBufferQueue_.pop();
        signal_->outIdxQueue_.pop();
        lock.unlock();

        if (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
            SHA512_Final(g_md, &g_ctx);
            OPENSSL_cleanse(&g_ctx, sizeof(g_ctx));
            MdCompare(g_md, SHA512_DIGEST_LENGTH, fileSourcesha256);
            if (afterEosDestoryCodec) {
                (void)Stop();
                Release();
            }
            break;
        }

        ProcessOutputData(buffer, index);
        if (errCount > 0) {
            break;
        }
    }
}

void VDecNdkInnerSample::ProcessOutputData(std::shared_ptr<AVBuffer> buffer, uint32_t index)
{
    if (!sfOutput) {
        uint32_t size = buffer->memory_->GetCapacity();
        if (size >= ((defaultWidth * defaultHeight * THREE) >> 1)) {
            uint8_t *cropBuffer = new uint8_t[size];
            if (memcpy_s(cropBuffer, size, buffer->memory_->GetAddr(),
                defaultWidth * defaultHeight) != EOK) {
                cout << "Fatal: memory copy failed Y" << endl;
            }
            // copy UV
            uint32_t uvSize = size - defaultWidth * defaultHeight;
            if (memcpy_s(cropBuffer + defaultWidth * defaultHeight, uvSize,
                buffer->memory_->GetAddr() + defaultWidth * defaultHeight, uvSize) != EOK) {
                cout << "Fatal: memory copy failed UV" << endl;
            }
            SHA512_Update(&g_ctx, cropBuffer, (defaultWidth * defaultHeight * THREE) >> 1);
            delete[] cropBuffer;
        }

        if (vdec_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
    } else {
        if (vdec_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: RenderOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
    }
}

void VDecNdkInnerSample::FlushBuffer()
{
    std::queue<std::shared_ptr<AVBuffer>> empty;
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    swap(empty, signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();

    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
}

void VDecNdkInnerSample::StopInloop()
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

void VDecNdkInnerSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}

void VDecNdkInnerSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

bool VDecNdkInnerSample::MdCompare(unsigned char *buffer, int len, const char *source[])
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