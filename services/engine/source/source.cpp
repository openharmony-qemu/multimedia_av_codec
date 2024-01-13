/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "source.h"
#include <iostream>
#include <dlfcn.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"
#include "media_description.h"
#include "avcodec_log.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "media_description.h"
#include "media_source.h"
#include "ffmpeg_converter.h"
#include "ffmpeg_format_helper.h"
#include "meta/format.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avstring.h"
#ifdef __cplusplus
}
#endif

static std::string g_libFileHead = "libhistreamer_plugin_";
static std::string g_fileSeparator = "/";
static std::string g_libFileTail = ".z.so";

#define CUVA_VERSION_MAP (static_cast<uint16_t>(1))
#define TERMINAL_PROVIDE_CODE (static_cast<uint16_t>(4))
#define TERMINAL_PROVIDE_ORIENTED_CODE (static_cast<uint16_t>(5))

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "Source"};

    inline bool FileIsExists(const char* name)
    {
        struct stat buffer;
        return (stat(name, &buffer) == 0);
    }

    static std::map<std::string, std::string> g_pluginMap = {
        {"http", "libhistreamer_plugin_HttpSource.z.so"},
        {"https", "libhistreamer_plugin_HttpSource.z.so"},
        {"fd", "libhistreamer_plugin_FileFdSource.z.so"}
    };

    static std::vector<AVCodecID> g_imageCodecID = {
        AV_CODEC_ID_MJPEG,
        AV_CODEC_ID_PNG,
        AV_CODEC_ID_PAM,
        AV_CODEC_ID_BMP,
        AV_CODEC_ID_JPEG2000,
        AV_CODEC_ID_TARGA,
        AV_CODEC_ID_TIFF,
        AV_CODEC_ID_GIF,
        AV_CODEC_ID_PCX,
        AV_CODEC_ID_XWD,
        AV_CODEC_ID_XBM,
        AV_CODEC_ID_WEBP,
        AV_CODEC_ID_APNG,
        AV_CODEC_ID_XPM,
        AV_CODEC_ID_SVG,
    };

    int32_t ParseProtocol(const std::string& uri, std::string& protocol)
    {
        AVCODEC_LOGD("ParseProtocol, input: uri=%{private}s, protocol=%{public}s", uri.c_str(), protocol.c_str());
        int32_t ret;
        auto const pos = uri.find("://");
        if (pos != std::string::npos) {
            auto prefix = uri.substr(0, pos);
            protocol.append(prefix);
            ret = AVCS_ERR_OK;
        }
        
        if (protocol.empty()) {
            AVCODEC_LOGE("ERROR:Invalid protocol: %{public}s", protocol.c_str());
            ret = AVCS_ERR_INVALID_OPERATION;
        }
        return ret;
    }

    RegisterFunc OpenFilePlugin(const std::string& path, const std::string& name, void** handler)
    {
        AVCODEC_LOGD("OpenFilePlugin, input: path=%{private}s, name=%{private}s", path.c_str(), name.c_str());
        if (FileIsExists(path.c_str())) {
            *handler = ::dlopen(path.c_str(), RTLD_NOW);
            if (*handler == nullptr) {
                AVCODEC_LOGE("dlopen failed due to %{private}s", ::dlerror());
            }
        }
        if (*handler) {
            std::string registerFuncName = "register_" + name;
            RegisterFunc registerFunc = nullptr;
            registerFunc = (RegisterFunc)(::dlsym(*handler, registerFuncName.c_str()));
            if (registerFunc) {
                return registerFunc;
            } else {
                AVCODEC_LOGE("register is not found in %{public}s", registerFuncName.c_str());
            }
        } else {
            AVCODEC_LOGE("dlopen failed: %{private}s", path.c_str());
        }
        return {};
    }

    bool StartWith(const char* name, const char* chars)
    {
        return !strncmp(name, chars, strlen(chars));
    }

    bool IsInputFormatSupported(const char* name)
    {
        if (!strcmp(name, "audio_device") || StartWith(name, "image") ||
            !strcmp(name, "mjpeg") || !strcmp(name, "redir") || StartWith(name, "u8") ||
            StartWith(name, "u16") || StartWith(name, "u24") ||
            StartWith(name, "u32") ||
            StartWith(name, "s8") || StartWith(name, "s16") ||
            StartWith(name, "s24") ||
            StartWith(name, "s32") || StartWith(name, "f32") ||
            StartWith(name, "f64") ||
            !strcmp(name, "mulaw") || !strcmp(name, "alaw")) {
            return false;
        }
        if (!strcmp(name, "sdp") || !strcmp(name, "rtsp") || !strcmp(name, "applehttp")) {
            return false;
        }
        return true;
    }
}

