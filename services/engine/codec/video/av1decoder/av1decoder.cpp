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
#include <set>
#include <thread>
#include <dlfcn.h>
#include <malloc.h>
#include "syspara/parameters.h"
#include "securec.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "utils.h"
#include "avcodec_codec_name.h"
#include "av1decoder.h"
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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Av1DecoderLoader"};

constexpr struct {
    const std::string_view codecName;
    const std::string_view mimeType;

} SUPPORT_AV1_DECODER[] = {
#ifdef SUPPORT_CODEC_AV1
    {AVCodecCodecName::VIDEO_DECODER_AV1_NAME, CodecMimeType::VIDEO_AV1},
#endif
};
constexpr uint32_t SUPPORT_AV1_DECODER_NUM = sizeof(SUPPORT_AV1_DECODER) / sizeof(SUPPORT_AV1_DECODER[0]);
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MIN_SIZE = 4;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 1920;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 1080;
constexpr int32_t VIDEO_BLOCKPERFRAME_MAX_SIZE = 8160; // MaxPicSize / (BlockWidth * blockHeight)
constexpr int32_t VIDEO_BLOCKPERSEC_MAX_SIZE = 326400; // MaxDisplayRate / (BlockWidth * blockHeight)
constexpr uint32_t DEFAULT_TRY_DECODE_TIME = 1;
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;
constexpr int32_t VIDEO_MAX_FRAMERATE = 300;
constexpr int32_t DAV1D_AGAIN = -11;
static constexpr float PRIMARY_SCALE = 0.00002;
static constexpr float LUMI_SCALE    = 0.0001;
using namespace OHOS::Media;

Av1Decoder::Av1Decoder(const std::string &name) : VideoDecoder(name)
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

Av1Decoder::~Av1Decoder()
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

void Av1Decoder::InitParams()
{
    av1DecoderInputArgs_.pStream = nullptr;
    av1DecoderInputArgs_.uiStreamLen = 0;
    av1DecoderInputArgs_.uiTimeStamp = 0;
    colorSpaceInfo_.color_range = 0;
    colorSpaceInfo_.pri = DAV1D_COLOR_PRI_UNKNOWN;
    colorSpaceInfo_.trc = DAV1D_TRC_UNKNOWN;
    colorSpaceInfo_.mtrx = DAV1D_MC_UNKNOWN;
}

void Av1Decoder::ConfigureDefaultVal(const Format &format, const std::string_view &formatKey, int32_t minVal,
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

bool Av1Decoder::CheckVideoPixelFormat(VideoPixelFormat vpf)
{
    if (vpf == VideoPixelFormat::NV12 || vpf == VideoPixelFormat::NV21) {
        return true;
    } else {
        AVCODEC_LOGE("Set parameter failed: pixel format value %{public}d invalid", vpf);
        return false;
    }
}

void Av1Decoder::ConfigurelWidthAndHeight(const Format &format, const std::string_view &formatKey, bool isWidth)
{
    if (isWidth == true) {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_SIZE, VIDEO_MAX_WIDTH_SIZE);
    } else {
        ConfigureDefaultVal(format, formatKey, VIDEO_MIN_SIZE, VIDEO_MAX_HEIGHT_SIZE);
    }
}

int32_t Av1Decoder::Initialize()
{
    AVCODEC_SYNC_TRACE;
    decName_ = "Av1Decoder_["+ std::to_string(instanceId_) + "]";
    AVCODEC_LOGI("current codec name: %{public}s", decName_.c_str());
    CHECK_AND_RETURN_RET_LOG(!codecName_.empty(), AVCS_ERR_INVALID_VAL, "Init codec failed:  empty name");
    std::string_view mime;
    for (uint32_t i = 0; i < SUPPORT_AV1_DECODER_NUM; ++i) {
        if (SUPPORT_AV1_DECODER[i].codecName == codecName_) {
            mime = SUPPORT_AV1_DECODER[i].mimeType;
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

int32_t Av1Decoder::CreateDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    int32_t createRet = 0;
    if (dav1dCtx_ == nullptr) {
        Dav1dSettings dav1dSettings;
        dav1d_default_settings(&dav1dSettings);
        dav1dSettings.logger.callback = AV1DecLog;
        dav1dSettings.n_threads = 2; // 2 threads
        dav1dSettings.max_frame_delay = 1;
        dav1dSettings.apply_grain = 1;
        createRet = dav1d_open(&dav1dCtx_, &dav1dSettings);
    }
    runLock.unlock();
    CHECK_AND_RETURN_RET_LOG(createRet >= 0 && dav1dCtx_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "av1 decoder create failed");
    return AVCS_ERR_OK;
}

void Av1Decoder::DeleteDecoder()
{
    std::unique_lock<std::mutex> runLock(decRunMutex_);
    if (dav1dCtx_ != nullptr) {
        dav1d_close(&dav1dCtx_);
    }
    dav1dCtx_ = nullptr;
    runLock.unlock();
}

AVPixelFormat Av1Decoder::ConvertAv1FmtToAVPixFmt(Dav1dPixelLayout fmt, int32_t bpc)
{
    AVPixelFormat ret = AVPixelFormat::AV_PIX_FMT_NONE;
    if (bpc == BITS_PER_PIXEL_COMPONENT_8) {
        switch (fmt) {
            case DAV1D_PIXEL_LAYOUT_I400:
                ret = AVPixelFormat::AV_PIX_FMT_GRAY8;
                break;
            case DAV1D_PIXEL_LAYOUT_I420:
                ret = AVPixelFormat::AV_PIX_FMT_YUV420P;
                break;
            case DAV1D_PIXEL_LAYOUT_I422:
                ret = AVPixelFormat::AV_PIX_FMT_YUV422P;
                break;
            case DAV1D_PIXEL_LAYOUT_I444:
                ret = AVPixelFormat::AV_PIX_FMT_YUV444P;
                break;
            default:
                break;
        }
    } else if (bpc == BITS_PER_PIXEL_COMPONENT_10) {
        switch (fmt) {
            case DAV1D_PIXEL_LAYOUT_I400:
                ret = AVPixelFormat::AV_PIX_FMT_GRAY10LE;
                break;
            case DAV1D_PIXEL_LAYOUT_I420:
                ret = AVPixelFormat::AV_PIX_FMT_YUV420P10LE;
                break;
            case DAV1D_PIXEL_LAYOUT_I422:
                ret = AVPixelFormat::AV_PIX_FMT_YUV422P10LE;
                break;
            case DAV1D_PIXEL_LAYOUT_I444:
                ret = AVPixelFormat::AV_PIX_FMT_YUV444P10LE;
                break;
            default:
                break;
        }
    }
    return ret;
}

void Av1Decoder::ConvertDecOutToAVFrame()
{
    if (cachedFrame_ == nullptr) {
        cachedFrame_ = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });
    }

    if (av1DecOutputImg_ == nullptr) {
        return;
    }
    uint32_t planeY = 0;
    uint32_t planeU = 1;
    uint32_t planeV = 2;
    cachedFrame_->data[planeY] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeY]); // Y
    cachedFrame_->data[planeU] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeU]); // U
    cachedFrame_->data[planeV] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeV]); // V

    cachedFrame_->format =
        static_cast<int>(ConvertAv1FmtToAVPixFmt(av1DecOutputImg_->p.layout, av1DecOutputImg_->p.bpc));
    uint32_t channelY = 0;
    uint32_t channelUV = 1;

    cachedFrame_->linesize[0] = static_cast<int32_t>(av1DecOutputImg_->stride[channelY]); // 0 y channel
    cachedFrame_->linesize[1] = static_cast<int32_t>(av1DecOutputImg_->stride[channelUV]); // 1 u channel
    cachedFrame_->linesize[2] = static_cast<int32_t>(av1DecOutputImg_->stride[channelUV]); // 2 v channel
    if (av1DecOutputImg_->p.bpc == BITS_PER_PIXEL_COMPONENT_10 &&
        outputPixelFmt_ == VideoPixelFormat::NV21) { // exchange uv channel
        cachedFrame_->data[planeU] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeV]);
        cachedFrame_->data[planeV] = static_cast<uint8_t*>(av1DecOutputImg_->data[planeU]);
    }

    cachedFrame_->width = static_cast<int32_t>(av1DecOutputImg_->p.w);
    cachedFrame_->height = static_cast<int32_t>(av1DecOutputImg_->p.h);
    cachedFrame_->pts = av1DecOutputImg_->m.timestamp;
}

