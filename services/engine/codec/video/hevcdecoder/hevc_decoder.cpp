/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <set>
#include <thread>
#include <dlfcn.h>
#include <malloc.h>
#include "syspara/parameters.h"
#include "securec.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "dma_swap.h"
#include "utils.h"
#include "avcodec_codec_name.h"
#include "hevc_decoder.h"
#include <fstream>
#include <cstdarg>
#include <sstream>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>
#include "v1_0/cm_color_space.h"
#include "v1_0/hdr_static_metadata.h"
#include "v1_0/buffer_handle_meta_key_type.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "HevcDecoderLoader"};
const char *HEVC_DEC_LIB_PATH = "libhevcdec_ohos.z.so";
const char *HEVC_DEC_CREATE_FUNC_NAME = "HEVC_CreateDecoder";
const char *HEVC_DEC_DECODE_FRAME_FUNC_NAME = "HEVC_DecodeFrame";
const char *HEVC_DEC_FLUSH_FRAME_FUNC_NAME = "HEVC_FlushFrame";
const char *HEVC_DEC_DELETE_FUNC_NAME = "HEVC_DeleteDecoder";


constexpr uint32_t VIDEO_PIX_DEPTH_YUV = 3;
constexpr int32_t VIDEO_MIN_BUFFER_SIZE = 1474560; // 1280*768
constexpr int32_t VIDEO_MAX_BUFFER_SIZE = 3110400; // 1080p
constexpr int32_t VIDEO_MIN_SIZE = 2;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 1920;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 1920;

constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_BLOCKPERFRAME_SIZE = 36864;
constexpr int32_t VIDEO_BLOCKPERSEC_SIZE = 983040;
static constexpr float PRIMARY_SCALE = 0.00002;
static constexpr float LUMI_SCALE    = 0.0001;
constexpr int32_t VIDEO_FRAMERATE_DEFAULT_SIZE = 30;
constexpr int32_t VIDEO_BITRATE_MAX_SIZE = 300000000;

constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
} SUPPORT_HEVC_DECODER[] = {
    {AVCodecCodecName::VIDEO_DECODER_HEVC_NAME, CodecMimeType::VIDEO_HEVC},
};
constexpr uint32_t SUPPORT_HEVC_DECODER_NUM = sizeof(SUPPORT_HEVC_DECODER) / sizeof(SUPPORT_HEVC_DECODER[0]);
static std::map<uint64_t, int32_t> g_surfaceCntMap;
static std::mutex g_surfaceMapMutex;
} // namespace
using namespace OHOS::Media;

HevcDecoder::HevcDecoder(const std::string &name) : VideoDecoder(name)
{
    AVCODEC_SYNC_TRACE;
    std::unique_lock<std::mutex> lock(decoderCountMutex_);
    pid_ = getpid();
    if (!freeIDSet_.empty()) {
        decInstanceID_ = freeIDSet_[0];
        freeIDSet_.erase(freeIDSet_.begin());
        decInstanceIDSet_.push_back(decInstanceID_);
    } else if (freeIDSet_.size() + decInstanceIDSet_.size() < VIDEO_INSTANCE_SIZE) {
        decInstanceID_ = freeIDSet_.size() + decInstanceIDSet_.size();
        decInstanceIDSet_.push_back(decInstanceID_);
    } else {
        decInstanceID_ = VIDEO_INSTANCE_SIZE + 1;
    }
    lock.unlock();

    if (decInstanceID_ < VIDEO_INSTANCE_SIZE) {
        handle_ = dlopen(HEVC_DEC_LIB_PATH, RTLD_LAZY);
        if (handle_ == nullptr) {
            AVCODEC_LOGE("Load codec failed: %{public}s", HEVC_DEC_LIB_PATH);
            isValid_ = false;
        }
        HevcFuncMatch();
        AVCODEC_LOGI("Num %{public}u HevcDecoder entered, state: Uninitialized", decInstanceID_);
    } else {
        AVCODEC_LOGE("Decoder already has %{public}d instances, cannot has more instances", VIDEO_INSTANCE_SIZE);
        isValid_ = false;
    }

    initParams_.uiChannelID = decInstanceID_;
    initParams_.logFxn = HevcDecLog;
    initParams_.uiDecodeMode = IHW265_DECODE_VIDEO;
    initParams_.eOutPutOrder = 1; // 1 default: display order
    InitParams();
    (void)memset_s(&colorSpaceInfo_, sizeof(HEVC_COLOR_SPACE_INFO), 0, sizeof(HEVC_COLOR_SPACE_INFO));
}

