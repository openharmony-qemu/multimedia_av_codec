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
#ifndef HISTREAMER_INITIAL_STREAM_SELECTION_H
#define HISTREAMER_INITIAL_STREAM_SELECTION_H

#include <climits>
#include <cstdint>
#include <limits>

namespace OHOS::Media::Plugins::HttpPlugin {
// Shared by the probe URL selection and the formal HTTP request metadata.
template<typename Streams>
typename Streams::value_type ChooseInitialStream(const Streams& streams, uint64_t resolution)
{
    typename Streams::value_type selected;
    uint64_t best = std::numeric_limits<uint64_t>::max();
    for (const auto& stream : streams) {
        if (stream == nullptr) {
            continue;
        }
        if (selected == nullptr) {
            selected = stream;
        }
        if (resolution == 0) {
            return selected;
        }
        if (stream->width >= std::numeric_limits<unsigned short>::max() ||
            stream->height >= std::numeric_limits<unsigned short>::max()) {
            continue;
        }
        uint64_t pixels = static_cast<uint64_t>(stream->width) * stream->height;
        uint64_t distance = pixels > resolution ? pixels - resolution : resolution - pixels;
        if (distance < best || (distance == best && stream->bitrate < selected->bitrate)) {
            selected = stream;
            best = distance;
        }
    }
    return selected;
}
} // namespace OHOS::Media::Plugins::HttpPlugin
#endif