void Av1Decoder::SendFrame()
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
        av1DecoderInputArgs_.pStream = nullptr;
        av1DecoderInputArgs_.uiStreamLen = 0;
        isSendEos_ = true;
        AVCODEC_LOGI("Send eos end");
    } else {
        av1DecoderInputArgs_.pStream = inputAVBuffer->memory_->GetAddr();
        av1DecoderInputArgs_.uiStreamLen = static_cast<UINT32>(inputAVBuffer->memory_->GetSize());
        av1DecoderInputArgs_.uiTimeStamp = inputAVBuffer->pts_;
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

int32_t Av1Decoder::DecodeAv1FrameOnce()
{
    int32_t ret = 0;
    if (dav1dCtx_ != nullptr) {
        if (!isSendEos_) {
            Dav1dData dav1dDataBuf;
            ret = dav1d_data_wrap(&dav1dDataBuf, av1DecoderInputArgs_.pStream, av1DecoderInputArgs_.uiStreamLen,
                                  AV1FreeCallback, nullptr);
            CHECK_AND_RETURN_RET_LOG(ret >= 0, ret, "feed av1 input stream failed!");
            dav1dDataBuf.m.timestamp = av1DecoderInputArgs_.uiTimeStamp;
            ret = dav1d_send_data(dav1dCtx_, &dav1dDataBuf);
            if (dav1dDataBuf.sz > 0) {
                dav1d_data_unref(&dav1dDataBuf);
            }
        }
        ret = dav1d_get_picture(dav1dCtx_, av1DecOutputImg_);
        if (ret < 0 && ret != DAV1D_AGAIN) {
            dav1d_picture_unref(av1DecOutputImg_);
            av1DecOutputImg_ = nullptr;
        }
    } else {
        AVCODEC_LOGE("dav1d decoder is null, send data to av1 decoder failed.");
        ret = -1;
    }
    return ret;
}

bool Av1Decoder::CheckStateRunning()
{
    if (state_ != State::RUNNING) {
        if (av1DecOutputImg_ != nullptr) {
            dav1d_picture_unref(av1DecOutputImg_);
            delete av1DecOutputImg_;
            av1DecOutputImg_ = nullptr;
        }
        return false;
    }
    return true;
}

int32_t Av1Decoder::DecodeFrameOnce()
{
    av1DecOutputImg_ = new Dav1dPicture{0};
    int32_t ret = DecodeAv1FrameOnce();
    if (ret == 0 && av1DecOutputImg_ != nullptr) {
        int32_t bitDepth = av1DecOutputImg_->p.bpc;
        ConvertDecOutToAVFrame();
#ifdef BUILD_ENG_VERSION
        DumpOutputBuffer(bitDepth);
#endif
        auto index = codecAvailQue_->Front();
        CHECK_AND_RETURN_RET_LOG(CheckStateRunning(), -1, "Not in running state");
        std::shared_ptr<CodecBuffer> frameBuffer = buffers_[INDEX_OUTPUT][index];
        int32_t status = AVCS_ERR_OK;
        std::unique_lock<std::mutex> convertLock(convertDataMutex_);
        int32_t width = cachedFrame_->width;
        int32_t height = cachedFrame_->height;
        convertLock.unlock();
        if (CheckFormatChange(index, width, height, bitDepth) == AVCS_ERR_OK) {
            CHECK_AND_RETURN_RET_LOG(CheckStateRunning(), -1, "Not in running state");
            frameBuffer = buffers_[INDEX_OUTPUT][index];
            status = FillFrameBuffer(frameBuffer);
        } else {
            CHECK_AND_RETURN_RET_LOG(CheckStateRunning(), -1, "Not in running state");
            callback_->OnError(AVCODEC_ERROR_EXTEND_START, AVCS_ERR_NO_MEMORY);
            state_ = State::ERROR;
            if (av1DecOutputImg_ != nullptr) {
                dav1d_picture_unref(av1DecOutputImg_);
                delete av1DecOutputImg_;
                av1DecOutputImg_ = nullptr;
            }
            return -1;
        }
        frameBuffer->avBuffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        FramePostProcess(frameBuffer, index, status);
    }
    if (av1DecOutputImg_ != nullptr) {
        dav1d_picture_unref(av1DecOutputImg_);
        delete av1DecOutputImg_;
        av1DecOutputImg_ = nullptr;
    }
    return ret;
}

void Av1Decoder::UpdateColorAspects(Dav1dSequenceHeader *seqHdr)
{
    if (colorSpaceInfo_.color_range != seqHdr->color_range || colorSpaceInfo_.pri != seqHdr->pri ||
        colorSpaceInfo_.trc != seqHdr->trc || colorSpaceInfo_.mtrx != seqHdr->mtrx) {
        colorSpaceInfo_.color_range = seqHdr->color_range;
        colorSpaceInfo_.pri = seqHdr->pri;
        colorSpaceInfo_.trc = seqHdr->trc;
        colorSpaceInfo_.mtrx = seqHdr->mtrx;
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG,
                            static_cast<int32_t>(colorSpaceInfo_.color_range));
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES,
                            static_cast<int32_t>(colorSpaceInfo_.pri));
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS,
                            static_cast<int32_t>(colorSpaceInfo_.trc));
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS,
                            static_cast<int32_t>(colorSpaceInfo_.mtrx));
        AVCODEC_LOGI("color aspects: isFullRange %{public}u, primary%{public}u, transfer %{public}u,"
            " matrix %{public}u", colorSpaceInfo_.color_range, colorSpaceInfo_.pri, colorSpaceInfo_.trc,
            colorSpaceInfo_.mtrx);
        callback_->OnOutputFormatChanged(format_);
    }
}