void HevcDecoder::HevcFuncMatch()
{
    if (handle_ != nullptr) {
        std::unique_lock<std::mutex> runlock(decRunMutex_);
        hevcDecoderCreateFunc_ = reinterpret_cast<CreateHevcDecoderFuncType>(dlsym(handle_,
            HEVC_DEC_CREATE_FUNC_NAME));
        hevcDecoderDecodecFrameFunc_ = reinterpret_cast<DecodeFuncType>(dlsym(handle_,
            HEVC_DEC_DECODE_FRAME_FUNC_NAME));
        hevcDecoderFlushFrameFunc_ = reinterpret_cast<FlushFuncType>(dlsym(handle_, HEVC_DEC_FLUSH_FRAME_FUNC_NAME));
        hevcDecoderDeleteFunc_ = reinterpret_cast<DeleteFuncType>(dlsym(handle_, HEVC_DEC_DELETE_FUNC_NAME));
        runlock.unlock();
        if (hevcDecoderCreateFunc_ == nullptr || hevcDecoderDecodecFrameFunc_ == nullptr ||
            hevcDecoderDeleteFunc_ == nullptr || hevcDecoderFlushFrameFunc_ == nullptr) {
                AVCODEC_LOGE("HevcDecoder hevcFuncMatch_ failed!");
                ReleaseHandle();
        }
    }
}

void HevcDecoder::ReleaseHandle()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    hevcDecoderCreateFunc_ = nullptr;
    hevcDecoderDecodecFrameFunc_ = nullptr;
    hevcDecoderFlushFrameFunc_ = nullptr;
    hevcDecoderDeleteFunc_ = nullptr;
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
    }
    runLock.unlock();
}

HevcDecoder::~HevcDecoder()
{
    ReleaseResource();
    callback_ = nullptr;
    ReleaseHandle();
    if (decInstanceID_ < VIDEO_INSTANCE_SIZE) {
        std::lock_guard<std::mutex> lock(decoderCountMutex_);
        freeIDSet_.push_back(decInstanceID_);
        auto it = std::find(decInstanceIDSet_.begin(), decInstanceIDSet_.end(), decInstanceID_);
        if (it != decInstanceIDSet_.end()) {
            decInstanceIDSet_.erase(it);
        }
    }
#ifdef BUILD_ENG_VERSION
    if (dumpInFile_ != nullptr) {
        dumpInFile_->close();
    }
    if (dumpOutFile_ != nullptr) {
        dumpOutFile_->close();
    }
    if (dumpConvertFile_ != nullptr) {
        dumpConvertFile_->close();
    }
#endif
    mallopt(M_FLUSH_THREAD_CACHE, 0);
}

void HevcDecoder::InitParams()
{
    hevcDecoderInputArgs_.pStream = nullptr;
    hevcDecoderInputArgs_.uiStreamLen = 0;
    hevcDecoderInputArgs_.uiTimeStamp = 0;
    hevcDecoderOutpusArgs_.pucOutYUV[0] = nullptr;
    hevcDecoderOutpusArgs_.pucOutYUV[1] = nullptr; // 1 u channel
    hevcDecoderOutpusArgs_.pucOutYUV[2] = nullptr; // 2 v channel
    hevcDecoderOutpusArgs_.uiDecBitDepth = 0;
    hevcDecoderOutpusArgs_.uiDecHeight = 0;
    hevcDecoderOutpusArgs_.uiDecStride = 0;
    hevcDecoderOutpusArgs_.uiDecWidth = 0;
    hevcDecoderOutpusArgs_.uiTimeStamp = 0;
    InitHdrParams();
}

void HevcDecoder::InitHdrParams()
{
    hevcDecoderOutpusArgs_.uiColorSpaceInfo.fullRangeFlag = 0;
    hevcDecoderOutpusArgs_.uiColorSpaceInfo.colorDescriptionPresentFlag = 0;
    hevcDecoderOutpusArgs_.uiColorSpaceInfo.colorPrimaries = 0;
    hevcDecoderOutpusArgs_.uiColorSpaceInfo.transferCharacteristic = 0;
    hevcDecoderOutpusArgs_.uiColorSpaceInfo.matrixCoeffs = 0;
    hevcDecoderOutpusArgs_.uiHdrMetadata.dynamicMetadataSize = 0;
    hevcDecoderOutpusArgs_.uiHdrMetadata.dynamicMetadata = nullptr;
    hevcDecoderOutpusArgs_.uiHdrMetadata.whitePointX = 0;
    hevcDecoderOutpusArgs_.uiHdrMetadata.whitePointY = 0;
    hevcDecoderOutpusArgs_.uiHdrMetadata.maxDisplayMasteringLuminance = 0;
    hevcDecoderOutpusArgs_.uiHdrMetadata.minDisplayMasteringLuminance = 0;
    hevcDecoderOutpusArgs_.uiHdrMetadata.maxContentLightLevel = 0;
    hevcDecoderOutpusArgs_.uiHdrMetadata.maxPicAverageLightLevel = 0;
    for (int i = 0; i < 3; i++) { // 3: GBR
        hevcDecoderOutpusArgs_.uiHdrMetadata.displayPrimariesX[i] = 0;
        hevcDecoderOutpusArgs_.uiHdrMetadata.displayPrimariesY[i] = 0;
    }
}