Status SourceRegister::AddPlugin(const PluginDefBase& def)
{
    auto& tmpDef = (SourcePluginDef&) def;
    sourcePlugin = (tmpDef.creator)(tmpDef.name);
    return Status::OK;
}

Status SourceRegister::AddPackage(const PackageDef& def)
{
    packageDef = std::make_shared<PackageDef>(def);
    return Status::OK;
}
constexpr size_t DEFAULT_READ_SIZE = 4096;

Source::Source()
    :formatContext_(nullptr), inputFormat_(nullptr)
{
    AVCODEC_LOGI("Source::Source is on call");
    av_log_set_level(AV_LOG_ERROR);
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
    hevcParser_ = HevcParserManager::Create();
}

Source::~Source()
{
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
    formatContext_ = nullptr;
    inputFormat_ = nullptr;
    if (sourcePlugin_ != nullptr) {
        sourcePlugin_->Stop();
        sourcePlugin_ = nullptr;
    }
    register_ = nullptr;
    avioContext_ = nullptr;
    handler_ = nullptr;
    hevcParser_ = nullptr;
    if (firstFrame_ != nullptr) {
        av_packet_free(&firstFrame_);
        av_free(firstFrame_);
        firstFrame_ = nullptr;
    }
    AVCODEC_LOGI("Source::~Source is on call");
}

int32_t Source::GetTrackCount(uint32_t &trackCount)
{
    CHECK_AND_RETURN_RET_LOG(formatContext_ != nullptr, AVCS_ERR_INVALID_OPERATION,
        "call GetTrackCount failed, because create source failed!");
    trackCount = static_cast<uint32_t>(formatContext_->nb_streams);
    return AVCS_ERR_OK;
}

int32_t Source::GetSourceFormat(Format &format)
{
    AVCODEC_LOGI("Source::GetSourceFormat is on call");
    CHECK_AND_RETURN_RET_LOG(formatContext_ != nullptr, AVCS_ERR_INVALID_OPERATION, "formatContext_ is nullptr!");
    
    FFmpegFormatHelper::ParseMediaInfo(*formatContext_, format);
    AVCODEC_LOGI("Source::GetSourceFormat result: %{public}s", format.Stringify().c_str());
    return AVCS_ERR_OK;
}

int32_t Source::GetTrackFormat(Format &format, uint32_t trackIndex)
{
    AVCODEC_LOGI("Source::GetTrackFormat is on call: trackIndex=%{public}u", trackIndex);
    CHECK_AND_RETURN_RET_LOG(formatContext_ != nullptr, AVCS_ERR_INVALID_OPERATION, "formatContext_ is nullptr!");
    CHECK_AND_RETURN_RET_LOG(trackIndex < static_cast<uint32_t>(formatContext_->nb_streams),
                             AVCS_ERR_INVALID_VAL, "trackIndex is invalid!");

    auto avStream = formatContext_->streams[trackIndex];
    CHECK_AND_RETURN_RET_LOG(avStream != nullptr, AVCS_ERR_INVALID_OPERATION, "stream is nullptr!");

    FFmpegFormatHelper::ParseTrackInfo(*avStream, format);
    if (avStream->codecpar->codec_id == AV_CODEC_ID_HEVC) {
        if (hevcParser_ != nullptr && firstFrame_ != nullptr) {
            hevcParser_->ConvertExtraDataToAnnexb(avStream->codecpar->extradata, avStream->codecpar->extradata_size);
            hevcParser_->ConvertPacketToAnnexb(&(firstFrame_->data), firstFrame_->size);
            hevcParser_->ParseAnnexbExtraData(firstFrame_->data, firstFrame_->size);
            ParseHEVCMetadataInfo(*avStream, format);
        } else {
            AVCODEC_LOGW("hevcParser_ is nullptr, parser hevc fail");
        }
    }
    AVCODEC_LOGD("Source::GetTrackFormat result: %{public}s", format.Stringify().c_str());
    return AVCS_ERR_OK;
}