int32_t Av1Decoder::ConvertHdrStaticMetadata(Dav1dContentLightLevel *contentLight,
                                             Dav1dMasteringDisplay *masteringDisplay,
                                             std::vector<uint8_t> &staticMetadataVec)
{
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    CHECK_AND_RETURN_RET_LOG(staticMetadataVec.size() == sizeof(HdrStaticMetadata), AVCS_ERR_INVALID_VAL,
        "wrong staticMetadataVec.size : %{public}d", static_cast<int32_t>(staticMetadataVec.size()));
    HdrStaticMetadata* hdrStaticMetadata = reinterpret_cast<HdrStaticMetadata*>(staticMetadataVec.data());
    CHECK_AND_RETURN_RET_LOG(hdrStaticMetadata != nullptr, AVCS_ERR_INVALID_VAL,
                             "vector convert to CM_HDR_Metadata_Type error");
    hdrStaticMetadata->smpte2086.displayPrimaryGreen.x = masteringDisplay->primaries[0][0] * PRIMARY_SCALE; // 0:G
    hdrStaticMetadata->smpte2086.displayPrimaryBlue.x  = masteringDisplay->primaries[1][0] * PRIMARY_SCALE; // 1:B
    hdrStaticMetadata->smpte2086.displayPrimaryRed.x   = masteringDisplay->primaries[2][0] * PRIMARY_SCALE; // 2:R
    hdrStaticMetadata->smpte2086.displayPrimaryGreen.y = masteringDisplay->primaries[0][1] * PRIMARY_SCALE; // 0:G
    hdrStaticMetadata->smpte2086.displayPrimaryBlue.y  = masteringDisplay->primaries[1][1] * PRIMARY_SCALE; // 1:B
    hdrStaticMetadata->smpte2086.displayPrimaryRed.y   = masteringDisplay->primaries[2][1] * PRIMARY_SCALE; // 2:R

    hdrStaticMetadata->smpte2086.whitePoint.x = masteringDisplay->white_point[0] * PRIMARY_SCALE;
    hdrStaticMetadata->smpte2086.whitePoint.y = masteringDisplay->white_point[1] * PRIMARY_SCALE;

    hdrStaticMetadata->smpte2086.maxLuminance = masteringDisplay->max_luminance * LUMI_SCALE;
    hdrStaticMetadata->smpte2086.minLuminance = masteringDisplay->min_luminance * LUMI_SCALE;
    if (hdrStaticMetadata->smpte2086.maxLuminance < hdrStaticMetadata->smpte2086.minLuminance) {
        std::swap(hdrStaticMetadata->smpte2086.maxLuminance, hdrStaticMetadata->smpte2086.minLuminance);
    }
    hdrStaticMetadata->cta861.maxContentLightLevel = static_cast<float>(contentLight->max_content_light_level);
    hdrStaticMetadata->cta861.maxFrameAverageLightLevel =
        static_cast<float>(contentLight->max_frame_average_light_level);
    return AVCS_ERR_OK;
}

