/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_SAMPLE_H
#define MPEG4_SAMPLE_H

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>
#include "common/status.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

struct Sample {
    enum SampleFlag : uint32_t {
        NONE = 0,
        EOS = 1 << 0,
        SYNC_FRAME = 1 << 1,
        DISCARD = 1 << 4,
    };
    int64_t pts;
    int64_t dts;
    int64_t duration;
    // File position is stable across reads and is used to identify a sample.
    int64_t pos = -1;
    uint32_t flag;
    int32_t size;
    std::vector<uint8_t> skipSamplesInfo;
    std::unique_ptr<uint8_t[]> data;
};

struct MPEG4Sample {
    int32_t offset = 0;
    std::shared_ptr<Sample> sample = nullptr;
    bool isAnnexb = false;
    bool isVttc = false;
    uint32_t trackId = UINT32_MAX;
    uint32_t sampleIndex = 0;
};

struct SampleIndexView {
    int64_t pos = 0;
    int64_t dts = 0;
    int64_t pts = 0;
    int64_t duration = 0;
    uint32_t flag = 0;
    int32_t size = 0;
};

class SampleCache {
public:
    bool HasCache() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !samples_.empty();
    }

    uint32_t GetCacheDataSize() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return dataSize_ > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(dataSize_);
    }

    uint32_t GetCacheFrameCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return samples_.size() > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(samples_.size());
    }

    bool ResetInfo(const std::shared_ptr<MPEG4Sample>& block)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (block == nullptr || block->sample == nullptr || samples_.empty()) {
            return true;
        }
        uint64_t blockSize = 0;
        if (!GetBlockSize(block, blockSize)) {
            return true;
        }
        dataSize_ = dataSize_ >= blockSize ? dataSize_ - blockSize : 0;
        return true;
    }

    bool SetInfo(const std::shared_ptr<MPEG4Sample>& block)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (block == nullptr || block->sample == nullptr || samples_.empty()) {
            return true;
        }
        uint64_t blockSize = 0;
        if (!GetBlockSize(block, blockSize)) {
            return true;
        }
        dataSize_ += blockSize;
        return true;
    }

    bool Push(std::shared_ptr<MPEG4Sample> block)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return PushLocked(std::move(block));
    }

    bool PushEosIfAbsent(std::shared_ptr<MPEG4Sample> block, bool& inserted)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        inserted = false;
        if (block == nullptr || block->sample == nullptr ||
            (block->sample->flag & Sample::SampleFlag::EOS) == 0) {
            return false;
        }
        if (!samples_.empty()) {
            const auto& tail = samples_.back();
            if (tail != nullptr && tail->sample != nullptr &&
                (tail->sample->flag & Sample::SampleFlag::EOS) != 0) {
                return true;
            }
        }
        if (!PushLocked(std::move(block))) {
            return false;
        }
        inserted = true;
        return true;
    }

    std::shared_ptr<MPEG4Sample> Pop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (samples_.empty()) {
            return nullptr;
        }
        auto block = samples_.front();
        samples_.pop_front();
        UpdateStatsOnPop(block);
        return block;
    }

    std::shared_ptr<MPEG4Sample> Front() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return samples_.empty() ? nullptr : samples_.front();
    }

    std::shared_ptr<MPEG4Sample> Back() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return samples_.empty() ? nullptr : samples_.back();
    }

    Status GetLastPTS(int64_t& maxPts) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (samples_.empty()) {
            maxPts = INT64_MIN;
            return Status::ERROR_NOT_EXISTED;
        }
        bool found = false;
        maxPts = INT64_MIN;
        for (const auto& block : samples_) {
            if (block == nullptr || block->sample == nullptr) {
                continue;
            }
            if (!found || block->sample->pts > maxPts) {
                maxPts = block->sample->pts;
                found = true;
            }
        }
        return found ? Status::OK : Status::ERROR_NOT_EXISTED;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        samples_.clear();
        dataSize_ = 0;
    }

private:
    bool PushLocked(std::shared_ptr<MPEG4Sample> block)
    {
        if (block == nullptr || block->sample == nullptr) {
            return false;
        }
        uint64_t blockSize = 0;
        if (!GetBlockSize(block, blockSize)) {
            return false;
        }
        samples_.push_back(std::move(block));
        UpdateStatsOnPush(blockSize);
        return true;
    }
    static bool GetBlockSize(const std::shared_ptr<MPEG4Sample>& block, uint64_t& blockSize)
    {
        if (block == nullptr || block->sample == nullptr || block->sample->size < 0) {
            return false;
        }
        blockSize = static_cast<uint64_t>(block->sample->size);
        return true;
    }

    void UpdateStatsOnPush(uint64_t blockSize)
    {
        dataSize_ += blockSize;
    }

    void UpdateStatsOnPop(const std::shared_ptr<MPEG4Sample>& block)
    {
        uint64_t blockSize = 0;
        if (!GetBlockSize(block, blockSize)) {
            return;
        }
        dataSize_ = dataSize_ >= blockSize ? dataSize_ - blockSize : 0;
    }

    mutable std::mutex mutex_;
    std::deque<std::shared_ptr<MPEG4Sample>> samples_;
    uint64_t dataSize_ {0};
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_SAMPLE_H
