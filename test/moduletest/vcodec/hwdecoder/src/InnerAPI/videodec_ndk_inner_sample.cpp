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
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include "iconsumer_surface.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "native_buffer_inner.h"
#include "display_type.h"
#include "videodec_ndk_inner_sample.h"

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

VDecInnerCallback::VDecInnerCallback(std::shared_ptr<VDecInnerSignal> signal) : innersignal_(signal) {}

void VDecInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VDecInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
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
    cout << "OnOutputBufferAvailable index:" << index << endl;
    unique_lock<mutex> lock(innersignal_->outMutex_);
    innersignal_->outIdxQueue_.push(index);
    innersignal_->infoQueue_.push(info);
    innersignal_->flagQueue_.push(flag);
    innersignal_->outBufferQueue_.push(buffer);
    cout << "**********out info size = " << info.size << endl;
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
    int64_t nanoTime = (int64_t)now.tv_sec * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VDecNdkInnerSample::CreateByMime(const std::string &mime)
{
    vdec_ = VideoDecoderFactory::CreateByMime(mime);
    return vdec_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::CreateByName(const std::string &name)
{
    vdec_ = VideoDecoderFactory::CreateByName(name);
    return vdec_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::Configure()
{
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

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
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
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
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
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
    int32_t ret = vdec_->Release();
    vdec_ = nullptr;
    if (signal_ != nullptr) {
        signal_ = nullptr;
    }
    return ret;
}

int32_t VDecNdkInnerSample::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    return vdec_->QueueInputBuffer(index, info, flag);
}

int32_t VDecNdkInnerSample::GetOutputFormat(Format &format)
{
    return vdec_->GetOutputFormat(format);
}

int32_t VDecNdkInnerSample::ReleaseOutputBuffer(uint32_t index)
{
    return vdec_->ReleaseOutputBuffer(index, false);
}

int32_t VDecNdkInnerSample::SetParameter(const Format &format)
{
    return vdec_->SetParameter(format);
}

int32_t VDecNdkInnerSample::SetCallback()
{
    signal_ = make_shared<VDecInnerSignal>();
    if (signal_ == nullptr) {
        cout << "Failed to new VEncInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    cb_ = make_shared<VDecInnerCallback>(signal_);
    return vdec_->SetCallback(cb_);
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

    inFile_->open(INP_DIR, ios::in | ios::binary);
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
        ReleaseInFile();
        StopInloop();
        Release();
        return AVCS_ERR_UNKNOWN;
    }

    return AVCS_ERR_OK;
}

int32_t VDecNdkInnerSample::RunVideoDecoder(const std::string &codeName)
{
    SF_OUTPUT = false;
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

int32_t VDecNdkInnerSample::PushData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index)
{
    static uint32_t repeat_count = 0;

    if (BEFORE_EOS_INPUT && frameCount > TEN) {
        SetEOS(index);
        return 1;
    }

    if (BEFORE_EOS_INPUT_INPUT && frameCount > TEN) {
        BEFORE_EOS_INPUT_INPUT = false;
    }

    char ch[4] = {};
    (void)inFile_->read(ch, START_CODE_SIZE);
    if (repeatRun && inFile_->eof()) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        cout << "repeat run " << repeat_count << endl;
        repeat_count++;
        return 0;
    }

    if (inFile_->eof()) {
        SetEOS(index);
        return 1;
    }

    uint32_t bufferSize = (uint32_t)(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << EIGHT) | ((ch[1] & 0xFF) << SIXTEEN) |
                                     ((ch[0] & 0xFF) << TWENTY_FOUR));
    if (bufferSize >= DEFAULT_WIDTH * DEFAULT_HEIGHT * THREE >> 1) {
        cout << "read bufferSize abnormal. buffersize = " << bufferSize << endl;
        return 1;
    }

    return SendData(bufferSize, index, buffer);
}

int32_t VDecNdkInnerSample::SendData(uint32_t bufferSize, uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;

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
        flag = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    } else {
        flag = AVCODEC_BUFFER_FLAG_NONE;
    }

    int32_t size = buffer->GetSize();
    if (size < (int32_t)(bufferSize + START_CODE_SIZE)) {
        delete[] fileBuffer;
        cout << "buffer size not enough." << endl;
        return 0;
    }

    uint8_t *avBuffer = buffer->GetBase();
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

    info.presentationTimeUs = GetSystemTimeUs();
    info.size = bufferSize + START_CODE_SIZE;
    info.offset = 0;
    int32_t result = vdec_->QueueInputBuffer(index, info, flag);
    if (result != AVCS_ERR_OK) {
        errCount = errCount + 1;
        cout << "push input data failed,error:" << result << endl;
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

void VDecNdkInnerSample::RepeatStartBeforeEOS()
{
    if (REPEAT_START_FLUSH_BEFORE_EOS > 0) {
        REPEAT_START_FLUSH_BEFORE_EOS--;
        vdec_->Flush();
        FlushBuffer();
        vdec_->Start();
    }

    if (REPEAT_START_STOP_BEFORE_EOS > 0) {
        REPEAT_START_STOP_BEFORE_EOS--;
        vdec_->Stop();
        FlushBuffer();
        vdec_->Start();
    }
}

void VDecNdkInnerSample::SetEOS(uint32_t index)
{
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 0;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;

    int32_t res = vdec_->QueueInputBuffer(index, info, flag);
    cout << "QueueInputBuffer EOS res: " << res << endl;
}

void VDecNdkInnerSample::WaitForEOS()
{
    if (!AFTER_EOS_DESTORY_CODEC && inputLoop_ && inputLoop_->joinable()) {
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
    while (true) {
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
    }
}

void VDecNdkInnerSample::OutputFunc()
{
    SHA512_Init(&g_ctx);
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() {
            return signal_->outIdxQueue_.size() > 0;
        });

        if (!isRunning_.load()) {
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
            if (AFTER_EOS_DESTORY_CODEC) {
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

void VDecNdkInnerSample::ProcessOutputData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index)
{
    if (!SF_OUTPUT) {
        uint32_t size = buffer->GetSize();
        if (size >= DEFAULT_WIDTH * DEFAULT_HEIGHT * THREE >> 1) {
            uint8_t *cropBuffer = new uint8_t[size];
            if (memcpy_s(cropBuffer, size, buffer->GetBase(),
				            DEFAULT_WIDTH * DEFAULT_HEIGHT) != EOK) {
                cout << "Fatal: memory copy failed Y" << endl;
            }
            // copy UV
            uint32_t uvSize = size - DEFAULT_WIDTH * DEFAULT_HEIGHT;
            if (memcpy_s(cropBuffer + DEFAULT_WIDTH * DEFAULT_HEIGHT, uvSize,
				            buffer->GetBase() + DEFAULT_WIDTH * DEFAULT_HEIGHT, uvSize) != EOK) {
                cout << "Fatal: memory copy failed UV" << endl;
            }
            SHA512_Update(&g_ctx, cropBuffer, DEFAULT_WIDTH * DEFAULT_HEIGHT * THREE >> 1);
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

void VDecNdkInnerSample::FlushBuffer()
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
        clearBufferqueue(signal_->infoQueue_);
        clearFlagqueue(signal_->flagQueue_);
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