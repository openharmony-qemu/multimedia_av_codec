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

#ifndef SOURCE_H
#define SOURCE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#ifdef __cplusplus
}
#endif

#include <thread>
#include "avcodec_errors.h"
#include "hevc_parser_manager.h"
#include "source_plugin.h"
#include "plugin_types.h"
#include "plugin_buffer.h"
#include "plugin_event.h"
#include "plugin_definition.h"
#include "sourcebase.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
struct SourceRegister : PackageRegister {
public:
    std::string name = "custom register";
    Status AddPlugin(const PluginDefBase& def) override;
    Status AddPackage(const PackageDef& def) override;
    std::shared_ptr<SourcePlugin> sourcePlugin {nullptr};
private:
    std::shared_ptr<PackageDef> packageDef;
};

class Source : public SourceBase {
public:
    Source();
    ~Source();

    int32_t Init(std::string& uri) override;
    int32_t GetTrackCount(uint32_t &trackCount) override;
    int32_t GetSourceFormat(Media::Format &format) override;
    int32_t GetTrackFormat(Media::Format &format, uint32_t trackIndex) override;
    uintptr_t GetSourceAddr() override;

private:
    struct CustomIOContext {
        SourcePlugin* sourcePlugin = nullptr;
        size_t offset = 0;
        size_t position = 0;
        bool eof = false;
        uint64_t fileSize = 0;
        AVIOContext* avioContext = nullptr;
    };

    std::shared_ptr<AVFormatContext> formatContext_;
    std::shared_ptr<AVInputFormat> inputFormat_;
    std::shared_ptr<SourcePlugin> sourcePlugin_;
    std::shared_ptr<SourceRegister> register_;
    CustomIOContext customIOContext_;
    AVIOContext* avioContext_ = nullptr;
    void* handler_ = nullptr;
    std::shared_ptr<HevcParserManager> hevcParser_ {nullptr};
    int32_t LoadDynamicPlugin(const std::string& path);
    int32_t SniffInputFormat();
    static int AVReadPacket(void* opaque, uint8_t* buf, int bufSize);
    static int64_t AVSeek(void* opaque, int64_t offset, int whence);
    void InitAVIOContext(int flags);
    int32_t InitAVFormatContext();
    void GetVideoFirstKeyFrame();
    void ParseHEVCMetadataInfo(const AVStream& avStream, Media::Format &format);
    void ParseHDRVividCUVVInfo(Media::Format &format);
    bool IsAVTrack(const AVStream &avStream);
    AVPacket *firstFrame_ = nullptr;
    bool hasHevc_ = false;
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVSOURCE_H