bool Source::IsAVTrack(const AVStream& avStream)
{
    if (avStream.codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        return true;
    } else if (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if ((avStream.disposition & AV_DISPOSITION_ATTACHED_PIC) ||
            (std::count(g_imageCodecID.begin(), g_imageCodecID.end(), avStream.codecpar->codec_id) > 0)) {
                return false;
        }
        return true;
    }
    return false;
}

void Source::GetVideoFirstKeyFrame()
{
    if (formatContext_ == nullptr) {
        return;
    }
    int64_t startPts = 0;
    int startTrackIndex = -1;
    for (uint32_t trackIndex = 0; trackIndex < formatContext_->nb_streams; ++trackIndex) {
        auto avStream = formatContext_->streams[trackIndex];
        if (avStream->codecpar->codec_id != AV_CODEC_ID_HEVC || firstFrame_ != nullptr) {
            continue;
        }
        hasHevc_ = true;
        firstFrame_ = av_packet_alloc();
        if (firstFrame_ == nullptr) {
            return;
        }
        while (av_read_frame(formatContext_.get(), firstFrame_) >= 0) {
            auto tempStream = formatContext_->streams[firstFrame_->stream_index];
            if (startTrackIndex < 0 && IsAVTrack(*tempStream)) {
                startPts = firstFrame_->pts;
                startTrackIndex = firstFrame_->stream_index;
            }

            if (static_cast<uint32_t>(firstFrame_->stream_index) == trackIndex) {
                break;
            }
            av_packet_unref(firstFrame_);
        }

        startPts = (startPts > 0) ? 0 : startPts;
        auto rtv = av_seek_frame(formatContext_.get(), startTrackIndex, startPts, AVSEEK_FLAG_BACKWARD);
        if (rtv < 0) {
            AVCODEC_LOGW("seek failed, return value: ffmpeg error:%{public}d", rtv);
            firstFrame_ = nullptr;
        }
    }
}

void Source::ParseHEVCMetadataInfo(const AVStream& avStream, Format &format)
{
    HevcParseFormat parse;
    parse.isHdrVivid = hevcParser_->IsHdrVivid();
    parse.colorRange = hevcParser_->GetColorRange();
    parse.colorPrimaries = hevcParser_->GetColorPrimaries();
    parse.colorTransfer = hevcParser_->GetColorTransfer();
    parse.colorMatrixCoeff = hevcParser_->GetColorMatrixCoeff();
    parse.profile = hevcParser_->GetProfileIdc();
    parse.level = hevcParser_->GetLevelIdc();
    parse.chromaLocation = hevcParser_->GetChromaLocation();
    parse.picWidInLumaSamples = hevcParser_->GetPicWidInLumaSamples();
    parse.picHetInLumaSamples = hevcParser_->GetPicHetInLumaSamples();

    FFmpegFormatHelper::ParseHevcInfo(*formatContext_, parse, format);
    if (parse.isHdrVivid) {
        ParseHDRVividCUVVInfo(format);
    }
}

void Source::ParseHDRVividCUVVInfo(Format &format)
{
    CUVVConfigBox cuvvBox = {CUVA_VERSION_MAP, TERMINAL_PROVIDE_CODE, TERMINAL_PROVIDE_ORIENTED_CODE};
    bool ret = format.PutBuffer(MediaDescriptionKey::MD_KEY_VIDEO_CUVV_CONFIG_BOX,
        reinterpret_cast<uint8_t*>(&cuvvBox), sizeof(cuvvBox));
    if (!ret) {
        AVCODEC_LOGW("Put hdr vivid cuvv info failed");
    }
}

