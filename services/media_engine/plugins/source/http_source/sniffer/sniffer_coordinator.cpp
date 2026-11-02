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
#define HST_LOG_TAG "SnifferCoordinator"
#include "sniffer/sniffer_coordinator.h"
#include <algorithm>
#include <cerrno>
#include <unistd.h>
#include "avcodec_dfx_component.h"
#include "common/log.h"
#include "common/media_source.h"
#include "utils/fd_source.h"

namespace OHOS::Media::Plugins::HttpPlugin {
namespace {
constexpr int32_t WRITE_ERROR = 23;
constexpr int32_t MAX_REDIRECTS = 8;
constexpr int32_t MIN_SNIFF_TIMEOUT_MS = 1;
constexpr int32_t REDIRECT_STATUS = 302; // Existing Downloader normalizes explicit redirect notifications to 302.
constexpr int32_t RESOLVE_HOST_ERROR = 6;
constexpr int32_t CONNECT_ERROR = 7;
constexpr int32_t TLS_CONNECT_ERROR = 35;
constexpr int32_t SEND_ERROR = 55;
constexpr int32_t RECEIVE_ERROR = 56;
constexpr int32_t QUEUE_WAIT_FOREVER = -1;
constexpr int32_t UNKNOWN_CLIENT_ERROR = -1;
constexpr char SNIFFER_TASK_NAME[] = "sniffer";
constexpr int64_t TIME_NOT_RECORDED = -1;
}

SnifferCoordinator::SnifferCoordinator(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader,
    DownloaderFactory downloaderFactory, uint64_t routeId)
    : routeId_(routeId != 0 ? routeId : MediaAVCodec::AllocateUniqueId()),
      sourceLoader_(std::move(sourceLoader)), downloaderFactory_(std::move(downloaderFactory))
{
    if (sourceLoader_ != nullptr && sourceLoader_->loader_ == nullptr) {
        sourceLoader_.reset();
    }
}

const char* SnifferCoordinator::StateName(State state)
{
    switch (state) {
        case State::IDLE:
            return "Idle";
        case State::PROBING:
            return "Probing";
        case State::SUCCEEDED:
            return "Succeeded";
        case State::TIMED_OUT:
            return "TimedOut";
        case State::CANCELLED:
            return "Cancelled";
        default:
            return "Failed";
    }
}

const char* SnifferCoordinator::ProtocolName(StreamProtocolType type)
{
    switch (type) {
        case StreamProtocolType::HLS:
            return "HLS";
        case StreamProtocolType::DASH:
            return "DASH";
        default:
            return "HTTP";
    }
}

void SnifferCoordinator::SetStateLocked(State state, const char* reason, ProbeFailureKind kind)
{
    state_ = state;
    reason_ = reason;
    if (state != State::IDLE && state != State::PROBING && decisionAt_ == Clock::time_point {}) {
        decisionAt_ = Clock::now();
        if (state == State::TIMED_OUT) {
            kind = ProbeFailureKind::TIMEOUT;
        } else if (state == State::CANCELLED) {
            kind = ProbeFailureKind::CANCELLED;
        } else if (state == State::SUCCEEDED) {
            kind = ProbeFailureKind::NONE;
        } else if (localManifest_ && kind == ProbeFailureKind::NETWORK) {
            kind = ProbeFailureKind::LOCAL_INPUT;
        }
        // Capture the decision before ForceClose can replace transport diagnostics with 23/42.
        failure_ = {kind, lastError_, lastServerError_, routeId_};
    }
}

ProbeFailure SnifferCoordinator::GetFailure() const
{
    std::lock_guard<std::mutex> lock(readyMutex_);
    return failure_;
}

void SnifferCoordinator::LogEvent(const char* event, bool debug)
{
    LogSnapshot value {};
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        auto now = Clock::now();
        auto elapsed = [this](Clock::time_point time) {
            return time == Clock::time_point {} ? TIME_NOT_RECORDED :
                std::chrono::duration_cast<std::chrono::milliseconds>(time - startedAt_).count();
        };
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - now).count();
        value = {StateName(state_), reason_, attempt_, retries_, redirects_, size_, lastError_, lastServerError_,
            elapsed(now), elapsed(decisionAt_), elapsed(firstDataAt_), cleanupMs_, std::max<int64_t>(remaining, 0),
            stopped_, result_ != nullptr ? result_->type : StreamProtocolType::HTTP,
            result_ != nullptr && result_->isFlv, result_ != nullptr && result_->sniffOk};
    }
    if (debug) {
        MEDIA_LOG_D("route=%{public}" PRIu64 " phase=probe event=%{public}s state=%{public}s reason=%{public}s",
            routeId_, event, value.state, value.reason);
        return;
    }
    MEDIA_LOG_I("route=%{public}" PRIu64 " phase=probe event=%{public}s state=%{public}s reason=%{public}s "
        "attempt=%{public}u retry=%{public}d redirect=%{public}d retained=%{public}zu "
        "client=%{public}d server=%{public}d protocol=%{public}s flv=%{public}d sniffOk=%{public}d "
        " elapsedMs=%{public}" PRId64 " decisionMs=%{public}" PRId64 " firstDataMs=%{public}" PRId64
        " cleanupMs=%{public}" PRId64 " remainingMs=%{public}" PRId64 " stopped=%{public}d",
        routeId_, event, value.state, value.reason, value.attempt, value.retries, value.redirects, value.retained,
        value.client, value.server, ProtocolName(value.protocol), value.isFlv, value.sniffOk,
        value.elapsedMs, value.decisionMs, value.firstDataMs, value.cleanupMs, value.remainingMs, value.stopped);
}

