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

#include "avmuxer_mock.h"
#include <iostream>
#include "avmuxer_ffmpeg_plugin_mock.h"
#include "plugin/plugin_manager_v2.h"
#include "data_sink_fd.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
std::shared_ptr<AVMuxerMock> AVMuxerMockFactory::CreateMuxer(int32_t fd, const OH_AVOutputFormat &format)
{
    static const std::unordered_map<OH_AVOutputFormat, std::string> table = {
        {AV_OUTPUT_FORMAT_DEFAULT, "ffmpegMux_mp4"},
        {AV_OUTPUT_FORMAT_MPEG_4, "ffmpegMux_mp4"},
        {AV_OUTPUT_FORMAT_M4A, "ffmpegMux_ipod"},
    };
    std::string name = "ffmpegMux_mp4";
    if (table.find(format) != table.end()) {
        name = table.at(format);
    }
    auto muxer = PluginManagerV2::Instance().CreatePluginByName(name);
    if (muxer == nullptr) {
        std::cout << "create plugin by name failed! name:" << name << std::endl;
    }
    return std::make_shared<AVMuxerFfmpegPluginMock>(std::make_shared<DataSinkFd>(fd),
        std::reinterpret_pointer_cast<MuxerPlugin>(muxer));
}
}  // namespace MediaAVCodec
}  // namespace OHOS