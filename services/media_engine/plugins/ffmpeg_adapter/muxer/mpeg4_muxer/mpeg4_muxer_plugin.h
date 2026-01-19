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

#ifndef MPEG4_MUXER_PLUGIN_H
#define MPEG4_MUXER_PLUGIN_H

#include "box_parser.h"
#include "avio_stream.h"
#include "basic_track.h"
#include "plugin/muxer_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
class Mpeg4MuxerPlugin : public MuxerPlugin {
public:
    explicit Mpeg4MuxerPlugin(std::string name);
    ~Mpeg4MuxerPlugin() override;

    Status SetDataSink(const std::shared_ptr<DataSink> &dataSink) override;
    Status AddTrack(int32_t& trackIndex, const std::shared_ptr<Meta> &trackDesc) override;
    Status Start() override;
    Status WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample) override;
    Status Stop() override;

    Status SetParameter(const std::shared_ptr<Meta> &param) override;
    Status SetUserMeta(const std::shared_ptr<Meta> &userMeta) override;
    Status Reset() override
    {
        return Status::NO_ERROR;
    }

private:
    Status WriteHeader();
    Status WriteTailer();
    Status MoveMoovBoxToFront(bool isNeedFree);
    Status SetRotation(const std::shared_ptr<Meta> &param);
    Status SetLocation(const std::shared_ptr<Meta> &param);
    Status SetCreationTime(const std::shared_ptr<Meta> &param);
    bool CheckGltfParam(std::shared_ptr<Meta> &param);
    void WriteMdatSize();
    void WriteFileLevelMetafBox();

private:
    std::shared_ptr<AVIOStream> io_ = nullptr;
    VideoRotation rotation_ { VIDEO_ROTATION_0 };
    uint32_t outputFormat_ = OHOS::MediaAVCodec::OUTPUT_FORMAT_MPEG_4;
    std::shared_ptr<BasicBox> moov_ = nullptr;
    std::shared_ptr<BoxParser> boxParser_ = nullptr;
    std::vector<std::shared_ptr<BasicTrack>> tracks_;
    std::shared_ptr<Meta> param_ = nullptr;
    std::shared_ptr<Meta> userMeta_ = nullptr;
    int64_t freePos_ = 0;
    int64_t mdatPos_ = 0;
    bool isH264_ = false;
    float latitude_ = 1000.0f;
    float longitude_ = 1000.0f;
    bool hasGltf_ = false;
    bool useTimedMeta_ = false;
    bool needFree_ = false;
    bool hasAigc_ = false;
};
} // Mpeg4
} // Plugins
} // Media
} // OHOS
#endif