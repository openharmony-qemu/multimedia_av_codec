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

#include <iostream>
#include <thread>
#include <malloc.h>
#include "syspara/parameters.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_codec_name.h"
#include "vpxDecoder.h"
#include "v1_0/cm_color_space.h"
#include "v1_0/hdr_static_metadata.h"
#include "v1_0/buffer_handle_meta_key_type.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "VpxDecoderLoader"};

const char *VP8_CODEC = "vp8";
const char *VP9_CODEC = "vp9";

constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;
} SUPPORT_VPX_DECODER[] = {
#ifdef SUPPORT_CODEC_VP8
    {AVCodecCodecName::VIDEO_DECODER_VP8_NAME, CodecMimeType::VIDEO_VP8},
#endif
#ifdef SUPPORT_CODEC_VP9
    {AVCodecCodecName::VIDEO_DECODER_VP9_NAME, CodecMimeType::VIDEO_VP9},
#endif
};
constexpr uint32_t SUPPORT_VPX_DECODER_NUM = sizeof(SUPPORT_VPX_DECODER) / sizeof(SUPPORT_VPX_DECODER[0]);
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VP8_MAX_WIDTH_SIZE = 3840;
constexpr int32_t VP8_MAX_HEIGHT_SIZE = 2160;
constexpr int32_t VP9_MAX_WIDTH_SIZE = 3840;
constexpr int32_t VP9_MAX_HEIGHT_SIZE = 2160;
constexpr int32_t VIDEO_MIN_WIDTH_SIZE = 4;
constexpr int32_t VIDEO_MIN_HEIGHT_SIZE = 4;
constexpr int32_t VP9_BLOCKPERSEC_SIZE = 648000; // MaxDisplayRate / (block_width*block_height)
constexpr int32_t VP8_BLOCKPERSEC_SIZE = 972000; // MaxDisplayRate / (block_width*block_height)
constexpr int32_t VP9_FRAMERATE_MAX_SIZE = 130;
constexpr int32_t VIDEO_FRAMERATE_DEFAULT_SIZE = 60;
constexpr int32_t VP8_BLOCKPERFRAME_SIZE = 32400; // MaxPicSize / (block_width*block_height)
constexpr int32_t VP9_BLOCKPERFRAME_SIZE = 32400; // MaxPicSize / (block_width*block_height)
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
using namespace OHOS::Media;
VpxDecoder::VpxDecoder(const std::string &name) : VideoDecoder(name)
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
        AVCODEC_LOGI("Num %{public}u Decoder entered, state: Uninitialized", decInstanceID_);
    } else {
        AVCODEC_LOGE("Decoder already has %{public}d instances, cannot has more instances", VIDEO_INSTANCE_SIZE);
        isValid_ = false;
    }
    InitParams();
    renderSurface_ = std::make_shared<RenderSurface>();
}

