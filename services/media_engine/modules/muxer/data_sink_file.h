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

#ifndef AVCODEC_DATA_SINK_FILE_H
#define AVCODEC_DATA_SINK_FILE_H

#include <iostream>
#include "plugin/data_sink.h"

namespace OHOS {
namespace Media {
class DataSinkFile : public Plugins::DataSink {
public:
    explicit DataSinkFile(FILE *file);
    DataSinkFile(const DataSinkFile &other) = delete;
    DataSinkFile& operator=(const DataSinkFile&) = delete;
    virtual ~DataSinkFile();

    int32_t Read(uint8_t *buf, int32_t bufSize) override;
    int32_t Write(const uint8_t *buf, int32_t bufSize) override;
    int64_t Seek(int64_t offset, int whence) override;
    int64_t GetCurrentPosition() const override;
    bool CanRead() override;

private:
    FILE *file_;
    int64_t pos_;
    int64_t end_;
    bool isCanRead_;
};
} // Media
} // OHOS
#endif