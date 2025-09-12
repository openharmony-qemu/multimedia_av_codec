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
#include <vector>
#include <string>
#include <sstream>
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "syncdecoder_sample.h"
#include "nlohmann/json.hpp"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace nlohmann;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
VDecSyncSample *g_decSample = nullptr;
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

VDecSyncSample::~VDecSyncSample()
{
    for (int i = 0; i < maxSurfNum; i++) {
        if (nativeWindow[i]) {
            OH_NativeWindow_DestroyNativeWindow(nativeWindow[i]);
            nativeWindow[i] = nullptr;
        }
    }
    if (nativeBuffer_ != nullptr) {
        OH_NativeBuffer_Unreference(nativeBuffer_);
        nativeBuffer_ = nullptr;
    }
    Stop();
    Release();
}

int64_t VDecSyncSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;
    return nanoTime / NANOS_IN_MICRO;
}

int32_t VDecSyncSample::ConfigureVideoDecoder()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    if (maxInputSize > 0) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, maxInputSize);
    }
    originalWidth = defaultWidth;
    originalHeight = defaultHeight;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, defualtPixelFormat);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    if (useHDRSource) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, DEFAULT_PROFILE);
    } else {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, DEFAULT_PROFILE);
    }

    if (transferFlag) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, OH_COLORSPACE_BT709_LIMIT);
    }
    if (nV21Flag) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV21);
    }
    if (enableVRR) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_OUTPUT_ENABLE_VRR, 1);
    }
    if (enableLowLatency) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY, 1);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, enbleSyncMode);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, enbleBlankFrame);
    int ret = OH_VideoDecoder_Configure(vdec_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

void VDecSyncSample::CreateSurface()
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

int32_t VDecSyncSample::CreateVideoDecoder(string codeName)
{
    vdec_ = OH_VideoDecoder_CreateByName(codeName.c_str());
    g_decSample = this;
    return vdec_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

int32_t VDecSyncSample::SyncInputFuncFuzz(const uint8_t *data, size_t size)
{
    uint32_t index;
    if (!isRunning_.load()) {
        return AV_ERR_NO_MEMORY;
    }
    cout << "OH_VideoDecoder_QueryInputBuffer start" << endl;
    if (OH_VideoDecoder_QueryInputBuffer(vdec_, &index, syncInputWaitTime) != AV_ERR_OK) {
        cout << "OH_VideoDecoder_QueryInputBuffer fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    cout << "OH_VideoDecoder_QueryInputBuffer succ" << endl;
    OH_AVBuffer *buffer = OH_VideoDecoder_GetInputBuffer(vdec_, index);
    if (buffer == nullptr) {
        cout << "OH_VideoDecoder_GetInputBuffer fail" << endl;
        errCount = errCount + 1;
        return AV_ERR_NO_MEMORY;
    }
    int32_t bufferSize = OH_AVBuffer_GetCapacity(buffer);
    uint8_t *bufferAddr = OH_AVBuffer_GetAddr(buffer);
    if (size > bufferSize - START_CODE_SIZE) {
        OH_VideoDecoder_PushInputBuffer(vdec_, index);
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr, bufferSize, START_CODE, START_CODE_SIZE) != EOK) {
        OH_VideoDecoder_PushInputBuffer(vdec_, index);
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr + START_CODE_SIZE, bufferSize - START_CODE_SIZE, data, size) != EOK) {
        OH_VideoDecoder_PushInputBuffer(vdec_, index);
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    OH_AVCodecBufferAttr attr;
    attr.pts = GetSystemTimeUs();
    attr.size = size;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    OH_AVErrCode ret = OH_VideoDecoder_PushInputBuffer(vdec_, index);
    return ret;
}

int32_t VDecSyncSample::SyncOutputFuncFuzz()
{
    uint32_t index = 0;
    cout << "OH_VideoDecoder_QueryOutputBuffer start" << endl;
    int32_t ret = OH_VideoDecoder_QueryOutputBuffer(vdec_, &index, syncOutputWaitTime);
    if (ret != AV_ERR_OK) {
        cout << "OH_VideoDecoder_QueryOutputBuffer fail" << endl;
        return AV_ERR_UNKNOWN;
    }
    cout << "OH_VideoDecoder_QueryOutputBuffer succ" << endl;
    OH_AVBuffer *buffer = OH_VideoDecoder_GetOutputBuffer(vdec_, index);
    if (buffer == nullptr) {
        cout << "OH_VideoDecoder_QueryOutputBuffer fail" << endl;
        return AV_ERR_UNKNOWN;
    }
    CompareHdrInfo(buffer);
    if (sfOutput) {
        if (isRenderAttime) {
            ret = OH_VideoDecoder_RenderOutputBufferAtTime(vdec_, index, renderTimestampNs);
        } else {
            ret = OH_VideoDecoder_RenderOutputBuffer(vdec_, index);
        }
    } else {
        ret = OH_VideoDecoder_FreeOutputBuffer(vdec_, index);
    }
    if (ret != AV_ERR_OK) {
        Flush();
        Start();
    }
    return AV_ERR_OK;
}

int32_t VDecSyncSample::Flush()
{
    isFlushing_.store(true);
    isRunning_.store(false);
    int32_t ret = OH_VideoDecoder_Flush(vdec_);
    isFlushing_.store(false);
    return ret;
}

int32_t VDecSyncSample::Reset()
{
    isRunning_.store(false);
    return OH_VideoDecoder_Reset(vdec_);
}

int32_t VDecSyncSample::Release()
{
    int ret = 0;
    if (vdec_ != nullptr) {
        ret = OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
    return ret;
}

int32_t VDecSyncSample::Stop()
{
    return OH_VideoDecoder_Stop(vdec_);
}

int32_t VDecSyncSample::Start()
{
    isRunning_.store(true);
    return OH_VideoDecoder_Start(vdec_);
}

int32_t VDecSyncSample::DecodeSetSurface()
{
    CreateSurface();
    return OH_VideoDecoder_SetSurface(vdec_, nativeWindow[0]);
}

void VDecSyncSample::SetParameter(int32_t data, int32_t data1)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, data);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, data1);
    OH_VideoDecoder_SetParameter(vdec_, format);
    OH_AVFormat_Destroy(format);
}

