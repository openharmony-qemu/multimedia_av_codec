/*
 * Helper for UT to control av_read_frame output order while keeping other FFmpeg
 * behaviors unchanged.
 */
#ifndef FFMPEG_AVREADFRAME_MOCK_HELPER_H
#define FFMPEG_AVREADFRAME_MOCK_HELPER_H

#include <atomic>
#include <deque>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>
extern "C" {
#include "libavformat/avformat.h"
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {

class AvReadFrameMockController {
public:
    AvReadFrameMockController() = default;
    void SetLogEnabled(bool on);
    bool IsLogEnabled() const;
    void Enable(bool on);
    void SetOrder(const std::vector<int>& order);
    void Reset();
    void PreloadFromReal(AVFormatContext* ctx, int count); // count<0 => until EOF
    void SetRealReadFrame(int (*fn)(AVFormatContext*, AVPacket*));
    int MockReadFrame(AVFormatContext* ctx, AVPacket* pkt);
    uint64_t GetReadCount() const;
    void ResetReadCount();

private:
    using PacketPtr = std::shared_ptr<AVPacket>;
    PacketPtr ClonePacket(const AVPacket* src);
    PacketPtr AllocPacket();
    int PopTargetStream();
    PacketPtr FetchFromCacheLocked(int streamId);
    void CacheFrame(PacketPtr&& frame);
    int ReadRealOnce(AVFormatContext* ctx, AVPacket* pkt);
    int ReadUntilHitTarget(AVFormatContext* ctx, AVPacket* outPkt, int targetStream);

    std::unordered_map<int, std::deque<PacketPtr>> cache_;
    std::queue<int> order_;
    std::mutex mtx_;
    std::atomic<bool> enabled_{false};
    std::atomic<bool> logEnabled_{false};
    std::atomic<uint64_t> readCount_{0};
    int (*real_read_frame_)(AVFormatContext*, AVPacket*) {nullptr};
};

} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // FFMPEG_AVREADFRAME_MOCK_HELPER_H

