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
int32_t DataProducerBase::Init(const std::shared_ptr<SampleInfo> &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sampleInfo_ = info;
    inputFile_ = std::make_unique<std::ifstream>(sampleInfo_->inputFilePath.data(), std::ios::binary | std::ios::in);
    CHECK_AND_RETURN_RET_LOG(inputFile_->is_open(), AVCODEC_SAMPLE_ERR_ERROR, "Open input file failed");

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t DataProducerBase::ReadSample(CodecBufferInfo &bufferInfo)
{
    if ((frameCount_ >= sampleInfo_->maxFrames || IsEOS()) && !Repeat()) {
        bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        return AVCODEC_SAMPLE_ERR_OK;
    }

    int32_t ret = FillBuffer(bufferInfo);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, ret, "Fill buffer failed");
    DumpInput(bufferInfo);

    if (bufferInfo.attr.pts == 0) {
        bufferInfo.attr.pts = frameCount_ *
            ((sampleInfo_->frameInterval <= 0) ? 1 : sampleInfo_->frameInterval) * 1000; // 1000: 1ms to us
    }
    frameCount_++;
    PrintProgress(sampleInfo_->sampleRepeatTimes, frameCount_);
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
    if (--sampleInfo_->sampleRepeatTimes < 0) {
        return false;
    }

    frameCount_ = 0;

    int32_t ret = Seek(0);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, false, "Seek failed");
    AVCODEC_LOGI("Seek input file to head, repeat times left: %{public}u", sampleInfo_->sampleRepeatTimes);
    return true;
}

void DataProducerBase::DumpInput(const CodecBufferInfo &bufferInfo)
{
    CHECK_AND_RETURN(sampleInfo_->needDumpInput);

    if (inputDumpFile_ == nullptr) {
        using namespace std::string_literals;

        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string inputFileName;
        if (sampleInfo_->codecType & 0b10) {  // 0b10: Video encoder mask
            inputFileName = "VideoEncoderIn_"s + ToString(sampleInfo_->pixelFormat) + "_" +
                std::to_string(sampleInfo_->videoWidth) + "_" + std::to_string(sampleInfo_->videoHeight) + "_" +
                std::to_string(time) + ".yuv";
        } else {
            inputFileName = "VideoDecoderIn_"s + std::to_string(time) + ".bin";
        }

        inputDumpFile_ = std::make_unique<std::ofstream>(inputFileName, std::ios::out | std::ios::trunc);
        if (!inputDumpFile_->is_open()) {
            inputDumpFile_ = nullptr;
            AVCODEC_LOGE("Output file open failed");
            return;
        }
    }

    uint8_t *bufferAddr = nullptr;
    if (bufferInfo.bufferAddr != nullptr) {
        bufferAddr = bufferInfo.bufferAddr;
    } else {
        bufferAddr = static_cast<uint8_t>(sampleInfo_->codecRunMode) & 0b10 ?    // 0b10: AVBuffer mode mask
                        OH_AVBuffer_GetAddr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer)) :
                        OH_AVMemory_GetAddr(reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer));
    }

    CHECK_AND_RETURN_LOG(bufferAddr != nullptr, "Buffer is nullptr");
    inputDumpFile_->write(reinterpret_cast<char *>(bufferAddr), bufferInfo.attr.size);
}

std::shared_ptr<DataProducerBase> DataProducerFactory::CreateDataProducer(const DataProducerType &type)
{
    std::shared_ptr<DataProducerBase> dataProducer;
    switch (type) {
        case DATA_PRODUCER_TYPE_DEMUXER:
            dataProducer = std::static_pointer_cast<DataProducerBase>(std::make_shared<Demuxer>());
            break;
        case DATA_PRODUCER_TYPE_BITSTREAM_READER:
            dataProducer = std::static_pointer_cast<DataProducerBase>(std::make_shared<BitstreamReader>());
            break;
        case DATA_PRODUCER_TYPE_RAW_DATA_READER:
            dataProducer = std::static_pointer_cast<DataProducerBase>(std::make_shared<RawdataReader>());
            break;
        default:
            AVCODEC_LOGE("Not supported data producer, type: %{public}d", type);
            dataProducer = nullptr;
    }

    return dataProducer;
}
} // Sample
} // MediaAVCodec
} // OHOS