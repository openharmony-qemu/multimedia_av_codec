/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_SAMPLE_DATA_PRODUCER_BASE_H
#define AVCODEC_SAMPLE_DATA_PRODUCER_BASE_H

#include <fstream>
#include <memory>
#include <mutex>
#include "sample_info.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class DataProducerBase {
public:
    virtual ~DataProducerBase() {};
    virtual int32_t Init(const std::shared_ptr<SampleInfo> &info);
    int32_t ReadSample(CodecBufferInfo &bufferInfo);
    virtual int32_t Seek(int64_t position);
    virtual bool Repeat();

protected:
    virtual int32_t FillBuffer(CodecBufferInfo &bufferInfo) = 0;
    virtual bool IsEOS() = 0;
    void DumpInput(const CodecBufferInfo &bufferInfo);

    std::unique_ptr<std::ifstream> inputFile_ = nullptr;
    std::unique_ptr<std::ofstream> inputDumpFile_ = nullptr;
    std::mutex mutex_;
    std::shared_ptr<SampleInfo> sampleInfo_;
    uint32_t frameCount_ = 0;
};

class DataProducerFactory {
public:
    static std::shared_ptr<DataProducerBase> CreateDataProducer(const DataProducerType &type);
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_DATA_PRODUCER_BASE_H