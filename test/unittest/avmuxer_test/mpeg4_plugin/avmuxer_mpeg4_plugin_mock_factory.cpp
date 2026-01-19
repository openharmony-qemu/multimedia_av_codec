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

#include "avmuxer_mock.h"
#include "avmuxer_mpeg4_plugin_mock.h"
#include "mpeg4_muxer_plugin.h"
#include "data_sink_fd.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
std::shared_ptr<AVMuxerMock> AVMuxerMockFactory::CreateMuxer(int32_t fd, const OH_AVOutputFormat &format)
{
    static const std::unordered_map<OH_AVOutputFormat, std::string> table = {
        {AV_OUTPUT_FORMAT_DEFAULT, "Mpeg4Mux_mp4"},
        {AV_OUTPUT_FORMAT_MPEG_4, "Mpeg4Mux_mp4"},
        {AV_OUTPUT_FORMAT_M4A, "Mpeg4Mux_m4a"},
    };
    std::string name = "Mpeg4Mux_mp4";
    if (table.find(format) != table.end()) {
        name = table.at(format);
    }
    return std::make_shared<AVMuxerMpeg4PluginMock>(std::make_shared<DataSinkFd>(fd),
        std::make_shared<Mpeg4::Mpeg4MuxerPlugin>(name));
}
}  // namespace MediaAVCodec
}  // namespace OHOS