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

#include "tester_common.h"
#include "hcodec_api.h"
#include "hcodec_log.h"
#include "hcodec_utils.h"
#include "native_avcodec_base.h"
#include "tester_capi.h"
#include "tester_codecbase.h"
#include "type_converter.h"
#include "v1_0/buffer_handle_meta_key_type.h"
#include "v1_0/cm_color_space.h"
#include "v1_0/hdr_static_metadata.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace Media;
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;

std::mutex TesterCommon::vividMtx_;
std::unordered_map<int64_t, std::vector<uint8_t>> TesterCommon::vividMap_;

int64_t TesterCommon::GetNowUs()
{
    auto now = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count();
}

shared_ptr<TesterCommon> TesterCommon::Create(const CommandOpt& opt)
{
    switch (opt.apiType) {
        case ApiType::TEST_CODEC_BASE:
            return make_shared<TesterCodecBase>(opt);
        case ApiType::TEST_C_API_OLD:
            return make_shared<TesterCapiOld>(opt);
        case ApiType::TEST_C_API_NEW:
            return make_shared<TesterCapiNew>(opt);
        default:
            return nullptr;
    }
}

bool TesterCommon::Run(const CommandOpt& opt)
{
    opt.Print();
    if (!opt.isEncoder && opt.decThenEnc) {
        return RunDecEnc(opt);
    }
    shared_ptr<TesterCommon> tester = Create(opt);
    CostRecorder::Instance().Clear();
    for (uint32_t i = 0; i < opt.repeatCnt; i++) {
        TLOGI("i = %u", i);
        bool ret = tester->RunOnce();
        if (!ret) {
            return false;
        }
    }
    CostRecorder::Instance().Print();
    return true;
}

bool TesterCommon::RunDecEnc(const CommandOpt& decOpt)
{
    {
        lock_guard<mutex> lk(vividMtx_);
        vividMap_.clear();
    }
    shared_ptr<TesterCommon> decoder = Create(decOpt);
    CommandOpt encOpt = decOpt;
    encOpt.isEncoder = true;
    shared_ptr<TesterCommon> encoder = Create(encOpt);

    bool ret = decoder->InitDemuxer();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->ConfigureDecoder();
    IF_TRUE_RETURN_VAL(!ret, false);

    ret = encoder->Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = encoder->SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = encoder->ConfigureEncoder();
    IF_TRUE_RETURN_VAL(!ret, false);

    sptr<Surface> surface = encoder->CreateInputSurface();
    IF_TRUE_RETURN_VAL(surface == nullptr, false);
    ret = decoder->SetOutputSurface(surface);
    IF_TRUE_RETURN_VAL(!ret, false);

    ret = encoder->Start();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = decoder->Start();
    IF_TRUE_RETURN_VAL(!ret, false);
    thread decOutThread(&TesterCommon::OutputLoop, decoder.get());
    thread encOutThread(&TesterCommon::OutputLoop, encoder.get());
    decoder->DecoderInputLoop();
    if (decOutThread.joinable()) {
        decOutThread.join();
    }
    encoder->NotifyEos();
    if (encOutThread.joinable()) {
        encOutThread.join();
    }
    decoder->Release();
    encoder->Release();
    TLOGI("RunDecEnc succ");
    return true;
}

bool TesterCommon::RunOnce()
{
    return opt_.isEncoder ? RunEncoder() : RunDecoder();
}

void TesterCommon::BeforeQueueInput(OH_AVCodecBufferAttr& attr)
{
    const char* codecStr = opt_.isEncoder ? "encoder" : "decoder";
    int64_t now = GetNowUs();
    attr.pts = now;
    if (!opt_.isEncoder && opt_.decThenEnc) {
        SaveVivid(attr.pts);
    }
    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        TLOGI("%s input:  flags=0x%x (eos)", codecStr, attr.flags);
        return;
    }
    if (attr.flags & AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        TLOGI("%s input:  flags=0x%x, pts=%" PRId64 ", size=%d", codecStr, attr.flags, attr.pts, attr.size);
        return;
    }
    TLOGI("%s input:  flags=0x%x, pts=%" PRId64 ", size=%d", codecStr, attr.flags, attr.pts, attr.size);
    if (firstInTime_ == 0) {
        firstInTime_ = now;
    }
    inTotalCnt_++;
    int64_t fromFirstInToNow = now - firstInTime_;
    if (fromFirstInToNow != 0) {
        inFps_ = inTotalCnt_ * US_TO_S / fromFirstInToNow;
    }
}

void TesterCommon::AfterGotOutput(const OH_AVCodecBufferAttr& attr)
{
    const char* codecStr = opt_.isEncoder ? "encoder" : "decoder";
    int64_t now = GetNowUs();
    if (attr.flags & AVCODEC_BUFFER_FLAG_EOS) {
        TLOGI("%s output: flags=0x%x (eos)", codecStr, attr.flags);
        return;
    }
    if (attr.flags & AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        TLOGI("%s output: flags=0x%x, pts=%" PRId64 ", size=%d", codecStr, attr.flags, attr.pts, attr.size);
        return;
    }
    if (firstOutTime_ == 0) {
        firstOutTime_ = now;
    }
    outTotalCnt_++;

    int64_t fromInToOut = now - attr.pts;
    totalCost_ += fromInToOut;
    double oneFrameCostMs = fromInToOut / US_TO_MS;
    double averageCostMs = totalCost_ / US_TO_MS / outTotalCnt_;

    int64_t fromFirstOutToNow = now - firstOutTime_;
    if (fromFirstOutToNow == 0) {
        TLOGI("%s output: flags=0x%x, pts=%" PRId64 ", size=%d, cost %.2f ms, average %.2f ms",
               codecStr, attr.flags, attr.pts, attr.size, oneFrameCostMs, averageCostMs);
    } else {
        double outFps = outTotalCnt_ * US_TO_S / fromFirstOutToNow;
        TLOGI("%s output: flags=0x%x, pts=%" PRId64 ", size=%d, cost %.2f ms, average %.2f ms, "
               "in fps %.2f, out fps %.2f",
               codecStr, attr.flags, attr.pts, attr.size, oneFrameCostMs, averageCostMs, inFps_, outFps);
    }
}

void TesterCommon::OutputLoop()
{
    while (true) {
        BufInfo buf {};
        if (!WaitForOutput(buf)) {
            return;
        }
        if (opt_.isEncoder && opt_.decThenEnc) {
            CheckVivid(buf);
        }
        ReturnOutput(buf.idx);
    }
}

void TesterCommon::CheckVivid(const BufInfo& buf)
{
    vector<uint8_t> inVividSei;
    {
        lock_guard<mutex> lk(vividMtx_);
        auto it = vividMap_.find(buf.attr.pts);
        if (it == vividMap_.end()) {
            return;
        }
        inVividSei = std::move(it->second);
        vividMap_.erase(buf.attr.pts);
    }
    shared_ptr<StartCodeDetector> demuxer = StartCodeDetector::Create(opt_.protocol);
    demuxer->SetSource(buf.va, buf.attr.size);
    optional<Sample> sample = demuxer->PeekNextSample();
    if (!sample.has_value()) {
        TLOGI("--- output pts %" PRId64 " has no sample but input has vivid", buf.attr.pts);
        return;
    }
    if (sample->vividSei.empty()) {
        TLOGI("--- output pts %" PRId64 " has no vivid but input has vivid", buf.attr.pts);
        return;
    }
    bool eq = std::equal(inVividSei.begin(), inVividSei.end(), sample->vividSei.begin());
    if (eq) {
        TLOGI("--- output pts %" PRId64 " has vivid and is same as input vivid", buf.attr.pts);
    } else {
        TLOGI("--- output pts %" PRId64 " has vivid but is different from input vivid", buf.attr.pts);
    }
}