int32_t Source::Init(std::string& uri)
{
    AVCODEC_LOGI("Source::Init is called");
    int32_t ret = LoadDynamicPlugin(uri);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED,
                             "init source failed when load source plugin!");
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    AVCODEC_LOGD("mediaSource Init: %{private}s", mediaSource->GetSourceUri().c_str());
    if (sourcePlugin_ == nullptr) {
        AVCODEC_LOGE("load sourcePlugin_ fail !");
        return AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED;
    }
    Status pluginRet = sourcePlugin_->SetSource(mediaSource);

    CHECK_AND_RETURN_RET_LOG(pluginRet == Status::OK, AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED,
                             "init source failed when set data source for plugin!");
    ret = SniffInputFormat();
    if (ret != AVCS_ERR_OK) {
        FaultEventWrite(FaultType::FAULT_TYPE_INNER_ERROR, "Sniff failed", "Source");
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED,
                             "init source failed when find input format!");
    CHECK_AND_RETURN_RET_LOG(inputFormat_ != nullptr, AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED,
                             "init source failed when find input format, cannnot match any input format!");
    ret = InitAVFormatContext();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED,
                             "init source failed when parse source info!");
    CHECK_AND_RETURN_RET_LOG(formatContext_ != nullptr, AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED,
                             "init source failed when init AVFormatContext!");
    return AVCS_ERR_OK;
}

int32_t Source::LoadDynamicPlugin(const std::string& path)
{
    AVCODEC_LOGI("LoadDynamicPlugin: %{private}s", path.c_str());
    std::string protocol;
    if (ParseProtocol(path, protocol) != AVCS_ERR_OK) {
        AVCODEC_LOGE("Couldn't find valid protocol for %{private}s", path.c_str());
        return AVCS_ERR_INVALID_OPERATION;
    }
    if (g_pluginMap.count(protocol) == 0) {
        AVCODEC_LOGE("Unsupport protocol: %{public}s", protocol.c_str());
        return AVCS_ERR_INVALID_OPERATION;
    }
    std::string libFileName = g_pluginMap[protocol];
    std::string filePluginPath = OH_FILE_PLUGIN_PATH + g_fileSeparator + libFileName;
    std::string pluginName =
        libFileName.substr(g_libFileHead.size(), libFileName.size() - g_libFileHead.size() - g_libFileTail.size());
    RegisterFunc registerFunc = OpenFilePlugin(filePluginPath, pluginName, &handler_);
    if (registerFunc) {
        register_ = std::make_shared<SourceRegister>();
        registerFunc(register_);
        sourcePlugin_ = register_->sourcePlugin;
        AVCODEC_LOGD("regist source plugin successful");
        return AVCS_ERR_OK;
    } else {
        AVCODEC_LOGD("regist source plugin failed, sourcePlugin path: %{private}s", filePluginPath.c_str());
        return AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED;
    }
}

int32_t Source::SniffInputFormat()
{
    size_t bufferSize = DEFAULT_READ_SIZE;
    uint64_t fileSize = 0;
    constexpr int probThresh = 50;
    constexpr size_t strMax = 4;
    int maxProb = 0;
    void* i = nullptr;
    if (sourcePlugin_->GetSize(fileSize) == Status::OK) {
        bufferSize = (static_cast<uint64_t>(bufferSize) < fileSize) ? bufferSize : fileSize;
    }
    // fix ffmpeg probe crash,refer to ffmpeg/tools/probetest.c
    std::vector<uint8_t> buff(bufferSize + AVPROBE_PADDING_SIZE);
    auto bufferInfo = std::make_shared<Buffer>();
    auto bufferMemory = bufferInfo->WrapMemory(buff.data(), bufferSize, 0);
    if (bufferMemory == nullptr) {
        return AVCS_ERR_NO_MEMORY;
    }
    auto ret = static_cast<int>(sourcePlugin_->Read(bufferInfo, bufferSize));
    CHECK_AND_RETURN_RET_LOG(ret == 0, AVCS_ERR_CREATE_SOURCE_SUB_SERVICE_FAILED,
        "create source service failed when probe source format!");
    CHECK_AND_RETURN_RET_LOG(buff.data() != nullptr, AVCS_ERR_INVALID_DATA,
        "data cannot be read when probe source format!");
    AVProbeData probeData = {"", buff.data(), static_cast<int>(bufferSize), ""};
    const AVInputFormat* inputFormat = nullptr;
    while ((inputFormat = av_demuxer_iterate(&i))) {
        if (inputFormat->long_name != nullptr && !strncmp(inputFormat->long_name, "pcm ", strMax)) {
            continue;
        }
        if (!IsInputFormatSupported(inputFormat->name)) {
            continue;
        }
        if (inputFormat->read_probe) {
            auto prob = inputFormat->read_probe(&probeData);
            if (prob > probThresh) {
                inputFormat_ = std::shared_ptr<AVInputFormat>(const_cast<AVInputFormat*>(inputFormat), [](void*) {});
                break;
            }
            if (prob > maxProb) {
                maxProb = prob;
                inputFormat_ = std::shared_ptr<AVInputFormat>(const_cast<AVInputFormat*>(inputFormat), [](void*) {});
            }
        }
    }
    if (inputFormat_ == nullptr) {
        AVCODEC_LOGE("sniff input format failed, can't find proper input format");
        return AVCS_ERR_INVALID_OPERATION;
    }
    return AVCS_ERR_OK;
}