void HevcDecoder::ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth)
{
    if (isWidth == true) {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_SIZE, VIDEO_MAX_WIDTH_SIZE);
    } else {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_SIZE, VIDEO_MAX_HEIGHT_SIZE);
    }
}

int32_t HevcDecoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    decName_ = "hevcdecoder_["+ std::to_string(instanceId_) + "]";
    AVCODEC_LOGI("Num %{public}u current codec name: %{public}s", decInstanceID_, decName_.c_str());
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_HEVC_DECODER_NUM; ++i) {
        if (SUPPORT_HEVC_DECODER[i].codecName == codecName_) {
            mime = SUPPORT_HEVC_DECODER[i].mimeType;
            break;
        }
    }
    CHECK_AND_RETURN_RET_LOG(
        !mime.empty(), 
        AVCS_ERR_INVALID_VAL, 
        "Init codec failed: not support name:%{public}s", 
        codecName_.c_str());
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mime);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, codecName_);
    decInfo_.mimeType = mime;
    sendTask_ = std::make_shared<TaskThread>("SendFrame");
    sendTask_->RegisterHandler([this] { SendFrame(); });

#ifdef BUILD_ENG_VERSION
    OpenDumpFile();
#endif
    state_ = State::INITIALIZED;
    AVCODEC_LOGI("Init codec successful,  state: Uninitialized -> Initialized");
    return AVCS_ERR_OK;
}

int32_t HevcDecoder::CreateDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    int32_t createRet = 0;
    if (hevcSDecoder_ == nullptr && hevcDecoderCreateFunc_ != nullptr) {
        createRet = hevcDecoderCreateFunc_(&hevcSDecoder_, &initParams_);
    }
    runLock.unlock();
    CHECK_AND_RETURN_RET_LOG(createRet == 0 && hevcSDecoder_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "hevc deocder create failed");
    return AVCS_ERR_OK;
}

void HevcDecoder::DeleteDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (hevcSDecoder_ != nullptr && hevcDecoderDeleteFunc_ != nullptr) {
        int ret = hevcDecoderDeleteFunc_(hevcSDecoder_);
        if (ret != 0) {
            AVCODEC_LOGE("Error: hevcDecoder delete error: %{public}d", ret);
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            state_ = State::ERROR;
        }
        hevcSDecoder_ = nullptr;
    }
    runLock.unlock();
}

void HevcDecoder::ConvertDecOutToAVFrame(int32_t bitDepth)
{
    std::lock_guard<std::mutex> convertLock(convertDataMutex_);
    if (cachedFrame_ == nullptr) {
        AVFrame *frame = av_frame_alloc();
        if (frame == nullptr) {
            AVCODEC_LOGE("HevcDecoder av_frame_alloc failed");
            callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY);
            state_ = State::ERROR;
            return;    
        }
        cachedFrame_ = std::shared_ptr<AVFrame>(frame, [](AVFrame *p) { av_frame_free(&p); });
    }
    cachedFrame_->data[0] = hevcDecoderOutpusArgs_.pucOutYUV[0];
    cachedFrame_->data[1] = hevcDecoderOutpusArgs_.pucOutYUV[1]; // 1 u channel
    cachedFrame_->data[2] = hevcDecoderOutpusArgs_.pucOutYUV[2]; // 2 v channel
    if (bitDepth == BITS_PER_PIXEL_COMPONENT_8) {
        cachedFrame_->format = static_cast<int>(AVPixelFormat::AV_PIX_FMT_YUV420P);
        cachedFrame_->linesize[0] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride);
        cachedFrame_->linesize[1] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride >> 1); // 1 u channel
        cachedFrame_->linesize[2] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride >> 1); // 2 v channel
    } else {
        cachedFrame_->format = static_cast<int>(AVPixelFormat::AV_PIX_FMT_YUV420P10LE);
        cachedFrame_->linesize[0] =
            static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride * 2); // 2 10bit per pixel 2bytes
        cachedFrame_->linesize[1] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride); // 1 u channel
        cachedFrame_->linesize[2] = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride); // 2 v channel
        if (outputPixelFmt_ == VideoPixelFormat::NV21) { // exchange uv channel
            cachedFrame_->data[1] = hevcDecoderOutpusArgs_.pucOutYUV[2]; // 2 u -> v
            cachedFrame_->data[2] = hevcDecoderOutpusArgs_.pucOutYUV[1]; // 2 v -> u
        }
    }
    cachedFrame_->width = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecWidth);
    cachedFrame_->height = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecHeight);
    cachedFrame_->pts = static_cast<int64_t>(hevcDecoderOutpusArgs_.uiTimeStamp);
}

void HevcDecoder::SendFrame()
{
    if (state_ == State::STOPPING || state_ == State::FLUSHING) {
        return;
    } else if (state_ != State::RUNNING || isSendEos_ || codecAvailQue_->Size() == 0u || inputAvailQue_->Size() == 0u) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = inputAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<CodecBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    std::shared_ptr<AVBuffer> &inputAVBuffer = inputBuffer->avBuffer;
    if (inputAVBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        hevcDecoderInputArgs_.pStream = nullptr;
        isSendEos_ = true;
        AVCODEC_LOGI("Send eos end");
    } else {
        hevcDecoderInputArgs_.pStream = inputAVBuffer->memory_->GetAddr();
        hevcDecoderInputArgs_.uiStreamLen = static_cast<UINT32>(inputAVBuffer->memory_->GetSize());
        hevcDecoderInputArgs_.uiTimeStamp = static_cast<UINT64>(inputAVBuffer->pts_);
    }

#ifdef BUILD_ENG_VERSION
    if (dumpInFile_ && dumpInFile_->is_open() && !isSendEos_) {
        dumpInFile_->write(reinterpret_cast<char*>(inputAVBuffer->memory_->GetAddr()),
                           static_cast<int32_t>(inputAVBuffer->memory_->GetSize()));
    }
#endif

    int32_t ret = 0;
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    do {
        ret = DecodeFrameOnce();
        if (!isSendEos_) {
            hevcDecoderInputArgs_.uiStreamLen -= hevcDecoderOutpusArgs_.uiBytsConsumed;
            hevcDecoderInputArgs_.pStream += hevcDecoderOutpusArgs_.uiBytsConsumed;
        }
    } while ((ret != -1) && ((!isSendEos_ && hevcDecoderInputArgs_.uiStreamLen != 0) || (isSendEos_ && ret == 0)));
    runLock.unlock();

    if (isSendEos_) {
        auto outIndex = codecAvailQue_->Front();
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][outIndex];
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        FramePostProcess(buffers_[INDEX_OUTPUT][outIndex], outIndex, AVCS_ERR_OK);
        state_ = State::EOS;
    }

    inputAvailQue_->Pop();
    inputBuffer->owner_ = Owner::OWNED_BY_USER;
    callback_->OnInputBufferAvailable(index, inputAVBuffer);
}

int32_t HevcDecoder::DecodeFrameOnce()
{
    int32_t ret = 0;
    if (hevcSDecoder_ == nullptr || hevcDecoderFlushFrameFunc_ == nullptr ||
        hevcDecoderDecodecFrameFunc_ == nullptr) {
        AVCODEC_LOGE("Num %{public}u Hevc decoder resources not available, cannot call decoder", decInstanceID_);
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_VID_DEC_FAILED);
        return -1;
    }
    if (isSendEos_) {
        ret = hevcDecoderFlushFrameFunc_(hevcSDecoder_, &hevcDecoderOutpusArgs_);
    } else {
        ret = hevcDecoderDecodecFrameFunc_(hevcSDecoder_, &hevcDecoderInputArgs_, &hevcDecoderOutpusArgs_);
    }

    if (ret == 0) {
        int32_t bitDepth = static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecBitDepth);
        CHECK_AND_RETURN_RET_LOG(bitDepth == BITS_PER_PIXEL_COMPONENT_8 || bitDepth == BITS_PER_PIXEL_COMPONENT_10, -1,
                                 "Unsupported bitDepth %{public}d", bitDepth);
        ConvertDecOutToAVFrame(bitDepth);
#ifdef BUILD_ENG_VERSION
        DumpOutputBuffer(bitDepth);
#endif
        auto index = codecAvailQue_->Front();
        CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
        int32_t status = AVCS_ERR_OK;
        std::unique_lock<std::mutex> convertLock(convertDataMutex_);
        int32_t width = cachedFrame_->width;
        int32_t height = cachedFrame_->height;
        convertLock.unlock();
        if (CheckFormatChange(index, width, height, bitDepth) == AVCS_ERR_OK) {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            frameBuffer = buffers_[INDEX_OUTPUT][index];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
            state_ = State::ERROR;
            return -1;
        }
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        FramePostProcess(frameBuffer, index, status);
    } else if (ret < -1 || (!isSendEos_ && ret == -1)) {
        AVCODEC_LOGE("Num %{public}u Hevc decode error: ret = %{public}d", decInstanceID_, ret);
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_STREAM);
    }
    return ret;
}

void HevcDecoder::ConfigureHdrColorSpaceIno(const Format &format)
{
    int32_t tmpValue = 0;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, tmpValue)) {
        AVCODEC_LOGE("format_ get MD_KEY_RANGE_FLAG failed!");
        return;
    }
    colorSpaceInfo_.fullRangeFlag = static_cast<UINT32>(tmpValue);

    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, tmpValue)) {
        AVCODEC_LOGE("format_ get MD_KEY_COLOR_PRIMARIES failed!");
        return;
    }
    colorSpaceInfo_.colorPrimaries = static_cast<UINT32>(tmpValue);

    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, tmpValue)) {
        AVCODEC_LOGE("format_ get MD_KEY_TRANSFER_CHARACTERISTICS failed!");
        return;
    }
    colorSpaceInfo_.transferCharacteristic = static_cast<UINT32>(tmpValue);

    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, tmpValue)) {
        AVCODEC_LOGE("format_ get MD_KEY_MATRIX_COEFFICIENTS failed!");
        return;
    }
    colorSpaceInfo_.matrixCoeffs = static_cast<UINT32>(tmpValue);

    colorSpaceInfo_.colorDescriptionPresentFlag = 1;
    AVCODEC_LOGD("Config hdr color space infor suc: range %{public}u prm %{public}u chrct %{public}u coeff %{public}u",
        colorSpaceInfo_.fullRangeFlag, colorSpaceInfo_.colorPrimaries, colorSpaceInfo_.transferCharacteristic,
        colorSpaceInfo_.matrixCoeffs);
}