static std::optional<GraphicPixelFormat> InnerFmtToDisplayFmt(VideoPixelFormat format, bool is10Bit)
{
    struct Fmt {
        VideoPixelFormat videoFmt;
        bool is10Bit;
        GraphicPixelFormat graFmt;
    };
    vector<Fmt> TABLE = {
        {VideoPixelFormat::YUVI420,     false,  GRAPHIC_PIXEL_FMT_YCBCR_420_P},
        {VideoPixelFormat::NV12,        false,  GRAPHIC_PIXEL_FMT_YCBCR_420_SP},
        {VideoPixelFormat::NV12,        true,   GRAPHIC_PIXEL_FMT_YCBCR_P010},
        {VideoPixelFormat::NV21,        false,  GRAPHIC_PIXEL_FMT_YCRCB_420_SP},
        {VideoPixelFormat::NV21,        true,   GRAPHIC_PIXEL_FMT_YCRCB_P010},
        {VideoPixelFormat::RGBA,        false,  GRAPHIC_PIXEL_FMT_RGBA_8888},
        {VideoPixelFormat::RGBA,        true,   GRAPHIC_PIXEL_FMT_RGBA_1010102},
        {VideoPixelFormat::RGBA1010102, false,  GRAPHIC_PIXEL_FMT_RGBA_1010102},
        {VideoPixelFormat::RGBA1010102, true,   GRAPHIC_PIXEL_FMT_RGBA_1010102},
    };
    auto it = find_if(TABLE.begin(), TABLE.end(), [format, is10Bit](const Fmt& p) {
        return p.videoFmt == format && p.is10Bit == is10Bit;
    });
    if (it != TABLE.end()) {
        return it->graFmt;
    }
    return nullopt;
}

bool TesterCommon::RunEncoder()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs_, false, "Failed to open file %s", opt_.inputFile.c_str());
    if (!opt_.isBufferMode) {
        is10Bit_ = (opt_.protocol == H265) && (opt_.profile == HEVC_PROFILE_MAIN_10);
        optional<GraphicPixelFormat> displayFmt = InnerFmtToDisplayFmt(opt_.pixFmt, is10Bit_);
        IF_TRUE_RETURN_VAL_WITH_MSG(!displayFmt, false, "invalid pixel format");
        displayFmt_ = displayFmt.value();
    }
    w_ = opt_.dispW;
    h_ = opt_.dispH;

    bool ret = Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = ConfigureEncoder();
    IF_TRUE_RETURN_VAL(!ret, false);
    sptr<Surface> surface;
    if (!opt_.isBufferMode) {
        producerSurface_ = CreateInputSurface();
        IF_TRUE_RETURN_VAL(producerSurface_ == nullptr, false);
    }
    ret = Start();
    IF_TRUE_RETURN_VAL(!ret, false);
    GetInputFormat();
    GetOutputFormat();

    thread th(&TesterCommon::OutputLoop, this);
    EncoderInputLoop();
    if (th.joinable()) {
        th.join();
    }
    ret = Stop();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Release();
    IF_TRUE_RETURN_VAL(!ret, false);
    return true;
}

bool TesterCommon::UpdateMemberFromResourceParam(const ResourceParams& param)
{
    ifstream ifs = ifstream(param.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs, false, "Failed to open file %s", param.inputFile.c_str());
    optional<GraphicPixelFormat> displayFmt = InnerFmtToDisplayFmt(param.pixFmt, is10Bit_);
    IF_TRUE_RETURN_VAL_WITH_MSG(!displayFmt, false, "invalid pixel format");
    ifs_ = std::move(ifs);
    displayFmt_ = displayFmt.value();
    w_ = param.dispW;
    h_ = param.dispH;
    return true;
}

void TesterCommon::EncoderInputLoop()
{
    while (true) {
        if (!opt_.isBufferMode && !opt_.resourceParamsMap.empty() &&
            opt_.resourceParamsMap.begin()->first == currInputCnt_) {
            UpdateMemberFromResourceParam(opt_.resourceParamsMap.begin()->second);
            opt_.resourceParamsMap.erase(opt_.resourceParamsMap.begin());
        }
        BufInfo buf {};
        bool ret = opt_.isBufferMode ? WaitForInput(buf) : WaitForInputSurfaceBuffer(buf);
        if (!ret) {
            continue;
        }
        buf.attr.size = ReadOneFrame(buf);
        if (buf.attr.size == 0) {
            buf.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            BeforeQueueInput(buf.attr);
            ret = opt_.isBufferMode ? ReturnInput(buf) : NotifyEos();
            if (ret) {
                TLOGI("queue eos succ, quit loop");
                return;
            } else {
                TLOGW("queue eos failed");
                continue;
            }
        }
        if (!opt_.setParameterParamsMap.empty() && opt_.setParameterParamsMap.begin()->first == currInputCnt_) {
            const SetParameterParams &param = opt_.setParameterParamsMap.begin()->second;
            if (param.requestIdr.has_value()) {
                RequestIDR();
            }
            SetEncoderParameter(param);
            opt_.setParameterParamsMap.erase(opt_.setParameterParamsMap.begin());
        }
        if (opt_.isBufferMode && !opt_.perFrameParamsMap.empty() &&
            opt_.perFrameParamsMap.begin()->first == currInputCnt_) {
            SetEncoderPerFrameParam(buf, opt_.perFrameParamsMap.begin()->second);
            opt_.perFrameParamsMap.erase(opt_.perFrameParamsMap.begin());
        }
        if (!opt_.isBufferMode && opt_.repeatAfter.has_value()) {
            this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000)); // 1000 ms
        }
        BeforeQueueInput(buf.attr);
        ret = opt_.isBufferMode ? ReturnInput(buf) : ReturnInputSurfaceBuffer(buf);
        if (!ret) {
            continue;
        }
        if (opt_.enableInputCb) {
            WaitForInput(buf);
            if (!opt_.perFrameParamsMap.empty() && opt_.perFrameParamsMap.begin()->first == currInputCnt_) {
                SetEncoderPerFrameParam(buf, opt_.perFrameParamsMap.begin()->second);
                opt_.perFrameParamsMap.erase(opt_.perFrameParamsMap.begin());
            }
            ReturnInput(buf);
        }
        currInputCnt_++;
    }
}

std::shared_ptr<AVBuffer> TesterCommon::CreateWaterMarkBuffer()
{
    ifstream ifs = ifstream(opt_.waterMark.waterMarkFile.inputFile, ios::binary);
    if (!ifs) {
        TLOGE("Failed to open file %s", opt_.waterMark.waterMarkFile.inputFile.c_str());
        return nullptr;
    }
    BufferRequestConfig cfg = {opt_.waterMark.waterMarkFile.dispW, opt_.waterMark.waterMarkFile.dispH,
                               32, GRAPHIC_PIXEL_FMT_RGBA_8888,
                               BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA, 0, };
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSurfaceAllocator(cfg);
    if (avAllocator == nullptr) {
        TLOGE("CreateSurfaceAllocator failed");
        return nullptr;
    }
    std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(avAllocator, 0);
    if (avBuffer == nullptr) {
        TLOGE("CreateAVBuffer failed");
        return nullptr;
    }
    sptr<SurfaceBuffer> surfaceBuffer = avBuffer->memory_->GetSurfaceBuffer();
    if (surfaceBuffer == nullptr) {
        TLOGE("Create SurfaceBuffer failed");
        return nullptr;
    }
    BufInfo buf;
    SurfaceBufferToBufferInfo(buf, surfaceBuffer);
    ReadOneFrameRGBA(ifs, buf);
    avBuffer->meta_->SetData(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_WATERMARK, true);
    avBuffer->meta_->SetData(OHOS::Media::Tag::VIDEO_COORDINATE_X, opt_.waterMark.dstX);
    avBuffer->meta_->SetData(OHOS::Media::Tag::VIDEO_COORDINATE_Y, opt_.waterMark.dstY);
    avBuffer->meta_->SetData(OHOS::Media::Tag::VIDEO_COORDINATE_W, opt_.waterMark.dstW);
    avBuffer->meta_->SetData(OHOS::Media::Tag::VIDEO_COORDINATE_H, opt_.waterMark.dstH);
    return avBuffer;
}

