/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "videotranscode_api11_sample.h"
#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>


using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
std::shared_ptr<std::ifstream> inFile_;
std::condition_variable g_cv;
std::atomic<bool> g_isRunning = true;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr int32_t EIGHT = 8;
constexpr int32_t SIXTEEN = 16;
constexpr int32_t TWENTY_FOUR = 24;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
FILE *g_outFile = nullptr;
FILE *g_outFileSec = nullptr;
}

int64_t GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

static void OnDecFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << " Dec Format Changed" << endl;
}

static void OnDecInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    VideoTransCodeApi11Sample *sample = static_cast<VideoTransCodeApi11Sample *>(userData);
    VSignal *signal = sample->decSignal;
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

static void OnDecOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    VideoTransCodeApi11Sample *sample = static_cast<VideoTransCodeApi11Sample *>(userData);
    OH_AVCodecBufferAttr attr;
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        OH_VideoEncoder_NotifyEndOfStream(sample->venc_);
        OH_VideoEncoder_NotifyEndOfStream(sample->vencSecondary);
    } else {
        OH_VideoDecoder_RenderOutputBuffer(codec, index);
        sample->frameCountDec++;
    }
}

static void OnDecError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    VideoTransCodeApi11Sample *sample = static_cast<VideoTransCodeApi11Sample *>(userData);
    sample->errorCount++;
    cout << "VdecError errorCode=" << errorCode << endl;
    g_isRunning.store(false);
    g_cv.notify_all();
}

static void OnEncError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    VideoTransCodeApi11Sample *sample = static_cast<VideoTransCodeApi11Sample *>(userData);
    sample->errorCount++;
    cout << "VencError errorCode=" << errorCode << endl;
    g_isRunning.store(false);
    g_cv.notify_all();
}

static void OnEncFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

static void VencInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}

static void OnEncOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    VideoTransCodeApi11Sample *sample = static_cast<VideoTransCodeApi11Sample*>(userData);
    OH_AVCodecBufferAttr attr;
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        cout << "CheckAttrFlag GET EOS" << endl;
        g_isRunning.store(false);
        g_cv.notify_all();
    } else if (attr.flags != AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
        if (codec == sample->venc_) {
            sample->frameCountEnc++;
        } else if (codec == sample->vencSecondary) {
            sample->frameCountEncSecondary++;
        }
    }
    if (codec == sample->venc_) {
        OH_VideoEncoder_FreeOutputBuffer(sample->venc_, index);
    } else if (codec == sample->vencSecondary) {
        OH_VideoEncoder_FreeOutputBuffer(sample->vencSecondary, index);
    }
}

int32_t DecAvcPushData(OH_AVBuffer *buffer, uint32_t bufferSize, std::unique_ptr<uint8_t[]> &&fileBuffer)
{
    int32_t size = OH_AVBuffer_GetCapacity(buffer);
    if (size < bufferSize + START_CODE_SIZE) {
        cout << "error: size < bufferSize" << endl;
        return 1;
    }
    uint8_t *avBuffer = OH_AVBuffer_GetAddr(buffer);
    if (avBuffer == nullptr) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        return 1;
    }
    if (memcpy_s(avBuffer, size, fileBuffer.get(), bufferSize + START_CODE_SIZE) != EOK) {
        cout << "Fatal: memcpy fail" << endl;
        return 1;
    }
    return 0;
}

int32_t VideoTransCodeApi11Sample::SendData(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    char ch[4] = {};
    (void)inFile_->read(ch, START_CODE_SIZE);
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        attr.offset = 0;
        OH_AVBuffer_SetBufferAttr(buffer, &attr);
        OH_VideoDecoder_PushInputBuffer(codec, index);
        return 1;
    }
    uint32_t bufferSize = static_cast<uint32_t>(((ch[3] & 0xFF)) ((ch[2] & 0xFF) << EIGHT) 
    | ((ch[1] & 0xFF) << SIXTEEN) | (([0] & 0xFF) << TWENTY_FOUR));
    std::unique_ptr<uint8_t[]> fileBuffer = std::make_unique<uint8_t[]>(bufferSize + START_CODE_SIZE);
    if (memcpy_s(fileBuffer.get(), bufferSize + START_CODE_SIZE, START_CODE, START_CODE_SIZE) != EOK) {
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        attr.offset = 0;
        OH_AVBuffer_SetBufferAttr(buffer, &attr);
        OH_VideoDecoder_PushInputBuffer(codec, index);
        return 1;
    }
    (void)inFile_->read((char *)fileBuffer.get() + START_CODE_SIZE, bufferSize);
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (DecAvcPushData(buffer, bufferSize, std::move(fileBuffer))) {
        return 0;
    }
    attr.pts = GetSystemTimeUs();
    attr.size = bufferSize + START_CODE_SIZE;
    attr.offset = 0;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t result = OH_VideoDecoder_PushInputBuffer(codec, index);
    if (result != AV_ERR_OK) {
        cout << "push input data failed,error:" << result << endl;
    } else {
        inputNum++;
    }
    return 0;
}

