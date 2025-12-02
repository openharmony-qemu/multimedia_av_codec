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

#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <atomic>
#include <map>
#include <shared_mutex>
#include <fstream>
#include <vector>
#include "block_queue.h"
#include "codecbase.h"
#include "render_surface.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {

typedef signed char INT8;
typedef signed int INT32;
typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
using AVBuffer = Media::AVBuffer;
using AVAllocator = Media::AVAllocator;
using AVAllocatorFactory = Media::AVAllocatorFactory;
using MemoryFlag = Media::MemoryFlag;
using FormatDataType = Media::FormatDataType;

class VideoDecoder : public RenderSurface, public CodecBase {
public:
    VideoDecoder(const std::string& codecName, const std::string& path);
    ~VideoDecoder() = default;
    int32_t Init(Media::Meta &callerInfo) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetParameter(const Format &format) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t ReleaseOutputBuffer(uint32_t index) override;
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback) override;
    int32_t NotifyMemoryRecycle() override;
    int32_t NotifyMemoryWriteBack() override;
    int32_t Configure(const Format &format) override;
    virtual bool CheckVideoPixelFormat(VideoPixelFormat vpf) = 0;
    virtual void ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth) = 0;
    virtual void ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal = 0,
        int32_t maxVal = INT_MAX) = 0;
    void ConfigureSurface(const Format &format, const std::string_view &formatKey, FormatDataType formatType);
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t RenderOutputBuffer(uint32_t index) override;
    int32_t CheckFormatChange(uint32_t index, int width, int height);
    virtual int32_t CreateDecoder() = 0;
    virtual void DeleteDecoder() = 0;
    bool IsValid() const { return isValid_; }
    void ReleaseResource();
    void FramePostProcess(std::shared_ptr<CodecBuffer> &frameBuffer, uint32_t index, int32_t status, int ret);
    int32_t FillFrameBuffer(const std::shared_ptr<CodecBuffer> &frameBuffer);
    void ResetData();
    int32_t UpdateOutputBuffer(uint32_t index);
    int32_t UpdateSurfaceMemory(uint32_t index);
    int32_t GetSurfaceBufferStride(const std::shared_ptr<CodecBuffer> &frameBuffer);
#ifdef BUILD_ENG_VERSION
    void OpenDumpFile();
    void DumpOutputBuffer();
    void DumpConvertOut(struct SurfaceInfo &surfaceInfo);
#endif

    static std::mutex decoderCountMutex_;
    static std::vector<uint32_t> freeIDSet_;
    uint32_t decInstanceID_;
    static std::vector<uint32_t> decInstanceIDSet_;
    void* handle_ = nullptr;
    std::string codecName_;
    std::string libPath_ = "";
    bool isValid_ = true;
    std::shared_ptr<MediaCodecCallback> callback_;
    bool isOutBufSetted_ = false;
    std::mutex decRunMutex_;
    std::shared_ptr<RenderSurface> renderSurface_ = nullptr;
    std::string decName_;
    CallerInfo decInfo_;
    std::shared_ptr<TaskThread> sendTask_ = nullptr;
    std::shared_ptr<AVFrame> cachedFrame_ = nullptr;
    std::atomic<bool> isSendEos_ = false;
    std::shared_ptr<BlockQueue<uint32_t>> inputAvailQue_;
    std::shared_ptr<Scale> scale_ = nullptr;
#ifdef BUILD_ENG_VERSION
    std::shared_ptr<std::ofstream> dumpInFile_ = nullptr;
    std::shared_ptr<std::ofstream> dumpOutFile_ = nullptr;
    std::shared_ptr<std::ofstream> dumpConvertFile_ = nullptr;
#endif

private:
    virtual int32_t Initialize() = 0;
    bool IsActive() const;
    void CalculateBufferSize();
    int32_t AllocateBuffers();
    void InitBuffers();
    void ResetBuffers();
    void ReleaseBuffers();
    void StopThread();
    virtual void SendFrame() = 0;
    int32_t AllocateInputBuffer(int32_t bufferCnt, int32_t inBufferSize);
    int32_t AllocateOutputBuffer(int32_t bufferCnt);
    int32_t AllocateOutputBuffersFromSurface(int32_t bufferCnt);
    void SetSurfaceParameter(const Format &format, const std::string_view &formatKey,
        FormatDataType formatType);
    int32_t Detach(sptr<SurfaceBuffer> surfaceBuffer);
    virtual int32_t DecodeFrameOnce() = 0;
    virtual void DecoderFuncMatch() = 0;
    virtual void ReleaseHandle() = 0;
    virtual void InitParams() = 0;
    void SetCallerToBuffer(sptr<SurfaceBuffer> surfaceBuffer);

    bool disableDmaSwap_ = false;
    int32_t inputBufferSize_ = 0;
    int32_t inputBufferCnt_ = 0;
    uint8_t *scaleData_[AV_NUM_DATA_POINTERS] = {nullptr};
    int32_t scaleLineSize_[AV_NUM_DATA_POINTERS] = {0};
    bool isConverted_ = false;
    sptr<Surface> surface_ = nullptr;
    std::mutex renderBufferMapMutex_;
    std::atomic<bool> isBufferAllocated_ = false;
};

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VIDEO_DECODER_H