bool TesterCommon::SurfaceBufferToBufferInfo(BufInfo& buf, sptr<SurfaceBuffer> surfaceBuffer)
{
    if (surfaceBuffer == nullptr) {
        TLOGE("null surfaceBuffer");
        return false;
    }
    buf.va = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr());
    buf.capacity = surfaceBuffer->GetSize();

    buf.dispW = surfaceBuffer->GetWidth();
    buf.dispH = surfaceBuffer->GetHeight();
    buf.fmt = static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat());
    buf.byteStride = surfaceBuffer->GetStride();
    return true;
}

bool TesterCommon::NativeBufferToBufferInfo(BufInfo& buf, OH_NativeBuffer* nativeBuffer)
{
    if (nativeBuffer == nullptr) {
        TLOGE("null OH_NativeBuffer");
        return false;
    }
    OH_NativeBuffer_Config cfg;
    OH_NativeBuffer_GetConfig(nativeBuffer, &cfg);
    buf.dispW = cfg.width;
    buf.dispH = cfg.height;
    buf.fmt = static_cast<GraphicPixelFormat>(cfg.format);
    buf.byteStride = cfg.stride;
    return true;
}

bool TesterCommon::WaitForInputSurfaceBuffer(BufInfo& buf)
{
    BufferRequestConfig cfg = {w_, h_, 32, displayFmt_,
                               BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA, 0, };
    sptr<SurfaceBuffer> surfaceBuffer;
    int32_t fence;
    GSError err = producerSurface_->RequestBuffer(surfaceBuffer, fence, cfg);
    if (err != GSERROR_OK) {
        return false;
    }
    buf.surfaceBuf = surfaceBuffer;
    return SurfaceBufferToBufferInfo(buf, surfaceBuffer);
}

void TesterCommon::SetStaticMetaData(BufInfo& buf)
{
    using namespace OHOS::HDI::Display::Graphic::Common::V1_0;
    vector<uint8_t> vec(sizeof(HdrStaticMetadata));
    HdrStaticMetadata* info = (HdrStaticMetadata*)vec.data();
    float value1 = 0.00002;
    float value2 = 0.0001;
    float lightLevel = 1000.0;
    info->smpte2086.displayPrimaryGreen.x = 8500 * value1; // 8500
    info->smpte2086.displayPrimaryGreen.y = 39850 * value1; // 39850
    info->smpte2086.displayPrimaryBlue.x = 6550 * value1; //6550
    info->smpte2086.displayPrimaryBlue.y = 2300 * value1; // 2300
    info->smpte2086.displayPrimaryRed.x = 35400 * value1; // 35400
    info->smpte2086.displayPrimaryRed.y = 14600 * value1; // 14600
    info->smpte2086.whitePoint.x = 15635 * value1; // 15635
    info->smpte2086.whitePoint.y = 16450 * value1; // 16450
    info->smpte2086.maxLuminance = 10000000 * value2; // 10000000
    info->smpte2086.minLuminance = 50 * value2;       // 50
    info->cta861.maxContentLightLevel = lightLevel;
    info->cta861.maxFrameAverageLightLevel = lightLevel;
    buf.surfaceBuf->SetMetadata(ATTRKEY_HDR_STATIC_METADATA, vec);
}
 
void TesterCommon::SetDynamicMetaData(BufInfo& buf)
{
    std::vector<uint8_t> HDR_VIVID_INPUT1 = {1, 47, 87, 30, 138, 187, 117, 240, 26, 190, 187, 29, 128, 0, 82,
                                    142, 25, 152, 6, 100, 171, 42, 207, 235, 248, 67, 176, 235, 252, 16, 93,
                                    242, 106, 132, 0, 10, 81, 199, 178, 80, 255, 217, 150, 101, 253, 149, 53,
                                    212, 169, 127, 160, 0, 0};
    buf.surfaceBuf->SetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, HDR_VIVID_INPUT1);
}

bool TesterCommon::ReturnInputSurfaceBuffer(BufInfo& buf)
{
    BufferFlushConfig flushConfig = {
        .damage = {
            .w = w_,
            .h = h_,
        },
        .timestamp = buf.attr.pts,
    };
    if (is10Bit_) {
        vector<uint8_t> vec(sizeof(CM_ColorSpaceInfo));
        CM_ColorSpaceInfo* info = (CM_ColorSpaceInfo*)vec.data();
        info->primaries = COLORPRIMARIES_BT2020;
        info->transfunc = TRANSFUNC_HLG;
        info->matrix = MATRIX_BT2020;
        info->range = RANGE_LIMITED;
        buf.surfaceBuf->SetMetadata(ATTRKEY_COLORSPACE_INFO, vec);
        SetStaticMetaData(buf);
        SetDynamicMetaData(buf);
    }
    GSError err = producerSurface_->FlushBuffer(buf.surfaceBuf, -1, flushConfig);
    if (err != GSERROR_OK) {
        TLOGE("FlushBuffer failed");
        return false;
    }
    return true;
}

#define RETURN_ZERO_IF_EOS(expectedSize) \
    do { \
        if (src.gcount() != (expectedSize)) { \
            TLOGI("no more data"); \
            return 0; \
        } \
    } while (0)

uint32_t TesterCommon::ReadOneFrameYUV420P(std::ifstream& src, ImgBuf& dstImg)
{
    char* dst = reinterpret_cast<char*>(dstImg.va);
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < dstImg.dispH; i++) {
        src.read(dst, dstImg.dispW);
        RETURN_ZERO_IF_EOS(dstImg.dispW);
        dst += dstImg.byteStride;
    }
    // copy U
    for (uint32_t i = 0; i < dstImg.dispH / SAMPLE_RATIO; i++) {
        src.read(dst, dstImg.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(dstImg.dispW / SAMPLE_RATIO);
        dst += dstImg.byteStride / SAMPLE_RATIO;
    }
    // copy V
    for (uint32_t i = 0; i < dstImg.dispH / SAMPLE_RATIO; i++) {
        src.read(dst, dstImg.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(dstImg.dispW / SAMPLE_RATIO);
        dst += dstImg.byteStride / SAMPLE_RATIO;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrameYUV420SP(std::ifstream& src, ImgBuf& dstImg, uint8_t bytesPerPixel)
{
    char* dst = reinterpret_cast<char*>(dstImg.va);
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < dstImg.dispH; i++) {
        src.read(dst, dstImg.dispW * bytesPerPixel);
        RETURN_ZERO_IF_EOS(dstImg.dispW * bytesPerPixel);
        dst += dstImg.byteStride;
    }
    // copy UV
    for (uint32_t i = 0; i < dstImg.dispH / SAMPLE_RATIO; i++) {
        src.read(dst, dstImg.dispW * bytesPerPixel);
        RETURN_ZERO_IF_EOS(dstImg.dispW * bytesPerPixel);
        dst += dstImg.byteStride;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrameRGBA(std::ifstream& src, ImgBuf& dstImg)
{
    char* dst = reinterpret_cast<char*>(dstImg.va);
    char* start = dst;
    for (uint32_t i = 0; i < dstImg.dispH; i++) {
        src.read(dst, dstImg.dispW * BYTES_PER_PIXEL_RBGA);
        RETURN_ZERO_IF_EOS(dstImg.dispW * BYTES_PER_PIXEL_RBGA);
        dst += dstImg.byteStride;
    }
    return dst - start;
}

uint32_t TesterCommon::ReadOneFrame(ImgBuf& dstImg)
{
    if (dstImg.va == nullptr) {
        TLOGE("dst image has null va");
        return 0;
    }
    if (dstImg.byteStride < dstImg.dispW) {
        TLOGE("byteStride %u < dispW %u", dstImg.byteStride, dstImg.dispW);
        return 0;
    }
    uint32_t sampleSize = 0;
    switch (dstImg.fmt) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010: {
            sampleSize = GetYuv420Size(dstImg.byteStride, dstImg.dispH);
            break;
        }
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            sampleSize = dstImg.byteStride * dstImg.dispH;
            break;
        }
        default:
            return 0;
    }
    if (sampleSize > dstImg.capacity) {
        TLOGE("sampleSize %u > dst capacity %zu", sampleSize, dstImg.capacity);
        return 0;
    }
    if (opt_.mockFrameCnt.has_value() && currInputCnt_ > opt_.mockFrameCnt.value()) {
        return 0;
    }
    if ((opt_.maxReadFrameCnt > 0 && currInputCnt_ >= opt_.maxReadFrameCnt)) {
        return sampleSize;
    }
    switch (dstImg.fmt) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P: {
            return ReadOneFrameYUV420P(ifs_, dstImg);
        }
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP: {
            return ReadOneFrameYUV420SP(ifs_, dstImg, 1);
        }
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010: {
            return ReadOneFrameYUV420SP(ifs_, dstImg, 2); // bytesPerPixel=2
        }
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            return ReadOneFrameRGBA(ifs_, dstImg);
        }
        default:
            return 0;
    }
}

bool TesterCommon::InitDemuxer()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    IF_TRUE_RETURN_VAL_WITH_MSG(!ifs_, false, "Failed to open file %s", opt_.inputFile.c_str());
    demuxer_ = StartCodeDetector::Create(opt_.protocol);
    totalSampleCnt_ = demuxer_->SetSource(opt_.inputFile);
    if (totalSampleCnt_ == 0) {
        TLOGE("no nalu found");
        return false;
    }
    return true;
}

bool TesterCommon::RunDecoder()
{
    bool ret = InitDemuxer();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Create();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = SetCallback();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = ConfigureDecoder();
    IF_TRUE_RETURN_VAL(!ret, false);
    if (!opt_.isBufferMode) {
        sptr<Surface> surface = CreateSurfaceNormal();
        if (surface == nullptr) {
            return false;
        }
        ret = SetOutputSurface(surface);
        IF_TRUE_RETURN_VAL(!ret, false);
    }
    GetInputFormat();
    GetOutputFormat();
    ret = Start();
    IF_TRUE_RETURN_VAL(!ret, false);

    thread outputThread(&TesterCommon::OutputLoop, this);
    DecoderInputLoop();
    if (outputThread.joinable()) {
        outputThread.join();
    }

    ret = Stop();
    IF_TRUE_RETURN_VAL(!ret, false);
    ret = Release();
    IF_TRUE_RETURN_VAL(!ret, false);
    return true;
}

sptr<Surface> TesterCommon::CreateSurfaceNormal()
{
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    if (consumerSurface == nullptr) {
        TLOGE("CreateSurfaceAsConsumer failed");
        return nullptr;
    }
    sptr<IBufferConsumerListener> listener = new Listener(this);
    GSError err = consumerSurface->RegisterConsumerListener(listener);
    if (err != GSERROR_OK) {
        TLOGE("RegisterConsumerListener failed");
        return nullptr;
    }
    err = consumerSurface->SetDefaultUsage(BUFFER_USAGE_CPU_READ);
    if (err != GSERROR_OK) {
        TLOGE("SetDefaultUsage failed");
        return nullptr;
    }
    sptr<IBufferProducer> bufferProducer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(bufferProducer);
    if (producerSurface == nullptr) {
        TLOGE("CreateSurfaceAsProducer failed");
        return nullptr;
    }
    surface_ = consumerSurface;
    return producerSurface;
}

void TesterCommon::Listener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t fence;
    int64_t timestamp;
    OHOS::Rect damage;
    GSError err = tester_->surface_->AcquireBuffer(buffer, fence, timestamp, damage);
    if (err != GSERROR_OK || buffer == nullptr) {
        TLOGW("AcquireBuffer failed");
        return;
    }
    tester_->surface_->ReleaseBuffer(buffer, -1);
}

