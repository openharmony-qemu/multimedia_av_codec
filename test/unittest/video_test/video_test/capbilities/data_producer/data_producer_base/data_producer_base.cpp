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

#include "data_producer_base.h"
#include "sample_helper.h"
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

#include "demuxer.h"
#include "bitstream_reader.h"
#include "rawdata_reader.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "DataProducerBase"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
DataProducerBase::~DataProducerBase()
{
    Release();
}

int32_t DataProducerBase::Init(SampleInfo &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sampleInfo_ = info;
    inputFile_ = std::make_unique<std::ifstream>(sampleInfo_.inputFilePath.data(), std::ios::binary | std::ios::in);
    CHECK_AND_RETURN_RET_LOG(inputFile_->is_open(), AVCODEC_SAMPLE_ERR_ERROR, "Open input file failed");

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t DataProducerBase::ReadSample(CodecBufferInfo &bufferInfo)
{
    if ((frameCount_ >= sampleInfo_.maxFrames) || (IsEOS() && !Repeat())) {
        bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        return AVCODEC_SAMPLE_ERR_OK;
    }

    int32_t ret = FillBuffer(bufferInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Fill buffer failed");

    frameCount_++;
    PrintProgress(sampleInfo_.repeatTimes, frameCount_);
    return ret;
}

inline int32_t DataProducerBase::Seek(int64_t position)
{
    CHECK_AND_RETURN_RET_LOG(inputFile_ != nullptr && inputFile_->is_open(),
        AVCODEC_SAMPLE_ERR_ERROR, "Input file is not open!");
    inputFile_->clear();
    inputFile_->seekg(position, std::ios::beg);
    return AVCODEC_SAMPLE_ERR_OK;
}

bool DataProducerBase::Repeat()
{
    if (--sampleInfo_.repeatTimes <= 0) {
        return false;
    }

    int32_t ret = Seek(0);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, false, "Seek failed");
    AVCODEC_LOGI("Seek input file to head, repeat times left: %{public}u", sampleInfo_.repeatTimes);
    return true;
}

int32_t DataProducerBase::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (inputFile_ != nullptr && inputFile_->is_open()) {
        inputFile_->close();
    }
    inputFile_ = nullptr;
    return AVCODEC_SAMPLE_ERR_OK;
}

std::shared_ptr<DataProducerBase> DataProducerFactory::CreateDataProducer(DataProducerInfo info)
{
    std::shared_ptr<DataProducerBase> dataProducer;
    switch (info.dataProducerType) {
        case DATA_PRODUCER_TYPE_DEMUXER:
            dataProducer = std::static_pointer_cast<DataProducerBase>(std::make_shared<Demuxer>());
            break;
        case DATA_PRODUCER_TYPE_BITSTREAM_READER:
            dataProducer =
                std::static_pointer_cast<DataProducerBase>(std::make_shared<BitstreamReader>(info.bitstreamType));
            break;
        case DATA_PRODUCER_TYPE_RAW_DATA_READER:
            dataProducer = std::static_pointer_cast<DataProducerBase>(std::make_shared<RawdataReader>());
            break;
        default:
            AVCODEC_LOGE("Not supported data producer, type: %{public}d", info.dataProducerType);
            dataProducer = nullptr;
    }

    return dataProducer;
}
} // Sample
} // MediaAVCodec
} // OHOS