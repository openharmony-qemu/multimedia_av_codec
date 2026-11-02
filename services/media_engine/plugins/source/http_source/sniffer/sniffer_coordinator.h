/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef HISTREAMER_SNIFFER_COORDINATOR_H
#define HISTREAMER_SNIFFER_COORDINATOR_H

#include <array>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include "download/downloader.h"
#include "sniffer/sniff_result.h"

namespace OHOS::Media::Plugins::HttpPlugin {
enum class ProbeFailureKind {
    NONE,
    NETWORK,
    TIMEOUT,
    CANCELLED,
    CONTENT,
    LOCAL_INPUT,
    INTERNAL
};
struct ProbeFailure {
    ProbeFailureKind kind {ProbeFailureKind::NONE};
    int32_t clientCode {0};
    int32_t serverCode {0};
    uint64_t routeId {0};
};
// Owns a disposable request. No transport or bytes are transferred to media downloaders.
class SnifferCoordinator : public std::enable_shared_from_this<SnifferCoordinator> {
public:
    using DownloaderFactory = std::function<std::shared_ptr<Downloader>()>;
    explicit SnifferCoordinator(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = nullptr,
        DownloaderFactory downloaderFactory = {}, uint64_t routeId = 0);
    ~SnifferCoordinator();
    // UID -1 retains the network backend's default application identity.
    void Start(const std::string& url, const std::map<std::string, std::string>& headers,
        bool localManifest = false, int32_t appUid = -1, int32_t timeoutMs = SNIFF_TIMEOUT_MS);
    bool WaitResult(std::shared_ptr<SniffResult>& out, int32_t timeoutMs = SNIFF_TIMEOUT_MS);
    void Stop();
    ProbeFailure GetFailure() const;
    uint64_t GetRouteId() const { return routeId_; }
    static bool IsRetryableCurlError(int32_t code);
    static constexpr size_t SNIFF_THRESHOLD = 16 * 1024;
    static constexpr int32_t SNIFF_TIMEOUT_MS = 5000;
    static constexpr int32_t MAX_SNIFF_RETRY = 3;

private:
    using Clock = std::chrono::steady_clock;
    enum class State {
        IDLE,
        PROBING,
        SUCCEEDED,
        TIMED_OUT,
        CANCELLED,
        FAILED
    };
    struct LogSnapshot {
        const char* state;
        const char* reason;
        uint32_t attempt;
        int32_t retries;
        int32_t redirects;
        size_t retained;
        int32_t client;
        int32_t server;
        int64_t elapsedMs;
        int64_t decisionMs;
        int64_t firstDataMs;
        int64_t cleanupMs;
        int64_t remainingMs;
        bool stopped;
        StreamProtocolType protocol;
        bool isFlv;
        bool sniffOk;
    };
    uint32_t OnData(uint8_t* data, uint32_t len);
    void MarkReady(bool isEos, int32_t error = 0, int32_t serverError = 0);
    std::shared_ptr<DownloadRequest> CreateRequest(const std::string& url);
    void StartAttempt(const std::string& url);
    bool RestartAttempt(std::shared_ptr<Downloader> previous);
    void ClassifyLocked(bool isEos);
    void ProbeFd(const std::string& url);
    void Finalize();
    uint32_t SaveDataLocked(uint8_t* data, uint32_t len);
    bool CanReadFd();
    bool SelectNextAttemptLocked();
    bool IsRetryableClientError(int32_t code) const;
    void SetStateLocked(State state, const char* reason, ProbeFailureKind kind = ProbeFailureKind::INTERNAL);
    void LogEvent(const char* event, bool debug = false);
    static const char* StateName(State state);
    static const char* ProtocolName(StreamProtocolType type);

    const uint64_t routeId_;
    Clock::time_point startedAt_ {Clock::now()};
    Clock::time_point decisionAt_ {};
    Clock::time_point firstDataAt_ {};
    const char* reason_ {"created"}; // Diagnostic text, never used to drive state transitions.
    uint32_t attempt_ {0};
    int32_t retries_ {0};
    int32_t redirects_ {0};
    int64_t cleanupMs_ {0};

    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_;
    DownloaderFactory downloaderFactory_;
    std::shared_ptr<Downloader> sniffer_;
    std::shared_ptr<SniffResult> result_;
    std::map<std::string, std::string> headers_;
    int32_t appUid_ {-1};
    std::array<uint8_t, SNIFF_THRESHOLD> buffer_ {};
    size_t size_ {0};
    Clock::time_point deadline_ {};
    State state_ {State::IDLE};
    bool stopped_ {false};
    bool localManifest_ {false};
    ProbeFailure failure_ {};
    bool responseReady_ {false};
    bool finalized_ {false};
    int32_t lastError_ {0};
    int32_t lastServerError_ {0};
    mutable std::mutex readyMutex_;
    std::condition_variable readyCv_;
    std::mutex finalizeMutex_;
};
} // namespace OHOS::Media::Plugins::HttpPlugin
#endif