VideoTransCodeApi11Sample::~VideoTransCodeApi11Sample()
{
    inputNum = 0;
    Release();
    if (g_outFile) {
        (void)fclose(g_outFile);
    }
    if (g_outFileSec) {
        (void)fclose(g_outFileSec);
    }
}

int32_t VideoTransCodeApi11Sample::CreateVideocoder(std::string codeName, std::string enCodeName)
{
    decSignal = new VSignal();
    if (decSignal == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    vdec_ = OH_VideoDecoder_CreateByName(codeName.c_str());
    if (vdec_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }

    OH_VideoEncoder_CreatePrimaryWithPreproc(enCodeName.c_str(), &venc_);
    if (venc_ == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    OH_VideoEncoder_CreateSecondaryFromPrimary(venc_, &vencSecondary);
    if (vencSecondary == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

void VideoTransCodeApi11Sample::InputFunc()
{
    while (!false) {
        if (!g_isRunning.load()) {
            break;
        }
        uint32_t index;
        unique_lock<mutex> lock(decSignal->inMutex_);
        decSignal->inCond_.wait(lock, [this]() {
            if (!g_isRunning.load()) {
                return true;
            }
            return decSignal->inIdxQueue_.size() > 0;
        });
        if (!g_isRunning.load()) {
            break;
        }
        index = decSignal->inIdxQueue_.front();
        auto buffer = decSignal->inBufferQueue_.front();

        decSignal->inIdxQueue_.pop();
        decSignal->inBufferQueue_.pop();
        lock.unlock();
        if (SendData(vdec_, index, buffer) == 1)
        {
            break;
        }
    }
}

int32_t SetFormatValue(OH_AVFormat *format)
{
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, defaultPixelFormat);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    int ret = OH_VideoDecoder_Configure(vdec_, format);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, defaultProfile);
    if (enableDownsampling) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, downsamplingWidth);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, downsamplingHeight);
        (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE, dropFrameRate);
    } else {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, cropLeft);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, cropTop);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, cropRight);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, cropBottom);
        (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE, dropFrameRate);
    }
    return AV_ERR_OK;
}

int32_t VideoTransCodeApi11Sample::Configure()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    int ret = AV_ERR_OK;
    ret = SetFormatValue(format);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = OH_VideoEncoder_Configure(venc_, format);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = OH_VideoEncoder_Configure(vencSecondary, format);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = OH_VideoEncoder_GetSurface(venc_, &window);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = OH_VideoDecoder_SetSurface(vdec_, window);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    encCb_.onError = OnEncError;
    encCb_.onStreamChanged = OnEncFormatChanged;
    encCb_.onNeedInputBuffer = VencInputDataReady;
    encCb_.onNewOutputBuffer = OnEncOutputDataReady;
    ret = OH_VideoEncoder_RegisterCallback(venc_, encCb_, this);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = OH_VideoEncoder_RegisterCallback(vencSecondary, encCb_, this);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    OH_AVFormat_Destroy(format);
    decCb_.onError = OnDecError;
    decCb_.onStreamChanged = OnDecFormatChanged;
    decCb_.onNeedInputBuffer = OnDecInputDataReady;
    decCb_.onNewOutputBuffer = OnDecOutputDataReady;
    return OH_VideoDecoder_RegisterCallback(vdec_, decCb_, this);
}

int32_t VideoTransCodeApi11Sample::Start()
{
    int32_t ret = 0;
    inFile_ = make_unique<ifstream>();
    inFile_->open(inpDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        (void)OH_VideoDecoder_Destroy(vdec_);
        (void)OH_VideoEncoder_Destroy(venc_);
        (void)OH_VideoEncoder_Destroy(vencSecondary);
        vdec_ = nullptr;
        venc_ = nullptr;
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    g_isRunning.store(true);
    ret = OH_VideoEncoder_Start(venc_);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    ret = OH_VideoDecoder_Start(vdec_);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    inputLoop_ = make_unique<thread>(&VideoTransCodeApi11Sample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        g_isRunning.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }

    return 0;
}

int32_t VideoTransCodeApi11Sample::StartSecondaryEncoder()
{
    int32_t ret = 0;
    ret = OH_VideoEncoder_Start(vencSecondary);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start secondary codec" << endl;
        return ret;
    }
    return AV_ERR_OK;
}

void VideoTransCodeApi11Sample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VideoTransCodeApi11Sample::WaitForEos()
{
    std::mutex mtx;
    unique_lock<mutex> lock(mtx);
    g_cv.wait(lock, []() {
        return !(g_isRunning.load());
    });
    if (inputLoop_) {
        inputLoop_->join();
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoEncoder_Stop(venc_);
    OH_VideoEncoder_Stop(vencSecondary);
}

int32_t VideoTransCodeApi11Sample::Release()
{
    if (decSignal != nullptr) {
        delete decSignal;
        decSignal = nullptr;
    }
    if (vdec_ != nullptr) {
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
    if (venc_ != nullptr) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
    if (vencSecondary != nullptr) {
        OH_VideoEncoder_Destroy(vencSecondary);
        vencSecondary = nullptr;
    }
    return 0;
}