VpxDecoder::~VpxDecoder()
{
    ReleaseResource();
    callback_ = nullptr;
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

void VpxDecoder::InitParams()
{
    vpxDecoderInputArgs_.pStream = nullptr;
    vpxDecoderInputArgs_.uiStreamLen = 0;
    vpxDecoderInputArgs_.uiTimeStamp = 0;
}

void VpxDecoder::ConfigureHdrMetadata(const Format &format)
{
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, colorSpaceInfo_.fullRangeFlag)) {
        AVCODEC_LOGE("format_ get MD_KEY_RANGE_FLAG failed!");
        return;
    }
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, colorSpaceInfo_.colorPrimaries)) {
        AVCODEC_LOGE("format_ get MD_KEY_COLOR_PRIMARIES failed!");
        return;
    }
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS,
                            colorSpaceInfo_.transferCharacteristic)) {
        AVCODEC_LOGE("format_ get MD_KEY_TRANSFER_CHARACTERISTICS failed!");
        return;
    }
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, colorSpaceInfo_.matrixCoeffs)) {
        AVCODEC_LOGE("format_ get MD_KEY_MATRIX_COEFFICIENTS failed!");
        return;
    }
    size_t extraSize = 0;
    uint8_t *extraData = nullptr;
    format.GetBuffer(MediaDescriptionKey::MD_KEY_VIDEO_HDR_METADATA, &extraData, extraSize);
    const AVMasteringDisplayMetadata *metadata = (AVMasteringDisplayMetadata *)extraData;
    if (metadata == nullptr) {
        AVCODEC_LOGE("Get HDR metadata from format failed");
        return;
    }
    auto safeDiv = [](int num, int den) -> float {
        if (den == 0) {
            return 0.0;
        }
        return static_cast<float>(num) / static_cast<float>(den);
    };
    hdrMetadata_.displayPrimariesX[0] =
        safeDiv(metadata->display_primaries[0][0].num, metadata->display_primaries[0][0].den); // 0: Y
    hdrMetadata_.displayPrimariesY[0] =
        safeDiv(metadata->display_primaries[0][1].num, metadata->display_primaries[0][1].den); // 0: Y
    hdrMetadata_.displayPrimariesX[1] =
        safeDiv(metadata->display_primaries[1][0].num, metadata->display_primaries[1][0].den); // 1: U
    hdrMetadata_.displayPrimariesY[1] =
        safeDiv(metadata->display_primaries[1][1].num, metadata->display_primaries[1][1].den); // 1: U
    hdrMetadata_.displayPrimariesX[2] =
        safeDiv(metadata->display_primaries[2][0].num, metadata->display_primaries[2][0].den); // 2: V
    hdrMetadata_.displayPrimariesY[2] =
        safeDiv(metadata->display_primaries[2][1].num, metadata->display_primaries[2][1].den); // 2: V
    hdrMetadata_.whitePointX = safeDiv(metadata->white_point[0].num, metadata->white_point[0].den);
    hdrMetadata_.whitePointY = safeDiv(metadata->white_point[1].num, metadata->white_point[1].den);
    hdrMetadata_.maxDisplayMasteringLuminance = safeDiv(metadata->max_luminance.num, metadata->max_luminance.den);
    hdrMetadata_.minDisplayMasteringLuminance = safeDiv(metadata->min_luminance.num, metadata->min_luminance.den);
    colorSpaceInfo_.colorDescriptionPresentFlag = 1;
}

void VpxDecoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
    int32_t maxVal)
{
    int32_t val32 = 0;
    if (format.GetIntValue(formatKey, val32) && val32 >= minVal && val32 <= maxVal) {
        format_.PutIntValue(formatKey, val32);
    } else {
        AVCODEC_LOGW("Set parameter failed: %{public}s, which minimum threshold=%{public}d, "
                     "maximum threshold=%{public}d",
                     formatKey.data(), minVal, maxVal);
    }
}

bool VpxDecoder::CheckVideoPixelFormat(VideoPixelFormat vpf)
{
    if (vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21 || vpf == VideoPixelFormat::YUV420P) {
        return true;
    } else {
        AVCODEC_LOGE("Set parameter failed: pixel format value %{public}d invalid", vpf);
        return false;
    }
}

void VpxDecoder::ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth)
{
    if (codecName_ == AVCodecCodecName::VIDEO_DECODER_VP8_NAME) {
        if (isWidth == true) {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_WIDTH_SIZE, VP8_MAX_WIDTH_SIZE);
        } else {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_HEIGHT_SIZE, VP8_MAX_HEIGHT_SIZE);
        }
    } else {
        if (isWidth == true) {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_WIDTH_SIZE, VP9_MAX_WIDTH_SIZE);
        } else {
            ConfigureDefaultVal(format, formatKey, VIDEO_MIN_HEIGHT_SIZE, VP9_MAX_HEIGHT_SIZE);
        }
    }
}

int32_t VpxDecoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    decName_ = "VpxDecoder_["+ std::to_string(instanceId_) + "]";
    AVCODEC_LOGI("current codec name: %{public}s", decName_.c_str());
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_VPX_DECODER_NUM; ++i) {
        if (SUPPORT_VPX_DECODER[i].codecName == codecName_) {
            mime = SUPPORT_VPX_DECODER[i].mimeType;
            break;
        }
    }
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

int32_t VpxDecoder::CreateDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    int32_t createRet = 0;
    if (vpxDecHandle_ == nullptr) {
        if (codecName_ == AVCodecCodecName::VIDEO_DECODER_VP8_NAME) {
            createRet = VpxCreateDecoderFunc(&vpxDecHandle_, VP8_CODEC);
        } else {
            createRet = VpxCreateDecoderFunc(&vpxDecHandle_, VP9_CODEC);
        }
    }
    runLock.unlock();
    CHECK_AND_RETURN_RET_LOG(createRet == 0 && vpxDecHandle_ != nullptr, AVCS_ERR_INVALID_OPERATION,
        "vpx decoder create failed");
    return AVCS_ERR_OK;
}