void TesterCommon::DecoderInputLoop()
{
    PrepareSeek();
    while (true) {
        if (!SeekIfNecessary()) {
            return;
        }
        BufInfo buf {};
        if (!WaitForInput(buf)) {
            continue;
        }

        size_t sampleIdx;
        bool isCsd;
        buf.attr.size = GetNextSample(buf, sampleIdx, isCsd);
        if (isCsd) {
            buf.attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        }
        if (buf.attr.size == 0 || (opt_.maxReadFrameCnt > 0 && currInputCnt_ > opt_.maxReadFrameCnt)) {
            buf.attr.size = 0;
            buf.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            BeforeQueueInput(buf.attr);
            if (ReturnInput(buf)) {
                TLOGI("queue eos succ, quit loop");
                return;
            } else {
                TLOGW("queue eos failed");
                continue;
            }
        }
        BeforeQueueInput(buf.attr);
        if (!ReturnInput(buf)) {
            TLOGW("queue sample %zu failed", sampleIdx);
            continue;
        }
        currInputCnt_++;
        currSampleIdx_ = sampleIdx;
        demuxer_->MoveToNext();
    }
}

void TesterCommon::SaveVivid(int64_t pts)
{
    optional<Sample> sample = demuxer_->PeekNextSample();
    if (!sample.has_value()) {
        return;
    }
    if (sample->vividSei.empty()) {
        return;
    }
    lock_guard<mutex> lk(vividMtx_);
    vividMap_[pts] = sample->vividSei;
}

static size_t GenerateRandomNumInRange(size_t rangeStart, size_t rangeEnd)
{
    return rangeStart + rand() % (rangeEnd - rangeStart);
}

void TesterCommon::PrepareSeek()
{
    int mockCnt = 0;
    size_t lastSeekTo = 0;
    while (mockCnt++ < opt_.flushCnt) {
        size_t seekFrom = GenerateRandomNumInRange(lastSeekTo, totalSampleCnt_);
        size_t seekTo = GenerateRandomNumInRange(0, totalSampleCnt_);
        TLOGI("mock seek from sample index %zu to %zu", seekFrom, seekTo);
        userSeekPos_.emplace_back(seekFrom, seekTo);
        lastSeekTo = seekTo;
    }
}

bool TesterCommon::SeekIfNecessary()
{
    if (userSeekPos_.empty()) {
        return true;
    }
    size_t seekFrom;
    size_t seekTo;
    std::tie(seekFrom, seekTo) = userSeekPos_.front();
    if (currSampleIdx_ != seekFrom) {
        return true;
    }
    TLOGI("begin to seek from sample index %zu to %zu", seekFrom, seekTo);
    if (!demuxer_->SeekTo(seekTo)) {
        return true;
    }
    if (!Flush()) {
        return false;
    }
    if (!Start()) {
        return false;
    }
    userSeekPos_.pop_front();
    return true;
}

int TesterCommon::GetNextSample(const Span& dstSpan, size_t& sampleIdx, bool& isCsd)
{
    optional<Sample> sample = demuxer_->PeekNextSample();
    if (!sample.has_value()) {
        return 0;
    }
    uint32_t sampleSize = sample->endPos - sample->startPos;
    if (sampleSize > dstSpan.capacity) {
        TLOGE("sampleSize %u > dst capacity %zu", sampleSize, dstSpan.capacity);
        return 0;
    }
    TLOGI("sample %zu: size = %u, isCsd = %d, isIDR = %d, %s",
          sample->idx, sampleSize, sample->isCsd, sample->isIdr, sample->s.c_str());
    sampleIdx = sample->idx;
    isCsd = sample->isCsd;
    ifs_.seekg(sample->startPos);
    ifs_.read(reinterpret_cast<char*>(dstSpan.va), sampleSize);
    return sampleSize;
}

std::string TesterCommon::GetCodecMime(const CodeType& type)
{
    switch (type) {
        case H264:
            return "video/avc";
        case H265:
            return "video/hevc";
        case H266:
            return "video/vvc";
        default:
            return "";
    }
}
} // namespace OHOS::MediaAVCodec