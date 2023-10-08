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

#ifndef PLUGIN_CORE_MUXER_H
#define PLUGIN_CORE_MUXER_H

#include "muxer_plugin.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
class Muxer {
public:
    Muxer(const Muxer &) = delete;
    Muxer operator=(const Muxer &) = delete;
    ~Muxer() = default;

    Status SetDataSink(const std::shared_ptr<DataSink>& dataSink);
    Status SetRotation(int32_t rotation);
    Status AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc);
    Status Start();
    Status WriteSample(uint32_t trackIndex, const uint8_t *sample, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    Status Stop();

private:
    friend class MuxerFactory;

    Muxer(uint32_t pkgVer, uint32_t apiVer, std::shared_ptr<MuxerPlugin> plugin);

private:
    const uint32_t pkgVersion_;
    const uint32_t apiVersion_;
    std::shared_ptr<MuxerPlugin> muxer_;
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // PLUGIN_CORE_MUXER_H
