/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/*
 * Implementation for av_read_frame mock controller used in demuxer UTs.
 */
#include "ffmpeg_avreadframe_mock_helper.h"
#include <iostream>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {

namespace {
struct AvPacketDeleter {
    void operator()(AVPacket* pkt) const
    {
        if (pkt != nullptr) {
            av_packet_free(&pkt);
        }
    }
};
} // namespace

void AvReadFrameMockController::SetLogEnabled(bool on)
{
    logEnabled_.store(on, std::memory_order_release);
}

bool AvReadFrameMockController::IsLogEnabled() const
{
    return logEnabled_.load(std::memory_order_acquire);
}

void AvReadFrameMockController::Enable(bool on)
{
    enabled_.store(on, std::memory_order_release);
    if (IsLogEnabled()) {
        std::cout << "[MOCK] Enable=" << (on ? 1 : 0) << std::endl;
    }
}

void AvReadFrameMockController::SetOrder(const std::vector<int>& order)
{
    std::lock_guard<std::mutex> lock(mtx_);
    std::queue<int> empty;
    std::swap(order_, empty);
    if (IsLogEnabled()) {
        std::cout << "[MOCK] SetOrder size=" << order.size() << " : ";
    }
    for (auto id : order) {
        order_.push(id);
        if (IsLogEnabled()) {
            std::cout << id << " ";
        }
    }
    if (IsLogEnabled()) {
        std::cout << std::endl;
    }
}

void AvReadFrameMockController::Reset()
{
    std::lock_guard<std::mutex> lock(mtx_);
    cache_.clear();
    std::queue<int> empty;
    std::swap(order_, empty);
    enabled_.store(false, std::memory_order_release);
    readCount_.store(0, std::memory_order_release);
    if (IsLogEnabled()) {
        std::cout << "[MOCK] Reset" << std::endl;
    }
}

AvReadFrameMockController::PacketPtr AvReadFrameMockController::AllocPacket()
{
    return PacketPtr(av_packet_alloc(), AvPacketDeleter());
}

AvReadFrameMockController::PacketPtr AvReadFrameMockController::ClonePacket(const AVPacket* src)
{
    AVPacket* clone = av_packet_clone(src);
    return PacketPtr(clone, AvPacketDeleter());
}

void AvReadFrameMockController::PreloadFromReal(AVFormatContext* ctx, int count)
{
    if (real_read_frame_ == nullptr || ctx == nullptr) {
        return;
    }
    int remain = count;
    while (remain != 0) {
        auto pkt = AllocPacket();
        if (!pkt) {
            break;
        }
        int ret = real_read_frame_(ctx, pkt.get());
        if (ret < 0) {
            break;
        }
        int streamId = pkt->stream_index;
        auto cloned = ClonePacket(pkt.get());
        if (!cloned) {
            break;
        }
        {
            std::lock_guard<std::mutex> lock(mtx_);
            cache_[streamId].emplace_back(std::move(cloned));
        }
        if (count > 0) {
            remain--;
        }
    }
}

int AvReadFrameMockController::ReadRealOnce(AVFormatContext* ctx, AVPacket* pkt)
{
    if (real_read_frame_ == nullptr || ctx == nullptr || pkt == nullptr) {
        return AVERROR(EINVAL);
    }
    int ret = real_read_frame_(ctx, pkt);
    if (ret >= 0) {
        readCount_.fetch_add(1, std::memory_order_acq_rel);
    }
    return ret;
}

int AvReadFrameMockController::PopTargetStream()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (order_.empty()) {
        if (IsLogEnabled()) {
            std::cout << "[MOCK] order empty -> real" << std::endl;
        }
        return -1;
    }
    int targetStream = order_.front();
    order_.pop();
    if (IsLogEnabled()) {
        std::cout << "[MOCK] pop target=" << targetStream << ", remain=" << order_.size() << std::endl;
    }
    return targetStream;
}