void Av1Decoder::FillHdrInfo(sptr<SurfaceBuffer> surfaceBuffer)
{
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr && av1DecOutputImg_ != nullptr,
        "surfaceBuffer or av1DecOutputImg_ is nullptr");
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    if (av1DecOutputImg_->seq_hdr != nullptr) {
        auto seqHdr = av1DecOutputImg_->seq_hdr;
        std::vector<uint8_t> colorSpaceInfoVec;
        int32_t convertRet = ConvertParamsToColorSpaceInfo(seqHdr->color_range,
                                                           seqHdr->pri,
                                                           seqHdr->trc,
                                                           seqHdr->mtrx,
                                                           colorSpaceInfoVec);
        CHECK_AND_RETURN_LOG(convertRet == AVCS_ERR_OK, "ConvertParamsToColorSpaceInfo failed");
        GSError setRet = surfaceBuffer->SetMetadata(ATTRKEY_COLORSPACE_INFO, colorSpaceInfoVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_COLORSPACE_INFO failed");
        std::vector<uint8_t> metadataTypeVec(sizeof(CM_HDR_Metadata_Type));
        CM_HDR_Metadata_Type* metadataType = reinterpret_cast<CM_HDR_Metadata_Type*>(metadataTypeVec.data());
        CHECK_AND_RETURN_LOG(metadataType != nullptr, "vector convert to CM_HDR_Metadata_Type error");
        if (av1DecOutputImg_->content_light && av1DecOutputImg_->mastering_display) {
            std::vector<uint8_t> staticMetadataVec(sizeof(HdrStaticMetadata));
            CHECK_AND_RETURN_LOG(ConvertHdrStaticMetadata(av1DecOutputImg_->content_light,
                av1DecOutputImg_->mastering_display, staticMetadataVec) == AVCS_ERR_OK,
                "ConvertHdrStaticMetadata error");
            setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, staticMetadataVec);
            CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "setmetadata ATTRKEY_HDR_STATIC_METADATA failed");
        }
        if (av1DecOutputImg_->itut_t35 != nullptr && av1DecOutputImg_->itut_t35->payload != nullptr &&
            av1DecOutputImg_->itut_t35->payload_size > 0) {
            std::vector<uint8_t> dynamicMetadataVec(av1DecOutputImg_->itut_t35->payload,
                av1DecOutputImg_->itut_t35->payload + av1DecOutputImg_->itut_t35->payload_size);
            setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, dynamicMetadataVec);
            CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_DYNAMIC_METADATA failed");
            *metadataType = CM_VIDEO_HDR_VIVID;
        } else {
            *metadataType = static_cast<CM_HDR_Metadata_Type>(GetMetaDataTypeByTransFunc(seqHdr->trc));
        }
        setRet = surfaceBuffer->SetMetadata(ATTRKEY_HDR_METADATA_TYPE, metadataTypeVec);
        CHECK_AND_RETURN_LOG(setRet == GSERROR_OK, "SetMetadata ATTRKEY_HDR_METADATA_TYPE failed");
        UpdateColorAspects(seqHdr);
        AVCODEC_LOGD("surface buffer fill hdr info successful");
    }
}

void Av1Decoder::FlushAllFrames()
{
    std::unique_lock<std::mutex> runlock(decRunMutex_);
    int ret = 0;
    while (ret == 0) {
        Dav1dPicture pic = { 0 };
        Dav1dPicture *outputImg = &pic;
        if (dav1dCtx_ != nullptr) {
            ret = dav1d_get_picture(dav1dCtx_, outputImg);
            dav1d_picture_unref(outputImg);
        } else {
            AVCODEC_LOGE("load libdav1d.z.so failed, get generated picture form av1 decoder failed.");
            ret = -1;
        }
        outputImg = nullptr;
    }
    runlock.unlock();
}

int32_t Av1Decoder::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    for (uint32_t i = 0; i < SUPPORT_AV1_DECODER_NUM; ++i) {
        CapabilityData capsData;
        capsData.codecName = static_cast<std::string>(SUPPORT_AV1_DECODER[i].codecName);
        capsData.mimeType = static_cast<std::string>(SUPPORT_AV1_DECODER[i].mimeType);
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
        capsData.frameRate.maxVal = VIDEO_MAX_FRAMERATE;
        capsData.blockPerFrame.minVal = 1;
        capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_MAX_SIZE;
        capsData.blockPerSecond.minVal = 1;
        capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_MAX_SIZE;
        capsData.blockSize.width = VIDEO_ALIGN_SIZE;
        capsData.blockSize.height = VIDEO_ALIGN_SIZE;
        capsData.supportSwapWidthHeight = true;
        capsData.pixFormat = {
            static_cast<int32_t>(VideoPixelFormat::NV12), static_cast<int32_t>(VideoPixelFormat::NV21)
        };
        capsData.graphicPixFormat = {
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010),
            static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010)
        };
        capsData.profiles = {
            static_cast<int32_t>(AV1_PROFILE_MAIN),
            static_cast<int32_t>(AV1_PROFILE_HIGH)
        };
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(AV1Level::AV1_LEVEL_73); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AV1_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AV1_PROFILE_HIGH), levels));
        capaArray.emplace_back(capsData);
    }
    return AVCS_ERR_OK;
}

void AV1DecLog(void *cookie, const char *format, va_list ap)
{
    int32_t maxSize = 1024; // 1024 max size of one log
    std::vector<char> buf(maxSize);
    if  (buf.empty()) {
        AVCODEC_LOGE("AV1DecLog buffer is empty!");
        return;
    }
    int32_t size = vsnprintf_s(buf.data(), buf.size(), buf.size()-1, format, ap);
    if (size >= maxSize) {
        size = maxSize - 1;
    }
    auto msg = std::string(buf.data(), size);
    AVCODEC_LOGI("%{public}s", msg.c_str());
}

void AV1FreeCallback(const uint8_t *data, void *userData)
{
    (void)data;
    (void)userData;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