void HevcDecoder::ConfigureHdrStaticMetadata(const Format &format)
{
    if (isValidHdrSttMd_.load()) {
        return;
    }

    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    hdrSttMetadata_ = std::make_shared<HdrStaticMetadata>();
    CHECK_AND_RETURN_LOG(hdrSttMetadata_ != nullptr, "Memory allocation failed!!");
    (void)memset_s(hdrSttMetadata_.get(), sizeof(HdrStaticMetadata), 0, sizeof(HdrStaticMetadata));

    size_t extraSize = 0;
    uint8_t *extraData = nullptr;
    format.GetBuffer(OHOS::Media::Tag::VIDEO_STATIC_METADATA_SMPT2086, &extraData, extraSize);
    const OHOS::Media::Plugins::Smpte2086 *smpte = (OHOS::Media::Plugins::Smpte2086 *)extraData;
    if (smpte != nullptr && extraSize == sizeof(OHOS::Media::Plugins::Smpte2086)) {
        hdrSttMetadata_->smpte2086.displayPrimaryRed.x = smpte->displayPrimaryRedX;
        hdrSttMetadata_->smpte2086.displayPrimaryRed.y  = smpte->displayPrimaryRedY;
        hdrSttMetadata_->smpte2086.displayPrimaryGreen.x   = smpte->displayPrimaryGreenX;
        hdrSttMetadata_->smpte2086.displayPrimaryGreen.y = smpte->displayPrimaryGreenY;
        hdrSttMetadata_->smpte2086.displayPrimaryBlue.x  = smpte->displayPrimaryBlueX;
        hdrSttMetadata_->smpte2086.displayPrimaryBlue.y   = smpte->displayPrimaryBlueY;

        hdrSttMetadata_->smpte2086.whitePoint.x = smpte->whitePointX;
        hdrSttMetadata_->smpte2086.whitePoint.y = smpte->whitePointY;

        hdrSttMetadata_->smpte2086.maxLuminance = smpte->maxLuminance;
        hdrSttMetadata_->smpte2086.minLuminance = smpte->minLuminance;
    } else {
        AVCODEC_LOGE("Get HDR smpt2086 from format failed");
        return;
    }

    extraData = nullptr;
    format.GetBuffer(OHOS::Media::Tag::VIDEO_STATIC_METADATA_CTA861, &extraData, extraSize);
    const OHOS::Media::Plugins::Cta861  *cta = (OHOS::Media::Plugins::Cta861 *)extraData;
    if (cta != nullptr && extraSize == sizeof(OHOS::Media::Plugins::Cta861)) {
        hdrSttMetadata_->cta861.maxContentLightLevel       = cta->maxContentLightLevel;
        hdrSttMetadata_->cta861.maxFrameAverageLightLevel  = cta->maxFrameAverageLightLevel;
    } else {
        AVCODEC_LOGW("Get HDR cta861 from format warning");
    }

    isValidHdrSttMd_.store(true);
    AVCODEC_LOGD("Config hdr static metadate successful");
}

void HevcDecoder::UpdateColorAspects(const HEVC_COLOR_SPACE_INFO &colorInfo)
{
    if (colorSpaceInfo_.fullRangeFlag != colorInfo.fullRangeFlag ||
        colorSpaceInfo_.colorDescriptionPresentFlag != colorInfo.colorDescriptionPresentFlag ||
        colorSpaceInfo_.colorPrimaries != colorInfo.colorPrimaries ||
        colorSpaceInfo_.transferCharacteristic != colorInfo.transferCharacteristic ||
        colorSpaceInfo_.matrixCoeffs != colorInfo.matrixCoeffs) {
        colorSpaceInfo_ = colorInfo;
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG,
                            static_cast<int32_t>(colorSpaceInfo_.fullRangeFlag));
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES,
                            static_cast<int32_t>(colorSpaceInfo_.colorPrimaries));
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS,
                            static_cast<int32_t>(colorSpaceInfo_.transferCharacteristic));
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS,
                            static_cast<int32_t>(colorSpaceInfo_.matrixCoeffs));
        AVCODEC_LOGI("color aspects: isFullRange %{public}u, primary %{public}u, transfer %{public}u,"
            "matrix %{public}u", colorSpaceInfo_.fullRangeFlag, colorSpaceInfo_.colorPrimaries,
            colorSpaceInfo_.transferCharacteristic, colorSpaceInfo_.matrixCoeffs);
        callback_->OnOutputFormatChanged(format_);
    }
}

int32_t HevcDecoder::ConvertHdrStaticMetadata(const HEVC_HDR_METADATA &hevcHdrMetadata,
                                              std::vector<uint8_t> &staticMetadataVec)
{
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    CHECK_AND_RETURN_RET_LOG(staticMetadataVec.size() == sizeof(HdrStaticMetadata), AVCS_ERR_INVALID_VAL,
        "wrong staticMetadataVec.size : %{public}d", static_cast<int32_t>(staticMetadataVec.size()));
    HdrStaticMetadata* hdrStaticMetadata = reinterpret_cast<HdrStaticMetadata*>(staticMetadataVec.data());
    CHECK_AND_RETURN_RET_LOG(hdrStaticMetadata != nullptr, AVCS_ERR_INVALID_VAL,
        "vector convert to HdrStaticMetadata error");

    hdrStaticMetadata->smpte2086.displayPrimaryGreen.x = hevcHdrMetadata.displayPrimariesX[0] * PRIMARY_SCALE; // 0: Y
    hdrStaticMetadata->smpte2086.displayPrimaryBlue.x  = hevcHdrMetadata.displayPrimariesX[1] * PRIMARY_SCALE; // 1: U
    hdrStaticMetadata->smpte2086.displayPrimaryRed.x   = hevcHdrMetadata.displayPrimariesX[2] * PRIMARY_SCALE; // 2: V
    hdrStaticMetadata->smpte2086.displayPrimaryGreen.y = hevcHdrMetadata.displayPrimariesY[0] * PRIMARY_SCALE; // 0: Y
    hdrStaticMetadata->smpte2086.displayPrimaryBlue.y  = hevcHdrMetadata.displayPrimariesY[1] * PRIMARY_SCALE; // 1: U
    hdrStaticMetadata->smpte2086.displayPrimaryRed.y   = hevcHdrMetadata.displayPrimariesY[2] * PRIMARY_SCALE; // 2: V

    hdrStaticMetadata->smpte2086.whitePoint.x = hevcHdrMetadata.whitePointX * PRIMARY_SCALE;
    hdrStaticMetadata->smpte2086.whitePoint.y = hevcHdrMetadata.whitePointY * PRIMARY_SCALE;

    hdrStaticMetadata->smpte2086.maxLuminance = hevcHdrMetadata.maxDisplayMasteringLuminance * LUMI_SCALE;
    hdrStaticMetadata->smpte2086.minLuminance = hevcHdrMetadata.minDisplayMasteringLuminance  * LUMI_SCALE;
    if (hdrStaticMetadata->smpte2086.maxLuminance < hdrStaticMetadata->smpte2086.minLuminance) {
        std::swap(hdrStaticMetadata->smpte2086.maxLuminance, hdrStaticMetadata->smpte2086.minLuminance);
    }

    hdrStaticMetadata->cta861.maxContentLightLevel = static_cast<float>(hevcHdrMetadata.maxContentLightLevel);
    hdrStaticMetadata->cta861.maxFrameAverageLightLevel = static_cast<float>(hevcHdrMetadata.maxPicAverageLightLevel);
    return AVCS_ERR_OK;
}

void HevcDecoder::FillHdrStaticInfo(sptr<SurfaceBuffer> surfaceBuffer)
{
    if (colorSpaceInfo_.colorDescriptionPresentFlag != 1) {
        return;
    }

    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    std::vector<uint8_t> colorSpaceInfoVec;
    int32_t convertRet = ConvertParamsToColorSpaceInfo(colorSpaceInfo_.fullRangeFlag,
                                                       colorSpaceInfo_.colorPrimaries,
                                                       colorSpaceInfo_.transferCharacteristic,
                                                       colorSpaceInfo_.matrixCoeffs,
                                                       colorSpaceInfoVec);
    CHECK_AND_RETURN_LOG(convertRet == AVCS_ERR_OK, "ConvertParamsToColorSpaceInfo failed");
    GSError setRet = surfaceBuffer->SetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
    CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_COLORSPACE_INFO failed");

    if (isValidHdrSttMd_.load()) {
        std::vector<uint8_t> staticMetadataVec(sizeof(HdrStaticMetadata));
        HdrStaticMetadata* hdrStaticMetadata = reinterpret_cast<HdrStaticMetadata*>(staticMetadataVec.data());
        CHECK_AND_RETURN_LOG(hdrStaticMetadata != nullptr, "vector convert to HdrStaticMetadata error");
        *hdrStaticMetadata = *hdrSttMetadata_;
        setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadataVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_STATIC_METADATA failed");
    }

    std::vector<uint8_t> metadataTypeVec(sizeof(CM_HDR_Metadata_Type));
    CM_HDR_Metadata_Type* metadataType = reinterpret_cast<CM_HDR_Metadata_Type*>(metadataTypeVec.data());
    CHECK_AND_RETURN_LOG(metadataType != nullptr, "vector convert to CM_HDR_Metadata_Type error");
    *metadataType = static_cast<CM_HDR_Metadata_Type>(
                GetMetaDataTypeByTransFunc(colorSpaceInfo_.transferCharacteristic));
    setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_METADATA_TYPE, metadataTypeVec);
    CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_METADATA_TYPE failed");
    AVCODEC_LOGD("surface buffer Fill hdr static info successful");
}