AvReadFrameMockController::PacketPtr AvReadFrameMockController::FetchFromCacheLocked(int streamId)
{
    auto iter = cache_.find(streamId);
    if (iter == cache_.end() || iter->second.empty()) {
        return nullptr;
    }
    PacketPtr p = iter->second.front();
    iter->second.pop_front();
    if (IsLogEnabled()) {
        std::cout << "[MOCK] cache hit stream=" << streamId <<
                  ", remain=" << iter->second.size() <<
                  std::endl;
    }
    return p;
}

void AvReadFrameMockController::CacheFrame(PacketPtr&& frame)
{
    if (!frame) {
        return;
    }
    const int sid = frame->stream_index; // must read before move
    std::lock_guard<std::mutex> lock(mtx_);
    cache_[sid].emplace_back(std::move(frame));
    if (IsLogEnabled()) {
        std::cout << "[MOCK] cache push stream=" << sid <<
                  ", size=" << cache_[sid].size() <<
                  std::endl;
    }
}

int AvReadFrameMockController::ReadUntilHitTarget(AVFormatContext* ctx, AVPacket* outPkt, int targetStream)
{
    if (IsLogEnabled()) {
        std::cout << "[MOCK] cache miss target=" << targetStream << ", read real until hit" << std::endl;
    }
    int readCount = 0;
    bool found = false;
    while (!found) {
        auto temp = AllocPacket();
        if (!temp) {
            return AVERROR(ENOMEM);
        }
        int ret = ReadRealOnce(ctx, temp.get());
        if (ret < 0) {
            if (IsLogEnabled()) {
                std::cout << "[MOCK] real read end ret=" << ret << std::endl;
            }
            return ret;
        }
        readCount++;
        if (temp->stream_index == targetStream) {
            av_packet_unref(outPkt);
            int refRet = av_packet_ref(outPkt, temp.get());
            if (IsLogEnabled()) {
                std::cout << "[MOCK] hit target=" << targetStream <<
                          " after reads=" << readCount <<
                          std::endl;
            }
            found = true;
            return refRet;
        }
        if (IsLogEnabled()) {
            std::cout << "[MOCK] read stream=" << temp->stream_index <<
                      " != target, cache it" << std::endl;
        }
        CacheFrame(ClonePacket(temp.get()));
    }
    return AVERROR(EINVAL); // should never reach here
}

int AvReadFrameMockController::MockReadFrame(AVFormatContext* ctx, AVPacket* pkt)
{
    if (ctx == nullptr || pkt == nullptr) {
        return AVERROR(EINVAL);
    }
    if (!enabled_.load(std::memory_order_acquire)) {
        int ret = ReadRealOnce(ctx, pkt);
        if (ret >= 0 && IsLogEnabled()) {
            std::cout << "[MOCK] disabled -> real, stream=" << pkt->stream_index << std::endl;
        }
        return ret;
    }

    const int targetStream = PopTargetStream();
    if (targetStream < 0) {
        int ret = ReadRealOnce(ctx, pkt);
        if (ret >= 0 && IsLogEnabled()) {
            std::cout << "[MOCK] no target -> real, stream=" << pkt->stream_index << std::endl;
        }
        return ret;
    }

    PacketPtr cachedPkt;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        cachedPkt = FetchFromCacheLocked(targetStream);
    }
    if (cachedPkt) {
        av_packet_unref(pkt);
        int refRet = av_packet_ref(pkt, cachedPkt.get());
        if (IsLogEnabled()) {
            std::cout << "[MOCK] return cached stream=" << targetStream << std::endl;
        }
        return refRet;
    }
    return ReadUntilHitTarget(ctx, pkt, targetStream);
}

uint64_t AvReadFrameMockController::GetReadCount() const
{
    return readCount_.load(std::memory_order_acquire);
}

void AvReadFrameMockController::ResetReadCount()
{
    readCount_.store(0, std::memory_order_release);
}

void AvReadFrameMockController::SetRealReadFrame(int (*fn)(AVFormatContext*, AVPacket*))
{
    real_read_frame_ = fn;
}

} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS

