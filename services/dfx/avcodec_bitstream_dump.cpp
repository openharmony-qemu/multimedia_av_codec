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

#include "avcodec_bitstream_dump.h"
#include <fstream>
#include <unistd.h>
#include <malloc.h>
#include <sys/time.h>
#include <sstream>
#include <map>
#include <iomanip>
#include "avcodec_log.h"

namespace {
    constexpr ::OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecBitStreamDumper"};
    constexpr int32_t FILE_MAX = 100;
    constexpr int32_t FILE_BIT_STREAM_MAX = 1000;
    constexpr uint8_t HEX_WIDTH = 2;

    const std::map<OHOS::MediaAVCodec::BitStreamDumpType, std::string_view> BIT_STREAM_DUMP_MAP = {
        { OHOS::MediaAVCodec::BitStreamDumpType::BIT_STREAM_DUMP_TYPE_DEFAULT, "Default" },
        { OHOS::MediaAVCodec::BitStreamDumpType::BIT_STREAM_DUMP_TYPE_VCODEC, "Video_Codec" },
        { OHOS::MediaAVCodec::BitStreamDumpType::BIT_STREAM_DUMP_TYPE_ACODEC, "Audio_Codec" },
        { OHOS::MediaAVCodec::BitStreamDumpType::BIT_STREAM_DUMP_TYPE_MUXER, "Muxer" },
        { OHOS::MediaAVCodec::BitStreamDumpType::BIT_STREAM_DUMP_TYPE_DEMUXER, "Demuxer" },
        { OHOS::MediaAVCodec::BitStreamDumpType::BIT_STREAM_DUMP_TYPE_SOURCE, "Source" },
    };
}

namespace OHOS {
namespace MediaAVCodec {
AVCodecBitStreamDumper &AVCodecBitStreamDumper::GetInstance()
{
    static AVCodecBitStreamDumper avcodecBitStreamDumper;
    return avcodecBitStreamDumper;
}

AVCodecBitStreamDumper::AVCodecBitStreamDumper() {}

AVCodecBitStreamDumper::~AVCodecBitStreamDumper()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isExit_ = true;
        cond_.notify_all();
    }
    if (thread_ != nullptr && thread_->joinable()) {
        thread_->join();
    }
}

static void AddNewBitStream(BitStreamDumpType type, const std::string &name,
                            const uint32_t index, const uint32_t size, std::string &bitstreamStr)
{
    struct timeval time = {};
    (void)gettimeofday(&time, nullptr);
    int64_t second = time.tv_sec % 60;
    int64_t allMinute = time.tv_sec / 60;
    int64_t minute = allMinute % 60;
    int64_t hour = allMinute / 60 % 24;
    int64_t mSecond = time.tv_usec / 1000;

    std::stringstream outStream;
    outStream << "### ";
    outStream << "[" << BIT_STREAM_DUMP_MAP.at(type).data() << "] ";
    outStream << "[" << name << "] ";
    outStream << "[" << "Index: " << std::to_string(index) << "] ";
    outStream << "[" << "Size: " << std::to_string(size) << "] ";
    outStream << "[" << std::to_string(hour) << ":" << std::to_string(minute) << ":" << std::to_string(second) <<
        "." << std::to_string(mSecond) << "] ";
    outStream << "[" << "pid: " << std::to_string(getpid()) << "] ";
    outStream << "[" << "tid: " << std::to_string(gettid()) << "] ";
    outStream << "[" << "uid: " << std::to_string(getuid()) << "] ";
    outStream << std::endl;

    bitstreamStr += outStream.str();
}

void AVCodecBitStreamDumper::SaveBitStream(BitStreamDumpType type, const std::string &name,
                                           const uint32_t index, const uint8_t* buffer, const uint32_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!isEnable_) {
        if (thread_ != nullptr) {
            AVCODEC_LOGI("Bitstream dumper shutdown.");
            thread_.reset();
        }
        return;
    }

    if (thread_ == nullptr) {
        AVCODEC_LOGI("Bitstream dumper init");
        thread_ = std::make_unique<std::thread>(&AVCodecBitStreamDumper::TaskProcessor, this);
    }

    std::stringstream outStream;
    outStream << std::hex << std::uppercase << std::setfill('0');
    for (uint32_t idx = 0; idx < size; idx++) {
        outStream << std::setw(HEX_WIDTH) << (int)(buffer[idx]) << " ";
    }

    AddNewBitStream(type, name, index, size, bitstreamString_);

    bitstreamString_ += outStream.str();
    bitstreamString_ += "\n\n";

    isDump_ = true;

    bitStreamCount_++;
    if (bitStreamCount_ >= FILE_BIT_STREAM_MAX) {
        cond_.notify_all();
    }
}

bool AVCodecBitStreamDumper::SwitchEnable()
{
    isEnable_ = !isEnable_;
    std::string status = isEnable_ ? "Enable" : "Disable";
    AVCODEC_LOGI("Bitstream dumper on status: %{public}s", status.c_str());
    return isEnable_;
}

void AVCodecBitStreamDumper::TaskProcessor()
{
    pthread_setname_np(pthread_self(), "AVCodecBitStreamDumperTask");
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
    while (true) {
        std::string temp;
        int32_t lineCount = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (isExit_) {
                return;
            }
            static constexpr int32_t timeout = 5; // every 5 second have a log
            cond_.wait_for(lock, std::chrono::seconds(timeout),
                [this] {
                    return isExit_ || isDump_ || bitStreamCount_ >= FILE_BIT_STREAM_MAX || !bitstreamString_.empty();
            });
            isDump_ = false;
            lineCount = bitStreamCount_;
            bitStreamCount_ = bitStreamCount_ >= FILE_BIT_STREAM_MAX ? 0 : bitStreamCount_;
            swap(bitstreamString_, temp);
        }

        std::string file = "/data/media/av_codec/bitstream_dump/";
        file += std::to_string(getpid());
        file += "_bitstream_dump_";
        file += std::to_string(fileCount_) + ".log";

        std::ofstream ofStream;
        if (isNewFile_) {
            ofStream.open(file, std::ios::out | std::ios::trunc);
        } else {
            ofStream.open(file, std::ios::out | std::ios::app);
        }
        if (!ofStream.is_open()) {
            AVCODEC_LOGE("Open bitstream dump file failed");
            continue;
        }
        isNewFile_ = false;
        if (lineCount >= FILE_BIT_STREAM_MAX) {
            isNewFile_ = true;
            fileCount_++;
            fileCount_ = fileCount_ > FILE_MAX ? 0 : fileCount_;
        }

        AVCODEC_LOGI("Save bitstream");
        ofStream.write(temp.c_str(), temp.size());
        ofStream.close();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS