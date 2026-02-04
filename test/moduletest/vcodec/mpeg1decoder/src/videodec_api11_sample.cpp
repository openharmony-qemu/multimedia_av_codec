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
#include <random>
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "videodec_api11_sample.h"
#include "media_description.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t MICRO_IN_SECOND = 1000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr int32_t THREE = 3;
constexpr int32_t EIGHT = 8;
constexpr int32_t TEN = 10;
constexpr int32_t SIXTEEN = 16;
constexpr int32_t TWENTY_FOUR = 24;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr int32_t DEFAULT_ANGLE = 90;
constexpr uint32_t MILLION = 1000000;
SHA512_CTX g_c;
unsigned char g_md[SHA512_DIGEST_LENGTH];
VDecAPI11Sample *dec_sample = nullptr;
int32_t g_strideSurface = 0;
int32_t g_sliceSurface = 0;
bool g_yuvSurface = false;
constexpr int32_t MIN_RANGE = 2;
constexpr int32_t MAX_RANGE = 4096;
constexpr int32_t MIN_FRANGE = 1;
constexpr int32_t MAX_FRANGE = 60;
constexpr int32_t EVEN_NUMBER = 2;
static const std::vector<uint32_t> mpeg1FrameSizes = {
    37704, 54570, 8120, 3154, 1145, 1968, 769, 578, 640, 558,
    622, 670, 15505, 574, 316, 461, 442, 416, 489, 628,
    606, 447, 481, 563, 12427, 427, 400, 634, 655, 615,
    641, 605, 535, 590, 635, 762, 12452, 692, 637, 694,
    632, 648, 797, 1001, 879, 895, 813, 830, 12558, 722,
    795, 843, 987, 968, 909, 924, 908, 1016, 929, 1012,
    12587, 842, 852, 849, 892, 1019, 1011, 994, 1081, 1013,
    1099, 1029, 12631, 895, 906, 1158, 1137, 1123, 1075, 1055,
    1190, 1227, 1301, 1394, 12771, 1071, 1199, 1240, 1255, 1288,
    1247, 1332, 1245, 1264, 1340, 1303, 12748, 1109, 1195, 1225,
    1173, 1299, 1279, 1306, 1274, 1442, 1291, 1300, 12616, 1080,
    1178, 1194, 1210, 1385, 1217, 1294, 1384, 1323, 1295, 1266,
    12558, 1159, 1201, 1342, 1410, 1376, 1358, 1276, 1501, 1373,
    1426, 1574, 12337, 1238, 1196, 1198, 1278, 1164, 1239, 1438,
    1475, 1296, 1259, 1377, 12138, 1070, 1009, 1191, 1211, 1161,
    1178, 1161, 1333, 1269, 1130, 1346, 11975, 1027, 1178, 1020,
    1157, 1037, 1108, 1215, 1221, 1300, 1071, 1169, 11758, 870,
    875, 1025, 1031, 1043, 968, 956, 984, 914, 1112, 1052,
    11635, 949
};
void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void clearBufferqueue(std::queue<OH_AVCodecBufferAttr> &q)
{
    std::queue<OH_AVCodecBufferAttr> empty;
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
        if (buffer == nullptr) {
            cout << "surface is nullptr" << endl;
            return;
        } else {
            int32_t frameSize = (g_strideSurface * g_sliceSurface * THREE) >> 1;
            if (g_yuvSurface && outFile_ != nullptr && frameSize <= buffer->GetSize()) {
                outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), frameSize);
            }
        }
        cs->ReleaseBuffer(buffer, -1);
    }

private:
    int64_t timestamp = 0;
    Rect damage = {};
    sptr<Surface> cs {nullptr};
    std::unique_ptr<std::ofstream> outFile_;
};

VDecAPI11Sample::~VDecAPI11Sample()
{
    isStarted_ = false;
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }

    if (sourceFormat_ != nullptr) {
        OH_AVFormat_Destroy(sourceFormat_);
        sourceFormat_ = nullptr;
    }

    if (memory_ != nullptr) {
        OH_AVMemory_Destroy(memory_);
        memory_ = nullptr;
    }
    if (videoSource_ != nullptr) {
        OH_AVSource_Destroy(videoSource_);
        videoSource_ = nullptr;
    }
    if (demuxer_ != nullptr) {
        OH_AVDemuxer_Destroy(demuxer_);
        demuxer_ = nullptr;
    }
    if (videoAvBuffer_ != nullptr) {
        OH_AVBuffer_Destroy(videoAvBuffer_);
        videoAvBuffer_ = nullptr;
    }
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    g_yuvSurface = false;
    for (int i = 0; i < maxSurfNum_; i++) {
        if (nativeWindow[i]) {
            OH_NativeWindow_DestroyNativeWindow(nativeWindow[i]);
            nativeWindow[i] = nullptr;
        }
    }
    Stop();
    Release();
}

void VdecAPI11Error(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    dec_sample->isRunning.store(false);
    dec_sample->signal->inCond.notify_all();
    dec_sample->signal->outCond.notify_all();
    dec_sample->isonError = true;
    cout << "Error errorCode=" << errorCode << endl;
}

void VdecAPI11FormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int32_t stride = 0;
    int32_t sliceHeight = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &currentWidth);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &currentHeight);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_STRIDE, &stride);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_SLICE_HEIGHT, &sliceHeight);
    dec_sample->defaultWidth = currentWidth;
    dec_sample->defaultHeight = currentHeight;
    g_strideSurface = stride;
    g_sliceSurface = sliceHeight;
    if (stride <= 0 || sliceHeight <= 0) {
        dec_sample->errCount++;
    }
}

void VdecAPI11InputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    if (dec_sample->inputCallbackFlush && dec_sample->outCount > 1) {
        OH_VideoDecoder_Flush(codec);
        cout << "OH_VideoDecoder_Flush end" << endl;
        dec_sample->isRunning.store(false);
        dec_sample->signal->inCond.notify_all();
        dec_sample->signal->outCond.notify_all();
        return;
    }
    if (dec_sample->inputCallbackStop && dec_sample->outCount > 1) {
        OH_VideoDecoder_Stop(codec);
        cout << "OH_VideoDecoder_Stop end" << endl;
        dec_sample->isRunning.store(false);
        dec_sample->signal->inCond.notify_all();
        dec_sample->signal->outCond.notify_all();
        return;
    }
    VDecAPI11Signal *signal = static_cast<VDecAPI11Signal *>(userData);
    unique_lock<mutex> lock(signal->inMutex);
    signal->inIdxQueue.push(index);
    signal->inBufferQueue.push(data);
    signal->inCond.notify_all();
}


int32_t VDecAPI11Sample::ConfigureVideoDecoderNoPixelFormat()
{
    if (autoSwitchSurface) {
        switchSurfaceFlag_ = (switchSurfaceFlag_ == 1) ? 0 : 1;
        if (OH_VideoDecoder_SetSurface(vdec_, nativeWindow[switchSurfaceFlag_]) != AV_ERR_INVALID_STATE) {
            errCount++;
        }
    }
    if (trackFormat == nullptr) {
        trackFormat = OH_AVFormat_Create();
    }
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_ENABLE_SYNC_MODE, enbleSyncMode);
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, enbleBlankFrame_);
    int ret = OH_VideoDecoder_Configure(vdec_, trackFormat);
    return ret;
}

void VdecAPI11OutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    if (dec_sample->outputCallbackFlush && dec_sample->outCount > 1) {
        OH_VideoDecoder_Flush(codec);
        cout << "OH_VideoDecoder_Flush end" << endl;
        dec_sample->isRunning.store(false);
        dec_sample->signal->inCond.notify_all();
        dec_sample->signal->outCond.notify_all();
        return;
    }
    if (dec_sample->outputCallbackStop && dec_sample->outCount > 1) {
        OH_VideoDecoder_Stop(codec);
        cout << "OH_VideoDecoder_Stop end" << endl;
        dec_sample->isRunning.store(false);
        dec_sample->signal->inCond.notify_all();
        dec_sample->signal->outCond.notify_all();
        return;
    }
    VDecAPI11Signal *signal = static_cast<VDecAPI11Signal *>(userData);
    unique_lock<mutex> lock(signal->outMutex);
    signal->outIdxQueue.push(index);
    signal->outBufferQueue.push(data);
    signal->outCond.notify_all();
}

void VDecAPI11Sample::Flush_buffer()
{
    unique_lock<mutex> inLock(signal->inMutex);
    clearIntqueue(signal->inIdxQueue);
    std::queue<OH_AVBuffer *> empty;
    swap(empty, signal->inBufferQueue);
    signal->inCond.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal->outMutex);
    clearIntqueue(signal->outIdxQueue);
    clearBufferqueue(signal->attrQueue);
    signal->outCond.notify_all();
    outLock.unlock();
}

bool VDecAPI11Sample::MdCompare(unsigned char buffer[], int len, const char *source[])
{
    bool result = true;
    for (int i = 0; i < len; i++) {
        unsigned char source_byte = std::stoul(source[i], nullptr, 16);
        if (buffer[i] != source_byte) {
            cout << "dismatch sign: buffer=" << buffer[i] << ", source_type=" << source_byte << endl;
            return false;
        }
    }
    return result;
}

int64_t VDecAPI11Sample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = (int64_t)now.tv_sec * NANOS_IN_SECOND + now.tv_nsec;
    return nanoTime / NANOS_IN_MICRO;
}

int32_t VDecAPI11Sample::ConfigureVideoDecoder()
{
    if (autoSwitchSurface) {
        switchSurfaceFlag_ = (switchSurfaceFlag_ == 1) ? 0 : 1;
        if (OH_VideoDecoder_SetSurface(vdec_, nativeWindow[switchSurfaceFlag_]) != AV_ERR_INVALID_STATE) {
            errCount++;
        }
    }
    if (trackFormat == nullptr) {
        trackFormat = OH_AVFormat_Create();
    }
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PIXEL_FORMAT, defaultPixelFormat);
    (void)OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_ENABLE_SYNC_MODE, enbleSyncMode);
    (void)OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, enbleBlankFrame_);
    int ret = OH_VideoDecoder_Configure(vdec_, trackFormat);
    return ret;
}

void VDecAPI11Sample::CreateSurface()
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
    sptr<IBufferConsumerListener> listener = new ConsumerListenerBuffer(cs[0], outputDir);
    cs[0]->RegisterConsumerListener(listener);
    auto p = cs[0]->GetProducer();
    ps[0] = Surface::CreateSurfaceAsProducer(p);
    nativeWindow[0] = CreateNativeWindowFromSurface(&ps[0]);
    if (autoSwitchSurface)  {
        cs[1] = Surface::CreateSurfaceAsConsumer();
        sptr<IBufferConsumerListener> listener2 = new ConsumerListenerBuffer(cs[1], outputDir2);
        cs[1]->RegisterConsumerListener(listener2);
        auto p2 = cs[1]->GetProducer();
        ps[1] = Surface::CreateSurfaceAsProducer(p2);
        nativeWindow[1] = CreateNativeWindowFromSurface(&ps[1]);
    }
}

int32_t VDecAPI11Sample::RunVideoDec_Surface(string codeName)
{
    sfOutput = true;
    int err = AV_ERR_OK;
    CreateSurface();
    if (!nativeWindow[0]) {
        cout << "Failed to create surface" << endl;
        return AV_ERR_UNKNOWN;
    }
    err = CreateVideoDecoder(codeName);
    if (err != AV_ERR_OK) {
        cout << "Failed to create video decoder" << endl;
        return err;
    }
    err = SetVideoDecoderCallback();
    if (err != AV_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return err;
    }
    err = ConfigureVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return err;
    }
    err = OH_VideoDecoder_SetSurface(vdec_, nativeWindow[0]);
    if (err != AV_ERR_OK) {
        cout << "Failed to set surface" << endl;
        return err;
    }
    err = StartVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return err;
    }
    return err;
}

int32_t VDecAPI11Sample::RunVideoDec(string codeName)
{
    sfOutput = false;
    int err = CreateVideoDecoder(codeName);
    if (err != AV_ERR_OK) {
        cout << "Failed to create video decoder" << endl;
        return err;
    }

    err = ConfigureVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return err;
    }

    err = SetVideoDecoderCallback();
    if (err != AV_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return err;
    }

    err = StartVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return err;
    }
    return err;
}

int32_t VDecAPI11Sample::SetVideoDecoderCallback()
{
    signal = new VDecAPI11Signal();
    if (signal == nullptr) {
        cout << "Failed to new VDecAPI11Signal" << endl;
        return AV_ERR_UNKNOWN;
    }

    cb_.onError = VdecAPI11Error;
    cb_.onStreamChanged = VdecAPI11FormatChanged;
    cb_.onNeedInputBuffer = VdecAPI11InputDataReady;
    cb_.onNewOutputBuffer = VdecAPI11OutputDataReady;
    return OH_VideoDecoder_RegisterCallback(vdec_, cb_, static_cast<void *>(signal));
}

void VDecAPI11Sample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VDecAPI11Sample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal->inMutex);
        clearIntqueue(signal->inIdxQueue);
        isRunning.store(false);
        signal->inCond.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_.reset();
    }
}

int32_t VDecAPI11Sample::CreateVideoDecoder(string codeName)
{
    vdec_ = OH_VideoDecoder_CreateByName(codeName.c_str());
    dec_sample = this;
    return vdec_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

int32_t VDecAPI11Sample::StartVideoDecoderReadStream()
{
    isRunning.store(true);
    int ret = OH_VideoDecoder_Start(vdec_);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning.store(false);
        ReleaseInFile();
        Release();
        return ret;
    }
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(inputDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "failed open file " << inputDir << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    return inputoutputloop();
}

int32_t VDecAPI11Sample::inputoutputloop()
{
    inputLoop_ = make_unique<thread>(&VDecAPI11Sample::InputReadStreamFuncTest, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VDecAPI11Sample::OutputFuncTest, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

void VDecAPI11Sample::InputReadStreamFuncTest()
{
    if (outputYuvSurface) {
        g_yuvSurface = true;
    }
    bool flag = true;
    while (flag) {
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        uint32_t index;
        unique_lock<mutex> lock(signal->inMutex);
        signal->inCond.wait(lock, [this]() {
            if (!isRunning.load()) {
                return true;
            }
            return signal->inIdxQueue.size() > 0;
        });
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        index = signal->inIdxQueue.front();
        auto buffer = signal->inBufferQueue.front();
        signal->inIdxQueue.pop();
        signal->inBufferQueue.pop();
        lock.unlock();
        int ret = PushDataReadStream(index, buffer);
        if (ret == 1) {
            flag = false;
            break;
        }
        if (sleepOnFPS) {
            usleep(MICRO_IN_SECOND / (int32_t)defaultFrameRate);
        }
    }
}

int32_t VDecAPI11Sample::PushDataReadStream(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    if (beforeEosInput_ && frameCount_ > TEN) {
        SetEOS(index, buffer);
        return 1;
    }
    if (beforeEosInputInput_ && frameCount_ > TEN) {
        memset_s(&attr, sizeof(OH_AVCodecBufferAttr), 0, sizeof(OH_AVCodecBufferAttr));
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        beforeEosInputInput_ = false;
    }

    if (repeatRun_ && frameCount_ >= mpeg1FrameSizes.size()) {
        static uint32_t repeat_count = 0;
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        frameCount_ = 0;
        cout << "repeat run " << repeat_count << endl;
        repeat_count++;
        return 0;
    }

    if (frameCount_ >= mpeg1FrameSizes.size()) {
        cout << "SetEOS frameCount_: " << frameCount_ << endl;
        SetEOS(index, buffer);
        return 1;
    }
    uint32_t frameSize = mpeg1FrameSizes[frameCount_];
    return SendData(frameSize, index, buffer);
}

int32_t VDecAPI11Sample::StartVideoDecoder()
{
    isRunning.store(true);
    int ret = OH_VideoDecoder_Start(vdec_);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning.store(false);
        ReleaseInFile();
        Release();
        return ret;
    }
    inputLoop_ = make_unique<thread>(&VDecAPI11Sample::InputFuncTest, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VDecAPI11Sample::OutputFuncTest, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }

    return AV_ERR_OK;
}

int32_t VDecAPI11Sample::StartSyncVideoDecoder()
{
    isRunning.store(true);
    int ret = OH_VideoDecoder_Start(vdec_);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning.store(false);
        ReleaseInFile();
        Release();
        return ret;
    }
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(inputDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "failed open file " << inputDir << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    signal = new VDecAPI11Signal();
    return inputoutputsyncloop();
}

int32_t VDecAPI11Sample::inputoutputsyncloop()
{
    inputLoop_ = make_unique<thread>(&VDecAPI11Sample::SyncInputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VDecAPI11Sample::SyncOutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }

    return AV_ERR_OK;
}

void VDecAPI11Sample::testAPI()
{
    cs[0] = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new ConsumerListenerBuffer(cs[0], outputDir);
    cs[0]->RegisterConsumerListener(listener);
    auto p = cs[0]->GetProducer();
    ps[0] = Surface::CreateSurfaceAsProducer(p);
    nativeWindow[0] = CreateNativeWindowFromSurface(&ps[0]);
    OH_VideoDecoder_SetSurface(vdec_, nativeWindow[0]);

    OH_VideoDecoder_Prepare(vdec_);
    OH_VideoDecoder_Start(vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    OH_VideoDecoder_SetParameter(vdec_, format);
    OH_AVFormat_Destroy(format);
    OH_VideoDecoder_GetOutputDescription(vdec_);
    OH_VideoDecoder_Flush(vdec_);
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Reset(vdec_);
    bool isvalid = false;
    OH_VideoDecoder_IsValid(vdec_, &isvalid);
}

void VDecAPI11Sample::WaitForEOS()
{
    if (!afterEosDestroyCodec && inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }

    if (outputLoop_ && outputLoop_->joinable()) {
        outputLoop_->join();
    }
}

void VDecAPI11Sample::InputFuncTest()
{
    if (outputYuvSurface) {
        g_yuvSurface = true;
    }
    bool flag = true;
    while (flag) {
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        uint32_t index;
        unique_lock<mutex> lock(signal->inMutex);
        signal->inCond.wait(lock, [this]() {
            if (!isRunning.load()) {
                return true;
            }
            return signal->inIdxQueue.size() > 0;
        });
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        index = signal->inIdxQueue.front();
        auto buffer = signal->inBufferQueue.front();
        signal->inIdxQueue.pop();
        signal->inBufferQueue.pop();
        lock.unlock();
        int ret = PushData(index, reinterpret_cast<OH_AVBuffer *>(buffer));
        if (ret == 1) {
            flag = false;
            break;
        }
        if (sleepOnFPS) {
            usleep(MICRO_IN_SECOND / (int32_t)defaultFrameRate);
        }
    }
}

int32_t VDecAPI11Sample::PushData(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    OH_AVDemuxer_ReadSampleBuffer(demuxer_, videoTrackId_, buffer);
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        sleep(1);
        SetEOS(index, buffer);
        return 1;
    }
    if (isRunning.load()) {
        OH_VideoDecoder_PushInputBuffer(vdec_, index) == AV_ERR_OK ? (0) : (errCount++);
        frameCount_ = frameCount_ + 1;
        outCount = outCount + 1;
        if (autoSwitchSurface && (frameCount_ % (int32_t)defaultFrameRate == 0)) {
            switchSurfaceFlag_ = (switchSurfaceFlag_ == 1) ? 0 : 1;
            OH_VideoDecoder_SetSurface(vdec_, nativeWindow[switchSurfaceFlag_]) == AV_ERR_OK ? (0) : (errCount++);
        }
    }
    return 0;
}

void VDecAPI11Sample::SyncInputFunc()
{
    if (outputYuvSurface) {
        g_yuvSurface = true;
    }
    bool flag = true;
    while (flag) {
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        uint32_t index;
        if (OH_VideoDecoder_QueryInputBuffer(vdec_, &index, syncInputWaitTime_) != AV_ERR_OK) {
            cout << "OH_VideoDecoder_QueryInputBuffer fail" << endl;
            continue;
        }
        OH_AVBuffer *buffer = OH_VideoDecoder_GetInputBuffer(vdec_, index);
        if (buffer == nullptr) {
            cout << "OH_VideoDecoder_GetInputBuffer fail" << endl;
            errCount = errCount + 1;
            continue;
        }
        if (!inFile_->eof()) {
            int ret = PushDataReadStream(index, buffer);
            if (ret == 1) {
                flag = false;
                break;
            }
        }
        if (sleepOnFPS) {
            usleep(MICRO_IN_SECOND / (int32_t)defaultFrameRate);
        }
    }
}

int32_t VDecAPI11Sample::CheckAndReturnBufferSize(OH_AVBuffer *buffer)
{
    int32_t size = OH_AVBuffer_GetCapacity(buffer);
    if (maxInputSize_ > 0 && (size > maxInputSize_)) {
        errCount++;
    }
    return size;
}

void VDecAPI11Sample::SetAttr(OH_AVCodecBufferAttr &attr, uint32_t bufferSize)
{
    int64_t startPts = GetSystemTimeUs();
    attr.pts = startPts;
    attr.size = bufferSize;
    attr.offset = 0;
}
uint32_t VDecAPI11Sample::SendData(uint32_t bufferSize, uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    uint8_t *fileBuffer = nullptr;
    if (bufferSize > 0) {
        fileBuffer = new uint8_t[bufferSize];
    } else {
        delete[] fileBuffer;
        return 0;
    }
    (void)inFile_->read(reinterpret_cast<char *>(fileBuffer), bufferSize);
    if (bufferSize != 0) {
        attr.flags = AVCODEC_BUFFER_FLAGS_SYNC_FRAME + AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    } else {
        SetEOS(index, buffer);
        return 1;
    }
    int32_t size = CheckAndReturnBufferSize(buffer);
    if (size < bufferSize) {
        delete[] fileBuffer;
        return 0;
    }
    uint8_t *avBuffer = OH_AVBuffer_GetAddr(buffer);
    if (avBuffer == nullptr) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        delete[] fileBuffer;
        return 0;
    }
    if (memcpy_s(avBuffer, size, fileBuffer, bufferSize) != EOK) {
        delete[] fileBuffer;
        return 0;
    }
    SetAttr(attr, bufferSize);
    if (isRunning.load()) {
        OH_AVBuffer_SetBufferAttr(buffer, &attr);
        OH_VideoDecoder_PushInputBuffer(vdec_, index) == AV_ERR_OK ? (0) : (errCount++);
        frameCount_ = frameCount_ + 1;
        outCount = outCount + 1;
        if (autoSwitchSurface && (frameCount_ % (int32_t)defaultFrameRate == 0)) {
            switchSurfaceFlag_ = (switchSurfaceFlag_ == 1) ? 0 : 1;
            OH_VideoDecoder_SetSurface(vdec_, nativeWindow[switchSurfaceFlag_]) == AV_ERR_OK ? (0) : (errCount++);
        }
    }
    delete[] fileBuffer;
    return 0;
}

void VDecAPI11Sample::CheckOutputDescription()
{
    OH_AVFormat *newFormat = OH_VideoDecoder_GetOutputDescription(vdec_);
    if (newFormat != nullptr) {
        int32_t cropTop = 0;
        int32_t cropBottom = 0;
        int32_t cropLeft = 0;
        int32_t cropRight = 0;
        int32_t stride = 0;
        int32_t sliceHeight = 0;
        int32_t picWidth = 0;
        int32_t picHeight = 0;
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_CROP_TOP, &cropTop);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_CROP_BOTTOM, &cropBottom);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_CROP_LEFT, &cropLeft);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_CROP_RIGHT, &cropRight);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_STRIDE, &stride);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_SLICE_HEIGHT, &sliceHeight);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_PIC_WIDTH, &picWidth);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_PIC_HEIGHT, &picHeight);
        if (cropTop != expectCropTop_ || cropBottom != expectCropBottom_ || cropLeft != expectCropLeft_) {
            std::cout << "cropTop:" << cropTop << " cropBottom:" << cropBottom << " cropLeft:" << cropLeft <<std::endl;
            errCount++;
        }
        if (cropRight != expectCropRight_ || stride <= 0 || sliceHeight <= 0) {
            std::cout << "cropRight:" << cropRight << std::endl;
            std::cout << "stride:" << stride << " sliceHeight:" << sliceHeight << std::endl;
            errCount++;
        }
        if (picWidth != originalWidth_ || picHeight != originalHeight_) {
            std::cout << "picWidth:" << picWidth << " picHeight:" << picHeight << std::endl;
            errCount++;
        }
    } else {
        errCount++;
    }
    OH_AVFormat_Destroy(newFormat);
}