SnifferCoordinator::~SnifferCoordinator()
{
    Stop();
    Finalize();
}

void SnifferCoordinator::Start(const std::string& url, const std::map<std::string, std::string>& headers,
    bool localManifest, int32_t appUid, int32_t timeoutMs)
{
    bool cancelled;
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        if (result_ != nullptr) {
            return;
        }
        result_ = std::make_shared<SniffResult>();
        startedAt_ = Clock::now();
        decisionAt_ = {};
        deadline_ = Clock::now() +
            std::chrono::milliseconds(std::max(MIN_SNIFF_TIMEOUT_MS, std::min(timeoutMs, SNIFF_TIMEOUT_MS)));
        headers_ = headers;
        appUid_ = appUid;
        localManifest_ = localManifest;
        SetStateLocked(stopped_ ? State::CANCELLED : State::PROBING, stopped_ ? "already_stopped" : "start");
        cancelled = stopped_;
    }
    MEDIA_LOG_I("route=%{public}" PRIu64 " event=probe_start local=%{public}d backend=%{public}s "
        "budgetMs=%{public}d target=%{public}zu capacity=%{public}zu", routeId_, localManifest,
        localManifest ? "fd" : (downloaderFactory_ ? "injected" :
            (sourceLoader_ != nullptr ? "app_loader" : "default_network")),
        std::max(MIN_SNIFF_TIMEOUT_MS, std::min(timeoutMs, SNIFF_TIMEOUT_MS)),
        MIN_SNIFF_BUFFER_SIZE, SNIFF_THRESHOLD);
    LogEvent("start");
    if (cancelled) {
        return;
    }
    if (localManifest) {
        ProbeFd(url);
    } else {
        StartAttempt(url);
    }
}

std::shared_ptr<DownloadRequest> SnifferCoordinator::CreateRequest(const std::string& url)
{
    auto weakSelf = weak_from_this();
    DataSaveFunc save = [weakSelf](uint8_t* data, uint32_t len, bool) -> uint32_t {
        auto self = weakSelf.lock();
        return self != nullptr ? self->OnData(data, len) : 0;
    };
    StatusCallbackFunc status = [weakSelf](DownloadStatus, std::shared_ptr<Downloader>&,
        std::shared_ptr<DownloadRequest>& request) {
        auto self = weakSelf.lock();
        if (self != nullptr) {
            self->MarkReady(false, request != nullptr ? request->GetClientError() : UNKNOWN_CLIENT_ERROR,
                request != nullptr ? request->GetServerError() : 0);
        }
    };
    RequestInfo info;
    info.url = url;
    info.httpHeader = headers_;
    info.timeoutMs = SNIFF_TIMEOUT_MS;
    auto request = std::make_shared<DownloadRequest>(save, status, info, true);
    request->SetDownloadDoneCb([weakSelf](const std::string&, const std::string&) {
        auto self = weakSelf.lock();
        if (self != nullptr) {
            self->MarkReady(true);
        }
    });
    return request;
}