void VpxDecoder::DeleteDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (vpxDecHandle_ != nullptr) {
        int ret = VpxDestroyDecoderFunc(&vpxDecHandle_);
        if (ret != 0) {
            AVCODEC_LOGE("Error: VpxDecoder delete error: %{public}d", ret);
            if (callback_ != nullptr) {
                callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN);
            }
            state_ = State::ERROR;
        }
        vpxDecHandle_ = nullptr;
    }
    runLock.unlock();
}

AVPixelFormat VpxDecoder::ConvertVpxFmtToAVPixFmt(vpx_img_fmt_t fmt)
{
    AVPixelFormat ret = AVPixelFormat::AV_PIX_FMT_NONE;
    switch (fmt) {
        case VPX_IMG_FMT_YV12:
            ret = AVPixelFormat::AV_PIX_FMT_YUV420P;
            break;
        case VPX_IMG_FMT_I420:
            ret = AVPixelFormat::AV_PIX_FMT_YUV420P;
            break;
        case VPX_IMG_FMT_I422:
            ret = AVPixelFormat::AV_PIX_FMT_YUV422P;
            break;
        case VPX_IMG_FMT_I444:
            ret = AVPixelFormat::AV_PIX_FMT_YUV444P;
            break;
        case VPX_IMG_FMT_I440:
            ret = AVPixelFormat::AV_PIX_FMT_YUV440P;
            break;
        case VPX_IMG_FMT_NV12:
            ret = AVPixelFormat::AV_PIX_FMT_NV12;
            break;
        case VPX_IMG_FMT_I42016:
            ret = AVPixelFormat::AV_PIX_FMT_YUV420P10LE;
            break;
        case VPX_IMG_FMT_I42216:
            ret = AVPixelFormat::AV_PIX_FMT_YUV422P10LE;
            break;
        case VPX_IMG_FMT_I44416:
            ret = AVPixelFormat::AV_PIX_FMT_YUV444P10LE;
            break;
        case VPX_IMG_FMT_I44016:
            ret = AVPixelFormat::AV_PIX_FMT_YUV440P10LE;
            break;
        default:
            break;
    }
    return ret;
}

void VpxDecoder::ConvertDecOutToAVFrame()
{
    if (cachedFrame_ == nullptr) {
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });
    }

    if (vpxDecOutputImg_ == nullptr) {
        return;
    }
    uint32_t planeY = 0;
    uint32_t planeU = 1;
    uint32_t planeV = 2;
    cachedFrame_->data[planeY] = vpxDecOutputImg_->planes[planeY]; /**< Y (Luminance) plane */
    cachedFrame_->data[planeU] = vpxDecOutputImg_->planes[planeU]; /**< U (Chroma) plane */
    cachedFrame_->data[planeV] = vpxDecOutputImg_->planes[planeV]; /**< V (Chroma) plane */

    if (vpxDecOutputImg_->bit_depth == BITS_PER_PIXEL_COMPONENT_10 && outputPixelFmt_ == VideoPixelFormat::NV21) {
        cachedFrame_->data[planeU] = vpxDecOutputImg_->planes[planeV]; /**< V (Chroma) plane */
        cachedFrame_->data[planeV] = vpxDecOutputImg_->planes[planeU]; /**< U (Chroma) plane */
    }

    cachedFrame_->format = static_cast<int>(ConvertVpxFmtToAVPixFmt(vpxDecOutputImg_->fmt));

    uint32_t channelY = 0;
    uint32_t channelU = 1;
    uint32_t channelV = 2;
    cachedFrame_->linesize[channelY] = static_cast<int32_t>(vpxDecOutputImg_->stride[channelY]); // 0 y channel
    cachedFrame_->linesize[channelU] = static_cast<int32_t>(vpxDecOutputImg_->stride[channelU]); // 1 u channel
    cachedFrame_->linesize[channelV] = static_cast<int32_t>(vpxDecOutputImg_->stride[channelV]); // 2 v channel

    cachedFrame_->width = static_cast<int32_t>(vpxDecOutputImg_->d_w);
    cachedFrame_->height = static_cast<int32_t>(vpxDecOutputImg_->d_h);
    cachedFrame_->pts = static_cast<int64_t>(vpxDecoderInputArgs_.uiTimeStamp);
}

void VpxDecoder::SendFrame()
{
    if (state_ == State::STOPPING || state_ == State::FLUSHING) {
        return;
    } else if (state_ != State::RUNNING || isSendEos_ ||
        codecAvailQue_->Size() == 0u || inputAvailQue_->Size() == 0u) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_TRY_DECODE_TIME));
        return;
    }
    uint32_t index = inputAvailQue_->Front();
    CHECK_AND_RETURN_LOG(state_ == State::RUNNING, "Not in running state");
    std::shared_ptr<CodecBuffer> &inputBuffer = buffers_[INDEX_INPUT][index];
    std::shared_ptr<AVBuffer> &inputAVBuffer = inputBuffer->avBuffer;
    if (inputAVBuffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        vpxDecoderInputArgs_.pStream = nullptr;
        vpxDecoderInputArgs_.uiStreamLen = 0;
        isSendEos_ = true;
        AVCODEC_LOGI("Send eos end");
    } else {
        vpxDecoderInputArgs_.pStream = inputAVBuffer->memory_->GetAddr();
        vpxDecoderInputArgs_.uiStreamLen = static_cast<UINT32>(inputAVBuffer->memory_->GetSize());
        vpxDecoderInputArgs_.uiTimeStamp = static_cast<UINT64>(inputAVBuffer->pts_);
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
    } while (isSendEos_ && ret == 0);
    runLock.unlock();

    if (isSendEos_) {
        auto outIndex = codecAvailQue_->Front();
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][outIndex];
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        FramePostProcess(buffers_[INDEX_OUTPUT][outIndex], outIndex, AVCS_ERR_OK);
        state_ = State::EOS;
    } else if (ret < 0) {
        AVCODEC_LOGE("decode frame error: ret = %{public}d", ret);
    }

    inputAvailQue_->Pop();
    inputBuffer->owner_ = Owner::OWNED_BY_USER;
    callback_->OnInputBufferAvailable(index, inputAVBuffer);
}

int32_t VpxDecoder::DecodeFrameOnce()
{
    int32_t ret = 0;
    if (vpxDecHandle_ != nullptr) {
        if (!isSendEos_) {
            ret = VpxDecodeFrameFunc(vpxDecHandle_, vpxDecoderInputArgs_.pStream, vpxDecoderInputArgs_.uiStreamLen);
        }
    } else {
        AVCODEC_LOGW("vpxDecoderFrameFunc_ = nullptr || vpxDecHandle_ = nullptr");
        ret = -1;
    }
    if (vpxDecHandle_ != nullptr) {
        VpxGetFrameFunc(vpxDecHandle_, &vpxDecOutputImg_);
        ret = vpxDecOutputImg_ == nullptr ? -1 : 0;
    } else {
        AVCODEC_LOGW("vpxDecoderGetFrameFunc_ = nullptr || vpxDecHandle_ = nullptr");
        ret = -1;
    }
    if (ret == 0 && vpxDecOutputImg_ != nullptr) {
        int32_t bitDepth = static_cast<int32_t>(vpxDecOutputImg_->bit_depth);
        ConvertDecOutToAVFrame();
#ifdef BUILD_ENG_VERSION
        DumpOutputBuffer(bitDepth);
#endif
        auto index = codecAvailQue_->Front();
        CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING, -1, "Not in running state");
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
        int32_t status = AVCS_ERR_OK;
        if (CheckFormatChange(index, cachedFrame_->width, cachedFrame_->height, bitDepth) == AVCS_ERR_OK) {
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
    }
    return ret;
}

void VpxDecoder::FlushAllFrames()
{
    std::unique_lock<std::mutex> runlock(decRunMutex_);
    if (vpxDecHandle_ != nullptr) {
        do {
            VpxGetFrameFunc(vpxDecHandle_, &vpxDecOutputImg_);
        } while (vpxDecOutputImg_ != nullptr);
    }
    runlock.unlock();
}

