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

#ifndef DATA_SINK_H
#define DATA_SINK_H

#include <iostream>

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
class DataSink {
public:
    virtual int32_t Read(uint8_t *buf, int32_t bufSize) = 0;
    virtual int32_t Write(uint8_t *buf, int32_t bufSize) = 0;
    virtual int64_t Seek(int64_t offset, int whence) = 0;
    virtual int64_t GetPos() const = 0;
};
} // Plugin
} // MediaAVCodec
} // OHOS
#endif