void SnifferCoordinator::StartAttempt(const std::string& url)
{
    auto attempt = downloaderFactory_ ? downloaderFactory_() :
        std::make_shared<Downloader>(SNIFFER_TASK_NAME, sourceLoader_);
    attempt->Init();
    attempt->SetAppUid(appUid_);
    auto request = CreateRequest(url);
    bool started = false;
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        if (state_ == State::PROBING && Clock::now() < deadline_) {
            ++attempt_;
            firstDataAt_ = {};
            sniffer_ = attempt;
            started = attempt->Download(request, QUEUE_WAIT_FOREVER);
            if (started) {
                attempt->Start();
            } else {
                SetStateLocked(State::FAILED, "enqueue_failed");
            }
        } else if (state_ == State::PROBING) {
            SetStateLocked(State::TIMED_OUT, "deadline_before_attempt");
        }
    }
    MEDIA_LOG_I("route=%{public}" PRIu64 " event=attempt_created downloader=%{public}" PRIu64
        " started=%{public}d", routeId_, attempt->GetUniqueId(), started);
    LogEvent("attempt_start");
    if (!started) {
        attempt->ForceClose();
        readyCv_.notify_all();
    }
}

void SnifferCoordinator::ClassifyLocked(bool isEos)
{
    if (state_ != State::PROBING) {
        return;
    }
    if (Clock::now() >= deadline_) {
        SetStateLocked(State::TIMED_OUT, "deadline_before_classification");
        return;
    }
    if (size_ == 0) {
        if (isEos) {
            SetStateLocked(State::FAILED, "empty_eof", ProbeFailureKind::CONTENT);
        }
        return;
    }
    auto probe = StreamProtocolSniffer::Probe(buffer_.data(), size_, isEos);
    if (Clock::now() >= deadline_) {
        SetStateLocked(State::TIMED_OUT, "deadline_after_classification");
        return;
    }
    if (probe.needMore) {
        if (isEos) {
            SetStateLocked(State::FAILED, size_ == buffer_.size() ? "undecidable_capacity" : "truncated_eof",
                ProbeFailureKind::CONTENT);
        }
        return;
    }
    result_->type = probe.type;
    result_->isFlv = probe.isFlv;
    result_->sniffOk = true;
    SetStateLocked(State::SUCCEEDED, size_ == buffer_.size() ? "capacity" : (isEos ? "eof" : "prefix"));
}

uint32_t SnifferCoordinator::OnData(uint8_t* data, uint32_t len)
{
    uint32_t received;
    bool first;
    bool changed;
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        auto previous = state_;
        first = state_ == State::PROBING && data != nullptr && len > 0 && firstDataAt_ == Clock::time_point {};
        if (first) {
            firstDataAt_ = Clock::now();
        }
        received = SaveDataLocked(data, len);
        changed = state_ != previous;
    }
    if (first) {
        MEDIA_LOG_D("route=%{public}" PRIu64 " event=first_data callbackBytes=%{public}u", routeId_, len);
    }
    if (changed) {
        LogEvent("data_decision");
    }
    return received;
}

uint32_t SnifferCoordinator::SaveDataLocked(uint8_t* data, uint32_t len)
{
    if (state_ != State::PROBING || data == nullptr) {
        return 0;
    }
    if (Clock::now() >= deadline_) {
        SetStateLocked(State::TIMED_OUT, "deadline_on_data");
        readyCv_.notify_all();
        return 0;
    }
    size_t consumed = 0;
    while (consumed < len && size_ < buffer_.size() && state_ == State::PROBING) {
        size_t target = std::min(buffer_.size(), size_ + MIN_SNIFF_BUFFER_SIZE);
        size_t count = std::min(static_cast<size_t>(len) - consumed, target - size_);
        std::copy_n(data + consumed, count, buffer_.data() + size_);
        size_ += count;
        consumed += count;
        ClassifyLocked(size_ == buffer_.size());
    }
    if (state_ != State::PROBING) {
        // Publish the reason BEFORE the library reports response error 23 for this short write.
        readyCv_.notify_all();
        return 0;
    }
    return len;
}

void SnifferCoordinator::MarkReady(bool isEos, int32_t error, int32_t serverError)
{
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        lastError_ = error;
        lastServerError_ = serverError;
        // Keep the raw response code, but never overwrite a published terminal outcome.
        if (state_ == State::PROBING) {
            if (Clock::now() >= deadline_) {
                SetStateLocked(State::TIMED_OUT, "deadline_on_response");
            } else if (serverError == REDIRECT_STATUS) {
                // Only interpret Downloader's redirect notification; leave transport behavior unchanged.
                responseReady_ = true;
            } else if (isEos) {
                ClassifyLocked(true);
            } else if (error == WRITE_ERROR) {
                SetStateLocked(State::FAILED, "unexpected_write_error", ProbeFailureKind::NETWORK);
            } else {
                responseReady_ = true;
            }
        }
    }
    LogEvent("response");
    readyCv_.notify_all();
}

bool SnifferCoordinator::CanReadFd()
{
    bool allowed;
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        if (state_ == State::PROBING && Clock::now() >= deadline_) {
            SetStateLocked(State::TIMED_OUT, "deadline_on_fd");
        }
        allowed = state_ == State::PROBING;
    }
    if (!allowed) {
        readyCv_.notify_all();
        LogEvent("fd_stopped");
    }
    return allowed;
}

void SnifferCoordinator::ProbeFd(const std::string& url)
{
    FdSource source;
    if (!FdSource::Parse(url, source)) {
        MEDIA_LOG_W("route=%{public}" PRIu64 " event=fd_parse_failed", routeId_);
        MarkReady(false, UNKNOWN_CLIENT_ERROR);
        return;
    }
    MEDIA_LOG_I("route=%{public}" PRIu64 " event=fd_range fd=%{public}d offset=%{public}" PRId64
        " size=%{public}" PRId64, routeId_, source.fd, source.offset, source.size);
    size_t offset = 0;
    while (offset < static_cast<uint64_t>(source.size)) {
        if (!CanReadFd()) {
            return;
        }
        size_t count = std::min<size_t>(MIN_SNIFF_BUFFER_SIZE, source.size - offset);
        std::array<uint8_t, MIN_SNIFF_BUFFER_SIZE> chunk {};
        auto n = pread(source.fd, chunk.data(), count, source.offset + offset);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            MEDIA_LOG_I("route=%{public}" PRIu64 " event=fd_read_end eos=%{public}d errno=%{public}d",
                routeId_, n == 0, n < 0 ? errno : 0);
            MarkReady(n == 0, n < 0 ? UNKNOWN_CLIENT_ERROR : 0);
            return;
        }
        offset += static_cast<size_t>(n);
        if (OnData(chunk.data(), static_cast<uint32_t>(n)) == 0) {
            return;
        }
    }
    MarkReady(true);
}

bool SnifferCoordinator::WaitResult(std::shared_ptr<SniffResult>& out, int32_t timeoutMs)
{
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        if (result_ == nullptr) {
            out.reset();
            return false;
        }
        deadline_ = std::min(deadline_, Clock::now() + std::chrono::milliseconds(std::max(timeoutMs, 0)));
    }
    LogEvent("wait_begin");
    std::shared_ptr<Downloader> previous;
    do {
        {
            std::unique_lock<std::mutex> lock(readyMutex_);
            readyCv_.wait_until(lock, deadline_, [this] { return state_ != State::PROBING || responseReady_; });
            if (state_ != State::PROBING) {
                break;
            }
            if (!SelectNextAttemptLocked()) {
                break;
            }
            previous = sniffer_;
        }
    } while (RestartAttempt(std::move(previous)));
    Finalize();
    LogEvent("wait_end");
    std::lock_guard<std::mutex> lock(readyMutex_);
    out = result_;
    return state_ == State::SUCCEEDED && !stopped_;
}