void HevcDecoder::FillHdrInfo(sptr<SurfaceBuffer> surfaceBuffer)
{
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr, "surfaceBuffer is nullptr");
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    HEVC_COLOR_SPACE_INFO &outColorInfo = hevcDecoderOutpusArgs_.uiColorSpaceInfo;
    HEVC_HDR_METADATA &hdrMetadata = hevcDecoderOutpusArgs_.uiHdrMetadata;
    if (outColorInfo.colorDescriptionPresentFlag > 0) {
        std::vector<uint8_t> colorSpaceInfoVec;
        int32_t convertRet = ConvertParamsToColorSpaceInfo(outColorInfo.fullRangeFlag,
                                                           outColorInfo.colorPrimaries,
                                                           outColorInfo.transferCharacteristic,
                                                           outColorInfo.matrixCoeffs,
                                                           colorSpaceInfoVec);
        CHECK_AND_RETURN_LOG(convertRet == AVCS_ERR_OK, "ConvertParamsToColorSpaceInfo failed");
        GSError setRet = surfaceBuffer->SetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_COLORSPACE_INFO failed");
        std::vector<uint8_t> metadataTypeVec(sizeof(CM_HDR_Metadata_Type));
        CM_HDR_Metadata_Type* metadataType = reinterpret_cast<CM_HDR_Metadata_Type*>(metadataTypeVec.data());
        CHECK_AND_RETURN_LOG(metadataType != nullptr, "vector convert to CM_HDR_Metadata_Type error");
        if (hdrMetadata.paramsValidFlag > 0) {
            std::vector<uint8_t> staticMetadataVec(sizeof(HdrStaticMetadata));
            CHECK_AND_RETURN_LOG(ConvertHdrStaticMetadata(hdrMetadata, staticMetadataVec) == AVCS_ERR_OK,
                "ConvertHdrStaticMetadata error");
            setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadataVec);
            CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_STATIC_METADATA failed");
        }
        if (hdrMetadata.dynamicMetadataSize > 0) {
            CHECK_AND_RETURN_LOG(hdrMetadata.dynamicMetadata != nullptr, "empty hdr dynamicMetadata");
            std::vector<uint8_t> staticMetadataVec(hdrMetadata.dynamicMetadata,
                hdrMetadata.dynamicMetadata + hdrMetadata.dynamicMetadataSize);
            setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, staticMetadataVec);
            CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_DYNAMIC_METADATA failed");
            *metadataType = CM_VIDEO_HDR_VIVID;
        } else {
            *metadataType = static_cast<CM_HDR_Metadata_Type>(
                GetMetaDataTypeByTransFunc(outColorInfo.transferCharacteristic));
        }
        setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_METADATA_TYPE, metadataTypeVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_METADATA_TYPE failed");
        UpdateColorAspects(outColorInfo);
        AVCODEC_LOGD("surface buffer Fill hdr info successful");
    } else {
        FillHdrStaticInfo(surfaceBuffer);
    }
}

void HevcDecoder::FlushAllFrames()
{
    int32_t ret = 0;
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    while (ret == 0 && hevcDecoderFlushFrameFunc_ != nullptr) {
        ret = hevcDecoderFlushFrameFunc_(hevcSDecoder_, &hevcDecoderOutpusArgs_);
    }
    runLock.unlock();
}

void HevcDecoder::CalculateBufferSize(void)
{
    if ((static_cast<UINT32>(width_ * height_ * VIDEO_PIX_DEPTH_YUV) >> 1) <= VIDEO_MIN_BUFFER_SIZE) {
        inputBufferSize_ = VIDEO_MIN_BUFFER_SIZE;
    } else {
        inputBufferSize_ = VIDEO_MAX_BUFFER_SIZE;
    }
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, inputBufferSize_);
    AVCODEC_LOGI("width = %{public}d, height = %{public}d, Input buffer size = %{public}d",
                 width_, height_, inputBufferSize_);
}
int32_t HevcDecoder::GetDecoderWidthStride(void)
{
    return hevcDecoderOutpusArgs_.uiDecStride == 0 ? width_ : static_cast<int32_t>(hevcDecoderOutpusArgs_.uiDecStride);
}

void HevcDecoder::ConfigureOutPutOrder(bool isDisplay)
{
    initParams_.eOutPutOrder = isDisplay ? 1 : 0;  // 1 by disply order
    AVCODEC_LOGI("Configure output, is display: %{public}d", isDisplay);
}