void VDecAPI11Sample::AutoSwitchSurface()
{
    if (autoSwitchSurface) {
        switchSurfaceFlag_ = (switchSurfaceFlag_ == 1) ? 0 : 1;
        if (OH_VideoDecoder_SetSurface(vdec_, nativeWindow[switchSurfaceFlag_]) != AV_ERR_OK) {
            errCount++;
        }
        OH_AVFormat *format = OH_AVFormat_Create();
        int32_t angle = DEFAULT_ANGLE * reinterpret_cast<int32_t>(switchSurfaceFlag_);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, angle);
        OH_VideoDecoder_SetParameter(vdec_, format);
        OH_AVFormat_Destroy(format);
    }
}

int32_t VDecAPI11Sample::CheckAttrFlag(OH_AVCodecBufferAttr attr)
{
    if (needCheckOutputDesc_) {
        CheckOutputDescription();
        needCheckOutputDesc_ = false;
    }
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        cout << "AVCODEC_BUFFER_FLAGS_EOS" << endl;
        AutoSwitchSurface();
        SHA512_Final(g_md, &g_c);
        OPENSSL_cleanse(&g_c, sizeof(g_c));
        if (!sfOutput && needCheckHash) {
            if (!MdCompare(g_md, SHA512_DIGEST_LENGTH, defaultPixelFormat == AV_PIXEL_FORMAT_NV12 ?
                fileSourcesha256Mpeg1_ : fileSourcesha256_2)) {
                cout << "MdCompare failed" << endl;
            }
        }
        return -1;
    }
    outFrameCount = outFrameCount + 1;
    return 0;
}

void VDecAPI11Sample::OutputFuncTest()
{
    FILE *outFile = nullptr;
    if (outputYuvFlag) {
        outFile = fopen(outputDir, "wb");
    }
    SHA512_Init(&g_c);
    bool flag = true;
    while (flag) {
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        OH_AVCodecBufferAttr attr;
        unique_lock<mutex> lock(signal->outMutex);
        signal->outCond.wait(lock, [this]() {
            if (!isRunning.load()) {
                return true;
            }
            return signal->outIdxQueue.size() > 0;
        });
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        uint32_t index = signal->outIdxQueue.front();
        OH_AVBuffer *buffer = signal->outBufferQueue.front();
        signal->outBufferQueue.pop();
        signal->outIdxQueue.pop();
        if (OH_AVBuffer_GetBufferAttr(buffer, &attr) != AV_ERR_OK) {
            errCount = errCount + 1;
        }
        lock.unlock();
        if (CheckAttrFlag(attr) == -1) {
            flag = false;
            break;
        }
        if (outFile != nullptr) {
            fwrite(OH_AVBuffer_GetAddr(buffer), 1, attr.size, outFile);
        }
        ProcessOutputData(buffer, index, attr.size);
        if (errCount > 0) {
            flag = false;
            break;
        }
    }
    if (outFile) {
        (void)fclose(outFile);
    }
}

void VDecAPI11Sample::SyncOutputFunc()
{
    FILE *outFile = nullptr;
    if (outputYuvFlag) {
        outFile = fopen(outputDir, "wb");
    }
    SHA512_Init(&g_c);
    bool flag = true;
    while (flag) {
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        OH_AVCodecBufferAttr attr;
        uint32_t index = 0;
        if (OH_VideoDecoder_QueryOutputBuffer(vdec_, &index, syncOutputWaitTime_) != AV_ERR_OK) {
            cout << "OH_VideoDecoder_QueryOutputBuffer fail" << endl;
            continue;
        }
        OH_AVBuffer *buffer = OH_VideoDecoder_GetOutputBuffer(vdec_, index);
        if (buffer == nullptr) {
            cout << "OH_VideoDecoder_GetOutputBuffer fail" << endl;
            errCount = errCount + 1;
            continue;
        }
        if (OH_AVBuffer_GetBufferAttr(buffer, &attr) != AV_ERR_OK) {
            errCount = errCount + 1;
        }
        if (SyncOutputFuncEos(attr, index) != AV_ERR_OK) {
            flag = false;
            break;
        }
        if (outFile != nullptr) {
            fwrite(OH_AVBuffer_GetAddr(buffer), 1, attr.size, outFile);
        }
        ProcessOutputData(buffer, index, attr.size);
        if (errCount > 0) {
            flag = false;
            break;
        }
    }
    if (outFile) {
        (void)fclose(outFile);
    }
}

int32_t VDecAPI11Sample::SyncOutputFuncEos(OH_AVCodecBufferAttr attr, uint32_t index)
{
    if (CheckAttrFlag(attr) == -1) {
        if (queryInputBufferEOS_) {
            OH_VideoDecoder_QueryInputBuffer(vdec_, &index, 0);
            OH_VideoDecoder_QueryInputBuffer(vdec_, &index, MILLION);
            OH_VideoDecoder_QueryInputBuffer(vdec_, &index, -1);
        }
        if (queryOutputBufferEOS_) {
            OH_VideoDecoder_QueryOutputBuffer(vdec_, &index, 0);
            OH_VideoDecoder_QueryOutputBuffer(vdec_, &index, MILLION);
            OH_VideoDecoder_QueryOutputBuffer(vdec_, &index, -1);
        }
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

void VDecAPI11Sample::ProcessOutputData(OH_AVBuffer *buffer, uint32_t index, int32_t size)
{
    GetVideoSupportedPixelFormats();
    GetFormatKey();
    if (!sfOutput) {
        if (size >= ((defaultWidth * defaultHeight * THREE) >> 1)) {
            uint8_t *cropBuffer = new uint8_t[size];
            if (memcpy_s(cropBuffer, size, OH_AVBuffer_GetAddr(buffer),
                         defaultWidth * defaultHeight) != EOK) {
                cout << "Fatal: memory copy failed Y" << endl;
            }
            // copy UV
            uint32_t uvSize = size - defaultWidth * defaultHeight;
            if (memcpy_s(cropBuffer + defaultWidth * defaultHeight, uvSize,
                         OH_AVBuffer_GetAddr(buffer) + defaultWidth * defaultHeight, uvSize) != EOK) {
                cout << "Fatal: memory copy failed UV" << endl;
            }
            SHA512_Update(&g_c, cropBuffer, size);
            delete[] cropBuffer;
        }
        if (OH_VideoDecoder_FreeOutputBuffer(vdec_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            errCount = errCount + 1;
        }
    } else {
        if (rsAtTime_) {
            int32_t usTimeNum = 1000;
            int32_t msTimeNum = 1000000;
            if (renderTimestampNs_ == 0) {
                renderTimestampNs_ = GetSystemTimeUs() * usTimeNum;
            }
            renderTimestampNs_ = renderTimestampNs_ + (usTimeNum / defaultFrameRate * msTimeNum);
            if (OH_VideoDecoder_RenderOutputBufferAtTime(vdec_, index, renderTimestampNs_) != AV_ERR_OK) {
                cout << "Fatal: RenderOutputBufferAtTime fail" << endl;
                errCount = errCount + 1;
            }
        } else {
            if (OH_VideoDecoder_RenderOutputBuffer(vdec_, index) != AV_ERR_OK) {
                cout << "Fatal: RenderOutputBuffer fail" << endl;
                errCount = errCount + 1;
            }
        }
    }
}

int32_t VDecAPI11Sample::state_EOS()
{
    uint32_t index;
    unique_lock<mutex> lock(signal->inMutex);
    signal->inCond.wait(lock, [this]() { return signal->inIdxQueue.size() > 0; });
    index = signal->inIdxQueue.front();
    signal->inIdxQueue.pop();
    signal->inBufferQueue.pop();
    lock.unlock();
    return OH_VideoDecoder_PushInputBuffer(vdec_, index);
}

void VDecAPI11Sample::SetEOS(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t res = OH_VideoDecoder_PushInputBuffer(vdec_, index);
    cout << "OH_VideoDecoder_PushInputBuffer    EOS   res: " << res << endl;
}

int32_t VDecAPI11Sample::Flush()
{
    unique_lock<mutex> inLock(signal->inMutex);
    clearIntqueue(signal->inIdxQueue);
    signal->inCond.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal->outMutex);
    clearIntqueue(signal->outIdxQueue);
    clearBufferqueue(signal->attrQueue);
    signal->outCond.notify_all();
    outLock.unlock();
    isRunning.store(false);
    return OH_VideoDecoder_Flush(vdec_);
}

int32_t VDecAPI11Sample::Reset()
{
    isRunning.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoDecoder_Reset(vdec_);
}

int32_t VDecAPI11Sample::Release()
{
    int ret = 0;
    if (vdec_ != nullptr) {
        ret = OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }

    if (signal != nullptr) {
        delete signal;
        signal = nullptr;
    }
    return ret;
}

int32_t VDecAPI11Sample::Stop()
{
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoDecoder_Stop(vdec_);
}

int32_t VDecAPI11Sample::Start()
{
    isRunning.store(true);
    return OH_VideoDecoder_Start(vdec_);
}

void VDecAPI11Sample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal->outMutex);
        clearIntqueue(signal->outIdxQueue);
        clearBufferqueue(signal->attrQueue);
        isRunning.store(false);
        signal->outCond.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
    }
}

int32_t VDecAPI11Sample::SetParameter(OH_AVFormat *format)
{
    return OH_VideoDecoder_SetParameter(vdec_, format);
}

int32_t VDecAPI11Sample::SwitchSurface()
{
    int32_t ret = OH_VideoDecoder_SetSurface(vdec_, nativeWindow[switchSurfaceFlag_]);
    switchSurfaceFlag_ = (switchSurfaceFlag_ == 1) ? 0 : 1;
    cout << "manual switch surf "<< switchSurfaceFlag_ << endl;
    return ret;
}

int32_t VDecAPI11Sample::RepeatCallSetSurface()
{
    for (int i = 0; i < repeatCallTime_; i++) {
        switchSurfaceFlag_ = (switchSurfaceFlag_ == 1) ? 0 : 1;
        int32_t ret = OH_VideoDecoder_SetSurface(vdec_, nativeWindow[switchSurfaceFlag_]);
        if (ret != AV_ERR_OK && ret != AV_ERR_OPERATE_NOT_PERMIT && ret != AV_ERR_INVALID_STATE) {
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    }
    return AV_ERR_OK;
}

int32_t VDecAPI11Sample::DecodeSetSurface()
{
    CreateSurface();
    return OH_VideoDecoder_SetSurface(vdec_, nativeWindow[0]);
}

int64_t VDecAPI11Sample::GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}
void VDecAPI11Sample::getFormat(const char *fileName)
{
    int trackType = 0;
    fd_ = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    cout << fileName << "-------" << fd_ << "-------" << size << endl;
    videoSource_ = OH_AVSource_CreateWithFD(fd_, 0, size);
    ASSERT_NE(videoSource_, nullptr);
    demuxer_ = OH_AVDemuxer_CreateWithSource(videoSource_);
    ASSERT_NE(demuxer_, nullptr);
    sourceFormat_ = OH_AVSource_GetSourceFormat(videoSource_);
    ASSERT_NE(sourceFormat_, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat_, OH_MD_KEY_TRACK_COUNT, &trackCount_));
    cout << "trackCount_----" << trackCount_ << endl;
    for (int32_t index = 0; index < trackCount_; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer_, index));
    }
    for (int32_t index = 0; index < trackCount_; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(videoSource_, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        if (trackType == MEDIA_TYPE_VID) {
            videoTrackId_ = index;
            break;
        }
    }
}

int32_t HighRand()
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dis(MIN_RANGE, MAX_RANGE);
    int HRand = dis(rng);
    if (HRand % EVEN_NUMBER != 0) {
        HRand = HRand + 1;
    }
    cout << "HRand is =  " << HRand << endl;
    return HRand;
}

int32_t FrameRand()
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dis(MIN_FRANGE, MAX_FRANGE);
    int FRand = dis(rng);
    if (FRand % EVEN_NUMBER != 0) {
        FRand = FRand + 1;
    }
    cout << "FRand is =  " << FRand << endl;
    return FRand;
}

int32_t WidthRand()
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dis(MIN_RANGE, MAX_RANGE);
    int WRand = dis(rng);
    if (WRand % EVEN_NUMBER != 0) {
        WRand = WRand + 1;
    }
    cout << "WRand is =  " << WRand << endl;
    return WRand;
}

int32_t VDecAPI11Sample::StartVideoDecoderFor263()
{
    isRunning.store(true);
    int ret = OH_VideoDecoder_Start(vdec_);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning.store(false);
        ReleaseInFile();
        Release();
        return ret;
    }
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(inputDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "failed open file " << inputDir << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    inputLoop_ = make_unique<thread>(&VDecAPI11Sample::InputFor263FuncTest, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VDecAPI11Sample::OutputFuncTest, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }

    return AV_ERR_OK;
}

void VDecAPI11Sample::InputFor263FuncTest()
{
    if (outputYuvSurface) {
        g_yuvSurface = true;
    }
    bool flag = true;
    while (flag) {
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        uint32_t index;
        unique_lock<mutex> lock(signal->inMutex);
        signal->inCond.wait(lock, [this]() {
            if (!isRunning.load()) {
                return true;
            }
            return signal->inIdxQueue.size() > 0;
        });
        if (!isRunning.load()) {
            flag = false;
            break;
        }
        index = signal->inIdxQueue.front();
        auto buffer = signal->inBufferQueue.front();
        signal->inIdxQueue.pop();
        signal->inBufferQueue.pop();
        lock.unlock();
        if (!inFile_->eof()) {
            int ret = PushDataFor263(index, buffer);
            if (ret == 1) {
                flag = false;
                break;
            }
        }
        if (sleepOnFPS) {
            usleep(MICRO_IN_SECOND / (int32_t)defaultFrameRate);
        }
    }
}

int32_t VDecAPI11Sample::PushDataFor263(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    if (beforeEosInput_ && frameCount_ > TEN) {
        SetEOS(index, buffer);
        return 1;
    }
    if (beforeEosInputInput_ && frameCount_ > TEN) {
        memset_s(&attr, sizeof(OH_AVCodecBufferAttr), 0, sizeof(OH_AVCodecBufferAttr));
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        beforeEosInputInput_ = false;
    }
    char ch[4] = {};
    (void)inFile_->read(ch, START_CODE_SIZE);
    if (repeatRun_ && inFile_->eof()) {
        static uint32_t repeat_count = 0;
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        cout << "repeat run " << repeat_count << endl;
        repeat_count++;
        return 0;
    }
    if (inFile_->eof()) {
        SetEOS(index, buffer);
        return 1;
    }
    uint32_t bufferSize = (uint32_t)(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << EIGHT) | ((ch[1] & 0xFF) << SIXTEEN) |
                                     ((ch[0] & 0xFF) << TWENTY_FOUR));
    if (useHDRSource_) {
        uint32_t zero = 0;
        uint32_t one = 1;
        uint32_t two = 2;
        uint32_t three = 3;
        bufferSize = (uint32_t)(((ch[zero] & 0xFF)) | ((ch[one] & 0xFF) << EIGHT) | ((ch[two] & 0xFF) << SIXTEEN) |
                                     ((ch[three] & 0xFF) << TWENTY_FOUR));
    }
    if (bufferSize >= ((defaultWidth * defaultHeight * THREE) >> 1)) {
        cout << "read bufferSize abnormal. buffersize = " << bufferSize << endl;
    }
    return SendData(bufferSize, index, buffer);
}

void VDecAPI11Sample::GetVideoSupportedPixelFormats()
{
    if (!isGetVideoSupportedPixelFormats || isGetVideoSupportedPixelFormatsNum != 0) {
        return;
    }
    OH_AVCapability *capability = OH_AVCodec_GetCapability(avcodecMimeType, isEncoder);
    OH_AVCapability_GetVideoSupportedNativeBufferFormats(capability, &pixlFormats, &pixlFormatNum);
    std::cout << "pixlFormats:" << *pixlFormats << "pixlFormatNum:" << pixlFormatNum << std::endl;
    for(int i = 0; i < pixlFormatNum; i++) {
        std::cout << "pixFormats[" << i << "]: " << *(pixlFormats + i) << std::endl;
    }
    isGetVideoSupportedPixelFormatsNum++;
}

void VDecAPI11Sample::GetFormatKey()
{
    if (!isGetFormatKey || isGetFormatKeyNum != 0) {
        return;
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_VideoDecoder_Configure(vdec_, format);
    format = OH_VideoDecoder_GetOutputDescription(vdec_);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_NATIVE_BUFFER_FORMAT, &firstCallBackKey);
    OH_AVFormat_Destroy(format);
    std::cout << "firstCallBackKey:" << firstCallBackKey << std::endl;
    isGetFormatKeyNum++;
}
   