void VpxDecoder::GetVp9CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.width.maxVal = VP9_MAX_WIDTH_SIZE;
        capsData.height.maxVal = VP9_MAX_HEIGHT_SIZE;
        capsData.blockPerSecond.maxVal = VP9_BLOCKPERSEC_SIZE;
        capsData.blockPerFrame.maxVal = VP9_BLOCKPERFRAME_SIZE;
        capsData.frameRate.maxVal = VP9_FRAMERATE_MAX_SIZE;
        capsData.supportSwapWidthHeight = true;
        capsData.profiles = {static_cast<int32_t>(VP9_PROFILE_0), static_cast<int32_t>(VP9_PROFILE_1)};
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(VP9Level::VP9_LEVEL_62); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VP9_PROFILE_0), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VP9_PROFILE_1), levels));
    }
}

void VpxDecoder::GetVp8CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.blockPerSecond.maxVal = VP8_BLOCKPERSEC_SIZE;
        capsData.blockPerFrame.maxVal = VP8_BLOCKPERFRAME_SIZE;
        capsData.width.maxVal = VP8_MAX_WIDTH_SIZE;
        capsData.height.maxVal = VP8_MAX_HEIGHT_SIZE;
        capsData.supportSwapWidthHeight = true;
    }
}

int32_t VpxDecoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    for (uint32_t i = 0; i < SUPPORT_VPX_DECODER_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_VPX_DECODER[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_VPX_DECODER[i].mimeType);
        capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
        capsData.isVendor = false;
        capsData.maxInstance = VIDEO_INSTANCE_SIZE;
        capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
        capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
        capsData.width.minVal = VIDEO_MIN_WIDTH_SIZE;
        capsData.height.minVal = VIDEO_MIN_HEIGHT_SIZE;
        capsData.frameRate.minVal = 0;
        capsData.frameRate.maxVal = VIDEO_FRAMERATE_DEFAULT_SIZE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerSecond.minVal = 1;
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
        if (capsData.mimeType == "video/vp9") {
            capaArray.emplace_back(capsData);
            GetVp9CapProf(capaArray);
        } else {
            capaArray.emplace_back(capsData);
            GetVp8CapProf(capaArray);
        }
    }
    return AVCS_ERR_OK;
}

void VpxDecoder::FillHdrInfo(sptr<SurfaceBuffer> surfaceBuffer)
{
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr, "surfaceBuffer is nullptr");
    if (colorSpaceInfo_.colorDescriptionPresentFlag) {
        std::vector<uint8_t> colorSpaceInfoVec;

        int32_t convertRet = ConvertParamsToColorSpaceInfo(colorSpaceInfo_.fullRangeFlag,
                                                           colorSpaceInfo_.colorPrimaries,
                                                           colorSpaceInfo_.transferCharacteristic,
                                                           colorSpaceInfo_.matrixCoeffs,
                                                           colorSpaceInfoVec);
        
        CHECK_AND_RETURN_LOG(convertRet == AVCS_ERR_OK, "ConvertParamsToColorSpaceInfo failed,convertRet");
        GSError setRet = surfaceBuffer->SetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_COLORSPACE_INFO failed,setRet = %{public}d",
            static_cast<int32_t>(setRet));

        std::vector<uint8_t> staticMetadataVec(sizeof(HdrStaticMetadata));
        CHECK_AND_RETURN_LOG(ConvertHdrStaticMetadata(hdrMetadata_, staticMetadataVec) == AVCS_ERR_OK,
            "ConvertHdrStaticMetadata error");
        setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadataVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_STATIC_METADATA failed");

        std::vector<uint8_t> metadataTypeVec(sizeof(CM_HDR_Metadata_Type));
        CM_HDR_Metadata_Type* metadataType = reinterpret_cast<CM_HDR_Metadata_Type*>(metadataTypeVec.data());
        CHECK_AND_RETURN_LOG(metadataType != nullptr, "vector convert to CM_HDR_Metadata_Type error");
        *metadataType =
            static_cast<CM_HDR_Metadata_Type>(GetMetaDataTypeByTransFunc(colorSpaceInfo_.transferCharacteristic));
        setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_METADATA_TYPE, metadataTypeVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_METADATA_TYPE failed");
        AVCODEC_LOGD("surface buffer Fill hdr info successful");
    }
}

int32_t VpxDecoder::ConvertHdrStaticMetadata(const HdrMetadata &hdrMetadata, std::vector<uint8_t> &staticMetadataVec)
{
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    CHECK_AND_RETURN_RET_LOG(staticMetadataVec.size() == sizeof(HdrStaticMetadata), AVCS_ERR_INVALID_VAL,
        "wrong staticMetadataVec.size : %{public}d", static_cast<int32_t>(staticMetadataVec.size()));
    HdrStaticMetadata* hdrStaticMetadata = reinterpret_cast<HdrStaticMetadata*>(staticMetadataVec.data());
    CHECK_AND_RETURN_RET_LOG(hdrStaticMetadata != nullptr, AVCS_ERR_INVALID_VAL,
        "vector convert to CM_HDR_Metadata_Type error");
    hdrStaticMetadata->smpte2086.displayPrimaryGreen.x = hdrMetadata.displayPrimariesX[0]; // 0: Y
    hdrStaticMetadata->smpte2086.displayPrimaryBlue.x = hdrMetadata.displayPrimariesX[1]; // 1: U
    hdrStaticMetadata->smpte2086.displayPrimaryRed.x = hdrMetadata.displayPrimariesX[2]; // 2: V
    hdrStaticMetadata->smpte2086.displayPrimaryGreen.y = hdrMetadata.displayPrimariesY[0]; // 0: Y
    hdrStaticMetadata->smpte2086.displayPrimaryBlue.y = hdrMetadata.displayPrimariesY[1]; // 1: U
    hdrStaticMetadata->smpte2086.displayPrimaryRed.y = hdrMetadata.displayPrimariesY[2]; // 2: V
    hdrStaticMetadata->smpte2086.whitePoint.x = hdrMetadata.whitePointX;
    hdrStaticMetadata->smpte2086.whitePoint.y = hdrMetadata.whitePointY;
    hdrStaticMetadata->smpte2086.maxLuminance = hdrMetadata.maxDisplayMasteringLuminance;
    hdrStaticMetadata->smpte2086.minLuminance = hdrMetadata.minDisplayMasteringLuminance;
    return AVCS_ERR_OK;
}

int VpxDecoder::VpxCreateDecoderFunc(void **vpxDecoder, const char *name)
{
    const VpxInterface *decoder = NULL;
    vpx_codec_ctx_t *ctx = (vpx_codec_ctx_t *)malloc(sizeof(*ctx));
    if (!ctx) {
        AVCODEC_LOGE("Error: Failed to allocate memory for vpx_codec_ctx_t");
        return -1;
    }

    decoder = get_vpx_decoder_by_name(name);
    if (!decoder) {
        free(ctx);
        AVCODEC_LOGE("Error: Unknown input codec!");
        return -1;
    }
    if (vpx_codec_dec_init(ctx, decoder->codec_interface(), NULL, VPX_CODEC_USE_FRAME_THREADING)) {
        free(ctx);
        AVCODEC_LOGE("Error: Failed to initialize decoder.");
        return -1;
    }
    *vpxDecoder = ctx;
    return 0;
}

int VpxDecoder::VpxDestroyDecoderFunc(void **vpxDecoder)
{
    if (vpxDecoder == NULL || *vpxDecoder == NULL) {
        return -1;
    }
    vpx_codec_ctx_t *codec = (vpx_codec_ctx_t *)(*vpxDecoder);
    vpx_codec_err_t res = vpx_codec_destroy(codec);
    free(codec);
    *vpxDecoder = NULL;
    if (res != VPX_CODEC_OK) {
        AVCODEC_LOGE("Error: Failed to destroy decoder: %s\n", vpx_codec_err_to_string(res));
        return -1;
    }
    return 0;
}

int VpxDecoder::VpxDecodeFrameFunc(void *vpxDecoder, const unsigned char *frame, unsigned int frameSize)
{
    vpx_codec_ctx_t *codec = (vpx_codec_ctx_t *)vpxDecoder;
    int ret = vpx_codec_decode(codec, frame, (unsigned int)frameSize, NULL, 0);
    if (ret) {
        AVCODEC_LOGE("Error: Failed to decode frame.");
        return ret;
    }
    return 0;
}

int VpxDecoder::VpxGetFrameFunc(void *vpxDecoder, vpx_image_t **outputImg)
{
    vpx_codec_ctx_t *codec = (vpx_codec_ctx_t *)vpxDecoder;
    vpx_codec_iter_t iter = NULL;
    *outputImg = vpx_codec_get_frame(codec, &iter);
    return 0;
}

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