int32_t HevcDecoder::CheckHevcDecLibStatus()
{
    void* handle = dlopen(HEVC_DEC_LIB_PATH, RTLD_LAZY);
    if (handle != nullptr) {
        auto hevcDecoderCreateFunc = reinterpret_cast<CreateHevcDecoderFuncType>(
            dlsym(handle, HEVC_DEC_CREATE_FUNC_NAME));
        auto hevcDecoderDecodecFrameFunc = reinterpret_cast<DecodeFuncType>(
            dlsym(handle, HEVC_DEC_DECODE_FRAME_FUNC_NAME));
        auto hevcDecoderFlushFrameFunc = reinterpret_cast<FlushFuncType>(dlsym(handle, HEVC_DEC_FLUSH_FRAME_FUNC_NAME));
        auto hevcDecoderDeleteFunc = reinterpret_cast<DeleteFuncType>(dlsym(handle, HEVC_DEC_DELETE_FUNC_NAME));
        if (hevcDecoderCreateFunc == nullptr || hevcDecoderDecodecFrameFunc == nullptr ||
            hevcDecoderDeleteFunc == nullptr || hevcDecoderFlushFrameFunc == nullptr) {
                AVCODEC_LOGE("HevcDecoder hevcFuncMatch_ failed!");
                hevcDecoderCreateFunc = nullptr;
                hevcDecoderDecodecFrameFunc = nullptr;
                hevcDecoderFlushFrameFunc = nullptr;
                hevcDecoderDeleteFunc = nullptr;
                dlclose(handle);
                handle = nullptr;
            }
    }

    if (handle == nullptr) {
        return AVCS_ERR_UNSUPPORT;
    }
    dlclose(handle);
    handle = nullptr;

    return AVCS_ERR_OK;
}

int32_t HevcDecoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    CHECK_AND_RETURN_RET_LOG(CheckHevcDecLibStatus() == AVCS_ERR_OK, AVCS_ERR_UNSUPPORT,
                             "hevc decoder libs not available");

    for (uint32_t i = 0; i < SUPPORT_HEVC_DECODER_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_HEVC_DECODER[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_HEVC_DECODER[i].mimeType);
        capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;

        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = VIDEO_MIN_SIZE;
        capsData.width.maxVal = VIDEO_MAX_WIDTH_SIZE;
        capsData.height.minVal = VIDEO_MIN_SIZE;
        capsData.height.maxVal = VIDEO_MAX_HEIGHT_SIZE;
        capsData.frameRate.minVal = 0;
        capsData.frameRate.maxVal = VIDEO_FRAMERATE_DEFAULT_SIZE;
        capsData.bitrate.minVal = 1;
        capsData.bitrate.maxVal = VIDEO_BITRATE_MAX_SIZE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_SIZE;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_SIZE;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;

        capsData.pixFormat = {static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21)};
        capsData.graphicPixFormat = {
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010)
        };
        capsData.profiles = {static_cast<int32_t>(HEVC_PROFILE_MAIN), static_cast<int32_t>(HEVC_PROFILE_MAIN_10)};

        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_62); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(HEVC_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(HEVC_PROFILE_MAIN_10), levels));
        capsData.featuresMap.insert(std::make_pair(
            static_cast<int32_t>(AVCapabilityFeature::VIDEO_DECODER_OUTPUT_IN_DECODING_ORDER), Format()));
        capaArray.emplace_back(capsData);
    }
    return AVCS_ERR_OK;
}

void HevcDecLog(UINT32 channelId, IHW265VIDEO_ALG_LOG_LEVEL eLevel, INT8 *pMsg, ...)
{
    va_list args;
    int32_t maxSize = 1024; // 1024 max size of one log
    std::vector<char> buf(maxSize);
    if (buf.empty()) {
        AVCODEC_LOGE("HevcDecLog buffer is empty!");
        return;
    }
    va_start(args, reinterpret_cast<const char*>(pMsg));
    int32_t size = vsnprintf_s(buf.data(), buf.size(), buf.size()-1, reinterpret_cast<const char*>(pMsg), args);
    va_end(args);
    if (size >= maxSize) {
        size = maxSize - 1;
    }

    auto msg = std::string(buf.data(), size);
    
    if (eLevel <= IHW265VIDEO_ALG_LOG_ERROR) {
        switch (eLevel) {
            case IHW265VIDEO_ALG_LOG_ERROR: {
                AVCODEC_LOGE("%{public}s", msg.c_str());
                break;
            }
            case IHW265VIDEO_ALG_LOG_WARNING: {
                AVCODEC_LOGW("%{public}s", msg.c_str());
                break;
            }
            case IHW265VIDEO_ALG_LOG_INFO: {
                AVCODEC_LOGI("%{public}s", msg.c_str());
                break;
            }
            case IHW265VIDEO_ALG_LOG_DEBUG: {
                AVCODEC_LOGD("%{public}s", msg.c_str());
                break;
            }
            default: {
                AVCODEC_LOGI("%{public}s", msg.c_str());
                break;
            }
        }
    }

    return;
}

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