void VDecSyncSample::CompareHdrInfo(OH_AVBuffer *buffer)
{
    if (!needCompareHdrInof || buffer == nullptr) {
        return;
    }
    nativeBuffer_ = OH_AVBuffer_GetNativeBuffer(buffer);
    if (nativeBuffer_ == nullptr) {
        cout << "Fatel: get native buffer fail" << endl;
        return;
    }
    GetHdrDynamicMetaData();
    GetHdrtaticMetaData();
    int metaDataType = 0;
    GetHdrMetaDataType(metaDataType);
}

void VDecSyncSample::GetHdrDynamicMetaData()
{
    int32_t metadataSize = 0;
    uint8_t *metadata = nullptr;
    if (OH_NativeBuffer_GetMetadataValue(nativeBuffer_, OH_HDR_DYNAMIC_METADATA, &metadataSize, &metadata) != 0) {
        cout << "get dynamic meta data faile" << endl;
        return;
    }
    delete[] metadata;
    metadata = nullptr;
}

void VDecSyncSample::GetHdrtaticMetaData()
{
    int32_t metadataSize = 0;
    uint8_t *metadata = nullptr;
    if (OH_NativeBuffer_GetMetadataValue(nativeBuffer_, OH_HDR_STATIC_METADATA, &metadataSize, &metadata) != 0) {
        cout << "get static meta data faile" << endl;
        return;
    }
    delete[] metadata;
    metadata = nullptr;
}

void VDecSyncSample::GetHdrMetaDataType(int &metaDataType)
{
    int32_t metadataSize = 0;
    uint8_t *metadata = nullptr;
    if (OH_NativeBuffer_GetMetadataValue(nativeBuffer_, OH_HDR_METADATA_TYPE, &metadataSize, &metadata) != 0) {
        return;
    }
    memcpy_s(&metaDataType, metadataSize, metadata, metadataSize);
    delete[] metadata;
    metadata = nullptr;
    return;
}