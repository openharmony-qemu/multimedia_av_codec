/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef BASE_CONFIG_H
#define BASE_CONFIG_H

#include "meta/media_types.h"

#include "avformat/avformat_mock.h"
#include "avsource_mock.h"
#include "demuxer_mock.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;

struct BufferValue {
    uint8_t* addr;
    size_t size;
};

using IntBufferValue = std::vector<int32_t>;
// {key, value}
using IntValueMap = std::map<std::string, int32_t>;
using LongValueMap = std::map<std::string, int64_t>;
using FloatValueMap = std::map<std::string, float>;
using DoubleValueMap = std::map<std::string, double>;
using StringValueMap = std::map<std::string, std::string>;
using BufferValueMap = std::map<std::string, BufferValue>;
using IntBufferValueMap = std::map<std::string, IntBufferValue>;

enum SourceType {
    LOCAL = 0,
    URI = 1
};

enum ValueType {
    INT = 0,
    LONG = 1,
    FLOAT = 2,
    DOUBLE = 3,
    STRING = 4,
    BUFFER = 5,
    INT_BUFFER = 6,
};

struct Info {
    IntValueMap intValue;
    LongValueMap longValue;
    FloatValueMap floatValue;
    DoubleValueMap doubleValue;
    StringValueMap stringValue;
    BufferValueMap bufferValue;
    IntBufferValueMap intBufferValue;
};

struct SeekInfoRecord {
    bool seekSuccess;
    // trackIndex, frameCount
    std::map<int32_t, int64_t> seekFrameInfo;
};
// {trakcIndex, {frameCount, keyFrameCount}}
using ReadInfo = std::map<int32_t, std::vector<int64_t>>;
// {seekTime, {seekMode, SeekInfo}}
using SeekInfo = std::map<int64_t, std::map<SeekMode, SeekInfoRecord>>;

struct FileBaseInfo {
    Info sourceInfo;
    // {trackIndex, info}
    std::map<int32_t, Info> trackInfo;
    ReadInfo expectReadInfo;
    SeekInfo expectSeekInfo;
};

using SourceInfoMap = std::map<std::string, Info>;
using TrackInfoMap = std::map<std::string, std::map<int32_t, Info>>;
using ReadInfoMap = std::map<std::string, ReadInfo>;
using SeekInfoMap = std::map<std::string, std::map<int64_t, std::vector<SeekInfoRecord>>>;
} // namespace MediaAVCodec
} // namespace OHOS
#endif // BASE_CONFIG_H
