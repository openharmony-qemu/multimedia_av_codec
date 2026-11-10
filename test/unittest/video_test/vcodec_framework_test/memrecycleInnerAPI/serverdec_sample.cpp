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
#include <chrono>
#include <map>
#include <utility>
#include "iconsumer_surface.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "native_buffer_inner.h"
#include "serverdec_sample.h"
#include "hevc_decoder_api.h"
#include "buffer/avallocator.h"
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
constexpr uint8_t H265_NALU_TYPE = 0x7e;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
constexpr uint8_t SPS = 33;
constexpr uint8_t PPS = 34;
constexpr uint32_t TAKEOVER_INPUT_BUFFER_COUNT = 4;
constexpr int32_t TAKEOVER_INPUT_BUFFER_CAPACITY = 1920 * 1080 * 3 / 2;
constexpr int64_t TAKEOVER_PTS_INTERVAL_US = 33333; // 30 fps
constexpr int32_t TAKEOVER_POLL_INTERVAL_MS = 100;
constexpr int32_t TAKEOVER_FEED_EOS = 1;

SHA512_CTX g_ctx;
unsigned char g_md[SHA512_DIGEST_LENGTH];

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

uint32_t ReadNalSizeBigEndian(const char ch[START_CODE_SIZE])
{
    return ((static_cast<uint32_t>(ch[3]) & 0xff)) |
           ((static_cast<uint32_t>(ch[2]) & 0xff) << EIGHT) |
           ((static_cast<uint32_t>(ch[1]) & 0xff) << SIXTEEN) |
           ((static_cast<uint32_t>(ch[0]) & 0xff) << TWENTY_FOUR);
}

} // namespace

VDecInnerCallback::VDecInnerCallback(std::shared_ptr<VDecInnerSignal> signal) : innersignal_(signal) {}

void VDecInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error errorType:" << errorType << " errorCode:" << errorCode << endl;
    hasError_.store(true);
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
    if ((fileBuffer[START_CODE_SIZE] & H265_NALU_TYPE) == SPS ||
        (fileBuffer[START_CODE_SIZE] & H265_NALU_TYPE) == PPS) {
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
            cout << "Fatal: ReleaseOutputBuffer fail" << endl; // RenderOutputBuffer 同线程同步直调会死锁
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

int32_t VDecNdkInnerSample::CreateHevcDecoderByNameAndInit(const std::string &codeName)
{
    CreateHevcDecoderByName(codeName, vdec_);
    if (vdec_ == nullptr) {
        cout << "Takeover: create hevc video decoder failed" << endl;
        return AVCS_ERR_INVALID_OPERATION;
    }
    Media::Meta codecInfo;
    int32_t ret = vdec_->Init(codecInfo);
    if (ret != AVCS_ERR_OK) {
        cout << "Takeover: init hevc video decoder failed, ret = " << ret << endl;
        Release();
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::ConfigureDecoder()
{
    if (vdec_ == nullptr) {
        return AVCS_ERR_INVALID_OPERATION;
    }
    return Configure();
}

int32_t VDecNdkInnerSample::QueueTakeoverInputBuffers(void)
{
    if (vdec_ == nullptr || takeoverPool_.empty()) {
        return AVCS_ERR_INVALID_OPERATION;
    }

    // 接管首 Start 软解不重复上报，业务侧直接把持有的接管池入队（内容原样保留，含首块 XPS)
    for (auto &item : takeoverPool_) {
        int32_t ret = vdec_->QueueInputBuffer(item.first);
        if (ret != AVCS_ERR_OK) {
            cout << "Takeover: queue takeover input buffer failed, index = " << item.first << " ret = " << ret << endl;
            return ret;
        }
    }
    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::BuildTakeoverInputPool()
{
    takeoverPool_.clear();
    for (uint32_t i = 0; i < TAKEOVER_INPUT_BUFFER_COUNT; i++) {
        char lenBuf[START_CODE_SIZE] = {0};
        (void)inFile_->read(lenBuf, START_CODE_SIZE);
        if (inFile_->eof() || inFile_->gcount() < static_cast<int64_t>(START_CODE_SIZE)) {
            cout << "Takeover: no enough NAL units in file, got " << i << endl;
            return AVCS_ERR_UNKNOWN;
        }
        uint32_t nalSize = ReadNalSizeBigEndian(lenBuf);
        if (nalSize == 0 || nalSize + START_CODE_SIZE > static_cast<uint32_t>(TAKEOVER_INPUT_BUFFER_CAPACITY)) {
            cout << "Takeover: abnormal NAL size " << nalSize << endl;
            return AVCS_ERR_UNKNOWN;
        }
        std::shared_ptr<AVAllocator> allocator =
            AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        if (allocator == nullptr) {
            cout << "Takeover: create allocator failed" << endl;
            return AVCS_ERR_NO_MEMORY;
        }
        std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(allocator, TAKEOVER_INPUT_BUFFER_CAPACITY);
        if (avBuffer == nullptr || avBuffer->memory_ == nullptr) {
            cout << "Takeover: create avBuffer failed, index = " << i << endl;
            return AVCS_ERR_NO_MEMORY;
        }
        uint8_t *addr = avBuffer->memory_->GetAddr();
        if (addr == nullptr) {
            cout << "Takeover: AVBuffer addr is null, index = " << i << endl;
            return AVCS_ERR_NO_MEMORY;
        }
        if (memcpy_s(addr, TAKEOVER_INPUT_BUFFER_CAPACITY, START_CODE, START_CODE_SIZE) != EOK) {
            cout << "Takeover: memcpy start code failed, index = " << i << endl;
            return AVCS_ERR_UNKNOWN;
        }
        (void)inFile_->read(reinterpret_cast<char *>(addr + START_CODE_SIZE), nalSize);
        if (inFile_->gcount() < static_cast<int64_t>(nalSize)) {
            cout << "Takeover: read NAL data failed, index = " << i << endl;
            return AVCS_ERR_UNKNOWN;
        }
        if ((addr[START_CODE_SIZE] & H265_NALU_TYPE) == SPS || (addr[START_CODE_SIZE] & H265_NALU_TYPE) == PPS) {
            avBuffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        } else {
            avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        }
        avBuffer->pts_ = static_cast<int64_t>(i) * TAKEOVER_PTS_INTERVAL_US;
        avBuffer->memory_->SetOffset(0);
        avBuffer->memory_->SetSize(nalSize + START_CODE_SIZE);
        takeoverPool_[i] = avBuffer;
        cout << "Takeover: build external buffer index = " << i << ", nalSize = " << nalSize << endl;
    }
    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::PrepareTakeoverInputPool()
{
    takeoverPool_.clear();
    takeoverSnapshot_.clear();
    takeoverReturnedCount_ = 0;
    takeoverOutCount_ = 0;
    takeoverAddrCheckPass_ = true;
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        return AVCS_ERR_NO_MEMORY;
    }
    inFile_->open(inputDir, ios::binary | ios::in);
    if (!inFile_->is_open()) {
        cout << "Takeover: open input file failed: " << inputDir << endl;
        ReleaseInFile();
        return AVCS_ERR_UNKNOWN;
    }
    int32_t ret = BuildTakeoverInputPool();
    if (ret != AVCS_ERR_OK) {
        ReleaseInFile();
        return ret;
    }
    takeoverSnapshot_ = takeoverPool_;
    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::SetTakeoverInputBuffers(std::map<uint32_t, std::shared_ptr<AVBuffer>> &pool)
{
    if (vdec_ == nullptr) {
        return AVCS_ERR_INVALID_OPERATION;
    }
    return vdec_->SetInputBuffers(pool);
}

bool VDecNdkInnerSample::CheckTakeoverReturned(uint32_t index, const std::shared_ptr<AVBuffer> &buffer)
{
    auto it = takeoverSnapshot_.find(index);
    if (it == takeoverSnapshot_.end()) {
        cout << "Takeover: returned index " << index << " not in snapshot pool" << endl;
        return false;
    }
    if (buffer == nullptr || buffer.get() != it->second.get() || buffer->memory_ == nullptr ||
        it->second->memory_ == nullptr || buffer->memory_->GetAddr() != it->second->memory_->GetAddr()) {
        cout << "Takeover: returned buffer mismatch, index = " << index << endl;
        return false;
    }
    return true;
}

int32_t VDecNdkInnerSample::WaitTakeoverPendingReturned(int32_t timeoutMs)
{
    if (vdec_ == nullptr || cb_ == nullptr || signal_ == nullptr) {
        return AVCS_ERR_INVALID_OPERATION;
    }
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (takeoverReturnedCount_ < TAKEOVER_INPUT_BUFFER_COUNT && !cb_->HasError() &&
        std::chrono::steady_clock::now() < deadline) {
        uint32_t index = 0;
        std::shared_ptr<AVBuffer> buffer = nullptr;
        bool hasInput = false;
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            hasInput = signal_->inCond_.wait_for(lock, std::chrono::milliseconds(TAKEOVER_POLL_INTERVAL_MS),
                [&]() { return !signal_->inIdxQueue_.empty(); });
            if (hasInput) {
                index = signal_->inIdxQueue_.front();
                buffer = signal_->inBufferQueue_.front();
                signal_->inIdxQueue_.pop();
                signal_->inBufferQueue_.pop();
            }
        }
        if (hasInput) {
            if (!CheckTakeoverReturned(index, buffer)) {
                takeoverAddrCheckPass_ = false;
                break;
            }
            takeoverReturnedCount_++;
        }
    }
    if (cb_->HasError()) {
        cout << "Takeover: decoder reported error" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    if (!takeoverAddrCheckPass_) {
        cout << "Takeover: returned index/buffer/addr check failed" << endl;
        return AVCS_ERR_INVALID_VAL;
    }
    if (takeoverReturnedCount_ != TAKEOVER_INPUT_BUFFER_COUNT) {
        cout << "Takeover: expect " << TAKEOVER_INPUT_BUFFER_COUNT << " pending returns, got "
            << takeoverReturnedCount_ << endl;
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::FeedTakeoverBuffer(std::shared_ptr<AVBuffer> buffer, uint32_t index)
{
    char lenBuf[START_CODE_SIZE] = {0};
    (void)inFile_->read(lenBuf, START_CODE_SIZE);
    if (inFile_->eof() || inFile_->gcount() < static_cast<int64_t>(START_CODE_SIZE)) {
        buffer->pts_ = 0;
        buffer->memory_->SetOffset(0);
        buffer->memory_->SetSize(0);
        buffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        return vdec_->QueueInputBuffer(index) == AVCS_ERR_OK ? TAKEOVER_FEED_EOS : AVCS_ERR_UNKNOWN;
    }
    uint32_t nalSize = ReadNalSizeBigEndian(lenBuf);
    if (nalSize == 0 || nalSize + START_CODE_SIZE > static_cast<uint32_t>(TAKEOVER_INPUT_BUFFER_CAPACITY)) {
        cout << "Takeover: abnormal NAL size " << nalSize << ", stop feeding" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    uint8_t *addr = buffer->memory_->GetAddr();
    if (addr == nullptr || memcpy_s(addr, TAKEOVER_INPUT_BUFFER_CAPACITY, START_CODE, START_CODE_SIZE) != EOK) {
        cout << "Takeover: write start code failed"<< endl;
        return AVCS_ERR_UNKNOWN;
    }
    (void)inFile_->read(reinterpret_cast<char *>(addr + START_CODE_SIZE), nalSize);
    if (inFile_->gcount() < static_cast<int64_t>(nalSize)) {
        return AVCS_ERR_UNKNOWN;
    }
    if ((addr[START_CODE_SIZE] & H265_NALU_TYPE) == SPS || (addr[START_CODE_SIZE] & H265_NALU_TYPE) == PPS) {
        buffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    } else {
        buffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
    }
    buffer->pts_ = GetSystemTimeUs();
    buffer->memory_->SetOffset(0);
    buffer->memory_->SetSize(nalSize + START_CODE_SIZE);
    if (vdec_->QueueInputBuffer(index) != AVCS_ERR_OK) {
        cout << "Takeover: queue input buffer failed, index = " << index << endl;
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::FeedTakeoverUntilEos(int32_t timeoutMs)
{
    if (vdec_ == nullptr || cb_ == nullptr || signal_ == nullptr) {
        return AVCS_ERR_INVALID_OPERATION;
    }
    bool eosDone = false;
    bool eosSubmitted = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!eosDone && !cb_->HasError() && std::chrono::steady_clock::now() < deadline) {
        uint32_t index = 0;
        std::shared_ptr<AVBuffer> buffer = nullptr;
        bool hasInput = false;
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            hasInput = signal_->inCond_.wait_for(lock, std::chrono::milliseconds(TAKEOVER_POLL_INTERVAL_MS),
                [&]() { return !signal_->inIdxQueue_.empty(); });
            if (hasInput) {
                index = signal_->inIdxQueue_.front();
                buffer = signal_->inBufferQueue_.front();
                signal_->inIdxQueue_.pop();
                signal_->inBufferQueue_.pop();
            }
        }
        if (hasInput) {
            if (!CheckTakeoverReturned(index, buffer)) {
                takeoverAddrCheckPass_ = false;
                break;
            }
            if (!eosSubmitted) {
                int32_t ret = FeedTakeoverBuffer(buffer, index);
                if (ret == TAKEOVER_FEED_EOS) {
                    eosSubmitted = true;
                } else if (ret != AVCS_ERR_OK) {
                    break;
                }
            }
        }
        while (!eosDone) {
            uint32_t outIndex = 0;
            std::shared_ptr<AVBuffer> outBuffer = nullptr;
            {
                unique_lock<mutex> lock(signal_->outMutex_);
                if (signal_->outIdxQueue_.empty()) {
                    break;
                }
                outIndex = signal_->outIdxQueue_.front();
                outBuffer = signal_->outBufferQueue_.front();
                signal_->outIdxQueue_.pop();
                signal_->outBufferQueue_.pop();
            }
            if (outBuffer != nullptr && (outBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) != 0) {
                eosDone = true;
                break;
            }
            if (vdec_->ReleaseOutputBuffer(outIndex) == AVCS_ERR_OK) {
                takeoverOutCount_++;
            }
        }
    }
    if (cb_->HasError()) {
        cout << "Takeover: decoder reported error" << endl;
        return AVCS_ERR_UNKNOWN;
    }
    if (!takeoverAddrCheckPass_) {
        cout << "Takeover: returned index/buffer/addr check failed" << endl;
        return AVCS_ERR_INVALID_VAL;
    }
    if (!eosDone) {
        cout << "Takeover: no EOS output, out frames=" << takeoverOutCount_ << endl;
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}