bool SnifferCoordinator::SelectNextAttemptLocked()
{
    if (Clock::now() >= deadline_) {
        SetStateLocked(State::TIMED_OUT, "deadline_while_waiting");
        return false;
    }
    bool redirect = lastServerError_ == REDIRECT_STATUS;
    if (redirect && redirects_ < MAX_REDIRECTS && sniffer_ != nullptr) {
        ++redirects_;
        reason_ = "explicit_redirect";
        return true;
    }
    if (!redirect && IsRetryableClientError(lastError_) && retries_ < MAX_SNIFF_RETRY && sniffer_ != nullptr) {
        ++retries_;
        reason_ = sourceLoader_ != nullptr ? "app_not_ready_retry" : "network_retry";
        return true;
    }
    SetStateLocked(State::FAILED, redirect ? "redirect_limit" : "no_retry_or_limit", ProbeFailureKind::NETWORK);
    return false;
}

bool SnifferCoordinator::RestartAttempt(std::shared_ptr<Downloader> previous)
{
    LogEvent("restart_close_begin");
    // Quiesce the old attempt outside the state lock before reusing any prefix state.
    previous->ForceClose();
    LogEvent("restart_close_end");
    const auto& request = previous->GetCurrentRequest();
    std::string nextUrl = request != nullptr ? request->GetUrl() : std::string();
    std::string location;
    if (request != nullptr) {
        request->GetLocation(location);
    }
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        sniffer_.reset();
        if (state_ != State::PROBING) {
            return false;
        }
        if (Clock::now() >= deadline_) {
            SetStateLocked(State::TIMED_OUT, "deadline_after_close");
            return false;
        }
        if (lastServerError_ == REDIRECT_STATUS && (nextUrl.empty() || location.empty())) {
            SetStateLocked(State::FAILED, "missing_redirect_target", ProbeFailureKind::NETWORK);
            return false;
        }
        size_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0);
        responseReady_ = false;
    }
    previous.reset();
    StartAttempt(nextUrl);
    return true;
}

void SnifferCoordinator::Finalize()
{
    std::unique_lock<std::mutex> cleanupLock(finalizeMutex_);
    if (finalized_) {
        return;
    }
    auto started = Clock::now();
    LogEvent("cleanup_begin");
    if (sniffer_ != nullptr) {
        sniffer_->ForceClose();
        auto request = sniffer_->GetCurrentRequest();
        if (request != nullptr) {
            std::lock_guard<std::mutex> lock(readyMutex_);
            lastError_ = request->GetClientError();
            lastServerError_ = request->GetServerError();
        }
        sniffer_.reset();
    }
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        if (result_ != nullptr) {
            result_->bytesProbed = size_;
        }
        std::fill(buffer_.begin(), buffer_.end(), 0);
        cleanupMs_ = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count();
    }
    finalized_ = true;
    cleanupLock.unlock();
    LogEvent("cleanup_end");
}

bool SnifferCoordinator::IsRetryableClientError(int32_t code) const
{
    if (sourceLoader_ != nullptr) {
        // AppClient reports LoadingRequestError as its negated integer value.
        return code == -static_cast<int32_t>(LoadingRequestError::LOADING_ERROR_NOT_READY);
    }
    return IsRetryableCurlError(code);
}

bool SnifferCoordinator::IsRetryableCurlError(int32_t code)
{
    return code == RESOLVE_HOST_ERROR || code == CONNECT_ERROR || code == TLS_CONNECT_ERROR ||
        code == SEND_ERROR || code == RECEIVE_ERROR;
}

void SnifferCoordinator::Stop()
{
    bool repeated;
    {
        std::lock_guard<std::mutex> lock(readyMutex_);
        repeated = stopped_;
        stopped_ = true;
        if (state_ == State::IDLE || state_ == State::PROBING) {
            SetStateLocked(State::CANCELLED, "stop_requested");
        }
    }
    LogEvent("stop", repeated);
    readyCv_.notify_all();
}
} // namespace OHOS::Media::Plugins::HttpPlugin
