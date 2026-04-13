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

#ifndef AVCODEC_FFMPEG_MUXER_REGISTER_H
#define AVCODEC_FFMPEG_MUXER_REGISTER_H

#include <unordered_map>
#include <set>
#include <string>
#include "plugin/muxer_plugin.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
class FFmpegMuxerRegister {
public:
    static Status RegisterMuxerPlugins(const std::shared_ptr<Register> &reg);
    static void UnregisterMuxerPlugins();
    static void FfmpegLogPrintWithLevel(void* avcl, int level, const char* fmt, va_list vl);
    static void ResetCodecParameter(AVCodecParameters* par);
    static std::shared_ptr<AVOutputFormat> GetAVOutputFormat(std::string pluginName);

    static AVIOContext* InitAvIoCtx(const std::shared_ptr<DataSink> &dataSink, int writeFlags);
    static void DeInitAvIoCtx(AVIOContext* ptr);
    static int32_t IoOpen(AVFormatContext* s, AVIOContext** pb, const char* url, int flags, AVDictionary** options);
    static int IoClose(AVFormatContext* s, AVIOContext* pb);

private:
    static bool UpdatePluginCapability(const AVOutputFormat* oFmt, MuxerPluginDef &pluginDef);
    static bool UpdatePluginInCapability(AVCodecID codecId, MuxerPluginDef &pluginDef);
    static bool FormatName2OutCapability(const std::string &fmtName, MuxerPluginDef &pluginDef);
    static bool CodecId2Cap(AVCodecID codecId, bool encoder, Capability &cap);
    static bool IsMuxerSupported(const char* name);

    static int32_t IoRead(void* opaque, uint8_t* buf, int bufSize);
    static int32_t IoWrite(void* opaque, const uint8_t* buf, int bufSize);
    static int64_t IoSeek(void* opaque, int64_t offset, int whence);

private:
    struct IOContext {
        std::shared_ptr<DataSink> dataSink_ {};
        int64_t pos_ {0};
        int64_t end_ {0};
    };

    static std::map<std::string, std::shared_ptr<AVOutputFormat>> pluginOutputFmt_;
    static std::set<std::string> supportedMuxer_;
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_FFMPEG_MUXER_REGISTER_H