void Source::InitAVIOContext(int flags)
{
    constexpr int bufferSize = 4096;
    customIOContext_.sourcePlugin = sourcePlugin_.get();
    Status pluginRet = sourcePlugin_->GetSize(customIOContext_.fileSize);
    if (pluginRet != Status::OK) {
        AVCODEC_LOGE("get file size failed when set data source for plugin!");
        return;
    }
    pluginRet = Status::ERROR_UNKNOWN;
    while (pluginRet == Status::ERROR_UNKNOWN) {
        pluginRet = sourcePlugin_->SeekTo(0);
        if (static_cast<int32_t>(pluginRet) < 0 && pluginRet != Status::ERROR_UNKNOWN) {
            AVCODEC_LOGE("Seek to 0 failed when set data source for plugin!");
            return;
        } else if (pluginRet == Status::ERROR_UNKNOWN) {
            AVCODEC_LOGW("Seek to 0 failed when set data source for plugin, try again");
            sleep(1);
        }
    }
    customIOContext_.offset = 0;
    customIOContext_.eof = false;
    auto buffer = static_cast<unsigned char*>(av_malloc(bufferSize));
    if (buffer == nullptr) {
        AVCODEC_LOGE("AllocAVIOContext failed to av_malloc...");
        return;
    }
    avioContext_ = avio_alloc_context(buffer, bufferSize, flags & AVIO_FLAG_WRITE,
                                    (void*)(&customIOContext_), AVReadPacket, NULL, AVSeek);
    customIOContext_.avioContext = avioContext_;
    if (avioContext_ == nullptr) {
        AVCODEC_LOGE("AllocAVIOContext failed to avio_alloc_context...");
        av_free(buffer);
        return;
    }
    Seekable seekable = sourcePlugin_->GetSeekable();
    AVCODEC_LOGD("seekable_ is %{public}d", (int)seekable);
    avioContext_->seekable = (seekable == Seekable::SEEKABLE) ? AVIO_SEEKABLE_NORMAL : 0;
    if (!(static_cast<uint32_t>(flags) & static_cast<uint32_t>(AVIO_FLAG_WRITE))) {
        avioContext_->buf_ptr = avioContext_->buf_end;
        avioContext_->write_flag = 0;
    }
}

int64_t Source::AVSeek(void *opaque, int64_t offset, int whence)
{
    auto customIOContext = static_cast<CustomIOContext*>(opaque);
    uint64_t newPos = 0;
    switch (whence) {
        case SEEK_SET:
            newPos = static_cast<uint64_t>(offset);
            customIOContext->offset = newPos;
            break;
        case SEEK_CUR:
            newPos = customIOContext->offset + offset;
            break;
        case SEEK_END:
        case AVSEEK_SIZE: {
            uint64_t mediaDataSize = 0;
            customIOContext->sourcePlugin->GetSize(mediaDataSize);
            if (mediaDataSize > 0) {
                newPos = mediaDataSize + offset;
            }
            break;
        }
        default:
            AVCODEC_LOGW("AVSeek unexpected whence: %{public}d", whence);
            break;
    }
    if (whence != AVSEEK_SIZE) {
        customIOContext->offset = newPos;
    }
    return newPos;
}

