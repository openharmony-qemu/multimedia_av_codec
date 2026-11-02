/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_SNIFF_RESULT_H
#define HISTREAMER_SNIFF_RESULT_H

#include "http_source_plugin_stream_protocol_sniffer.h"

namespace OHOS::Media::Plugins::HttpPlugin {
// Classification only. Probe bytes and transport are destroyed before Open().
struct SniffResult {
    StreamProtocolType type {StreamProtocolType::HTTP};
    bool isFlv {false};
    bool sniffOk {false};
    size_t bytesProbed {0};
};
} // namespace OHOS::Media::Plugins::HttpPlugin
#endif