int Source::AVReadPacket(void *opaque, uint8_t *buf, int bufSize)
{
    int rtv = -1;
    auto readSize = bufSize;
    auto customIOContext = static_cast<CustomIOContext*>(opaque);
    auto buffer = std::make_shared<Buffer>();
    auto bufData = buffer->WrapMemory(buf, bufSize, 0);
    if ((customIOContext->avioContext->seekable != static_cast<int>(Seekable::SEEKABLE)) ||
        (customIOContext->fileSize == 0)) {
        return rtv;
    }
    
    if (customIOContext->offset > customIOContext->fileSize) {
        AVCODEC_LOGW("ERROR: offset: %{public}zu is larger than totalSize: %{public}" PRIu64,
                        customIOContext->offset, customIOContext->fileSize);
        return rtv;
    }
    if (static_cast<size_t>(customIOContext->offset + bufSize) > customIOContext->fileSize) {
        readSize = customIOContext->fileSize - customIOContext->offset;
    }
    if (customIOContext->position != customIOContext->offset) {
        Status pluginRet = Status::ERROR_UNKNOWN;
        while (pluginRet == Status::ERROR_UNKNOWN) {
            pluginRet = customIOContext->sourcePlugin->SeekTo(customIOContext->offset);
            if (static_cast<int32_t>(pluginRet) < 0 && pluginRet != Status::ERROR_UNKNOWN) {
                AVCODEC_LOGE("Seek to %{public}zu failed when read AVPacket!", customIOContext->offset);
                return rtv;
            } else if (pluginRet == Status::ERROR_UNKNOWN) {
                AVCODEC_LOGW("Seek to %{public}zu failed when read AVPacket, try again", customIOContext->offset);
                sleep(1);
            }
        }
        customIOContext->position = customIOContext->offset;
    }
    int32_t result = static_cast<int32_t>(
                customIOContext->sourcePlugin->Read(buffer, static_cast<size_t>(readSize)));
    if (result == 0) {
        rtv = buffer->GetMemory()->GetSize();
        customIOContext->offset += rtv;
        customIOContext->position += rtv;
    } else if (static_cast<int>(result) == 1) {
        customIOContext->eof = true;
        rtv = AVERROR_EOF;
    } else {
        AVCODEC_LOGE("AVReadPacket failed with rtv = %{public}d", static_cast<int>(result));
    }

    return rtv;
}

int32_t Source::InitAVFormatContext()
{
    AVFormatContext *formatContext = avformat_alloc_context();
    if (formatContext == nullptr) {
        AVCODEC_LOGE("InitAVFormatContext failed, because  alloc AVFormatContext failed.");
        return AVCS_ERR_INVALID_OPERATION;
    }
    InitAVIOContext(AVIO_FLAG_READ);
    if (avioContext_ == nullptr) {
        AVCODEC_LOGE("InitAVFormatContext failed, because  init AVIOContext failed.");
        return AVCS_ERR_INVALID_OPERATION;
    }
    formatContext->pb = avioContext_;
    formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

    int ret = avformat_open_input(&formatContext, nullptr, inputFormat_.get(), nullptr);
    if (ret != 0) {
        AVCODEC_LOGE("avformat_open_input failed by %{public}s, err:%{public}s", inputFormat_->name,
            FFMpegConverter::AVStrError(ret).c_str());
        return AVCS_ERR_INVALID_OPERATION;
    }

    ret = avformat_find_stream_info(formatContext, NULL);
    if (ret < 0) {
        AVCODEC_LOGE("avformat_find_stream_info failed by %{public}s, err:%{public}s",
            inputFormat_->name, FFMpegConverter::AVStrError(ret).c_str());
        return AVCS_ERR_INVALID_OPERATION;
    }

    formatContext_ = std::shared_ptr<AVFormatContext>(formatContext, [](AVFormatContext* ptr) {
        if (ptr != nullptr) {
            auto p = ptr->pb;
            avformat_close_input(&ptr);
            if (p != nullptr) {
                p->opaque = nullptr;
                av_freep(&(p->buffer));
                av_opt_free(p);
                avio_context_free(&p);
                p = nullptr;
            }
        }
    });

    GetVideoFirstKeyFrame(); // 如果是hevc，需要获取视频轨的第一帧关键帧获取sei里边的属性
    if (hasHevc_) {
        CHECK_AND_RETURN_RET_LOG(firstFrame_ != nullptr && firstFrame_->data != nullptr,
            AVCS_ERR_INVALID_STATE, "Init AVFormatContext failed due to get sei info failed.");
    }
    return AVCS_ERR_OK;
}

uintptr_t Source::GetSourceAddr()
{
    CHECK_AND_RETURN_RET_LOG(formatContext_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                             "GetSourceAddr failed, formatContext_ is nullptr!");
    return (uintptr_t)(formatContext_.get());
}
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS