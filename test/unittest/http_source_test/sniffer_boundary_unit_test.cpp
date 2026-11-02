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
#include <algorithm>
#include <fcntl.h>
#include <vector>
#include "gtest/gtest.h"
#include "sniffer_test_support.h"
#include "utils/fd_source.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
using namespace SnifferTest;
class SnifferBoundaryUnitTest : public testing::Test {};

namespace {
std::string XmlAt(size_t delimiter)
{
    const std::string beginning = "<!--";
    const std::string ending = "--><MPD>";
    return beginning + std::string(delimiter - beginning.size() - ending.size(), 'x') + ending;
}

void Feed(const std::shared_ptr<SnifferCoordinator>& probe, std::string& bytes, size_t chunk)
{
    for (size_t offset = 0; offset < bytes.size();) {
        auto count = std::min(chunk, bytes.size() - offset);
        if (probe->OnData(reinterpret_cast<uint8_t*>(bytes.data() + offset), count) == 0) {
            return;
        }
        offset += count;
    }
    probe->MarkReady(true);
}
} // namespace

HWTEST_F(SnifferBoundaryUnitTest, XmlDelimiterAndCallbackBoundaries_001, TestSize.Level2)
{
    // Positions are one-based; the root delimiter is the last byte of each complete XML prefix.
    const std::vector<size_t> positions {255, 256, 257, 16383, 16384, 16385};
    const std::vector<size_t> chunks {1, 17, 256, 16384, 65536}; // Include bytewise and oversized callbacks.
    std::vector<bool> outcomes;
    std::vector<bool> expected;
    for (size_t position : positions) {
        for (size_t chunk : chunks) {
            auto probe = Pending();
            auto bytes = XmlAt(position);
            Feed(probe, bytes, chunk);
            std::shared_ptr<SniffResult> result;
            bool ok = probe->WaitResult(result);
            outcomes.push_back(ok);
            expected.push_back(position <= SnifferCoordinator::SNIFF_THRESHOLD);
            ASSERT_NE(result, nullptr);
            EXPECT_EQ(result->bytesProbed, std::min(position, SnifferCoordinator::SNIFF_THRESHOLD));
            EXPECT_EQ(result->type, ok ? StreamProtocolType::DASH : StreamProtocolType::HTTP);
        }
    }
    EXPECT_EQ(outcomes, expected);
}

HWTEST_F(SnifferBoundaryUnitTest, TruncatedSignaturesAtEof_002, TestSize.Level2)
{
    const std::vector<std::string> prefixes {"#EXTM", "FLV", "<MPD", "<!-- unfinished", "<?xml"};
    std::vector<bool> results;
    for (auto bytes : prefixes) {
        auto probe = Pending();
        EXPECT_EQ(probe->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size()), bytes.size());
        probe->MarkReady(true);
        std::shared_ptr<SniffResult> result;
        results.push_back(probe->WaitResult(result));
        ASSERT_NE(result, nullptr);
        EXPECT_FALSE(result->sniffOk);
        EXPECT_EQ(result->bytesProbed, bytes.size());
    }
    EXPECT_EQ(results, std::vector<bool>(prefixes.size(), false));
}

HWTEST_F(SnifferBoundaryUnitTest, NullAndZeroLengthThenEmptyEof_003, TestSize.Level2)
{
    auto probe = Pending();
    uint8_t byte = 'x';
    EXPECT_EQ(probe->OnData(nullptr, sizeof(byte)), 0u);
    EXPECT_EQ(probe->OnData(&byte, 0), 0u);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::PROBING);
    probe->MarkReady(true);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    EXPECT_EQ(result->bytesProbed, 0u);
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::FAILED);
}

HWTEST_F(SnifferBoundaryUnitTest, ShortOrdinaryBodyCanFinishOnExplicitEof_004, TestSize.Level2)
{
    auto probe = Pending();
    std::string bytes = "abc";
    EXPECT_EQ(probe->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size()), bytes.size());
    EXPECT_EQ(probe->state_, SnifferCoordinator::State::PROBING);
    probe->MarkReady(true);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(probe->WaitResult(result));
    EXPECT_EQ(result->type, StreamProtocolType::HTTP);
    EXPECT_EQ(result->bytesProbed, bytes.size());
}

HWTEST_F(SnifferBoundaryUnitTest, OversizedSingleCallbackStopsAndClearsStorage_005, TestSize.Level2)
{
    auto probe = Pending();
    std::string bytes(SnifferCoordinator::SNIFF_THRESHOLD * 4, 'x'); // Four times the storage capacity.
    EXPECT_EQ(probe->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size()), 0u);
    std::shared_ptr<SniffResult> result;
    ASSERT_TRUE(probe->WaitResult(result));
    EXPECT_EQ(result->bytesProbed, MIN_SNIFF_BUFFER_SIZE);
    EXPECT_EQ(probe->OnData(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size()), 0u);
    EXPECT_EQ(probe->size_, MIN_SNIFF_BUFFER_SIZE);
    EXPECT_TRUE(std::all_of(probe->buffer_.begin(), probe->buffer_.end(), [](uint8_t byte) { return byte == 0; }));
}

HWTEST_F(SnifferBoundaryUnitTest, FdRangeFallbacksAndProbeFailures_008, TestSize.Level2)
{
    TempFile file(HLS);
    ASSERT_EQ(file.written, static_cast<ssize_t>(sizeof(HLS) - 1));
    const int64_t length = sizeof(HLS) - 1;
    struct Case { std::string query; int64_t offset; int64_t size; };
    const std::vector<Case> cases {
        {"?offset=0&size=-1", 0, 0}, {"?offset=abc&size=xyz", 0, 0},
        {"?offset=-1&size=99", length, 0}, {"?offset=999999&size=99", length, 0},
        {"?offset=0&size=999999", 0, length}};
    std::vector<int64_t> observed;
    for (const auto& item : cases) {
        FdSource source;
        ASSERT_TRUE(source.Parse(file.Uri() + item.query, source));
        EXPECT_EQ(source.offset, item.offset);
        EXPECT_EQ(source.size, item.size);
        auto probe = std::make_shared<SnifferCoordinator>();
        probe->Start(file.Uri() + item.query, {}, true);
        std::shared_ptr<SniffResult> result;
        EXPECT_EQ(probe->WaitResult(result), item.size != 0);
        EXPECT_EQ(result->bytesProbed, static_cast<size_t>(item.size));
        observed.push_back(lseek(file.fd, 0, SEEK_CUR));
    }
    EXPECT_EQ(observed, std::vector<int64_t>(cases.size(), length));
}

HWTEST_F(SnifferBoundaryUnitTest, InvalidFdKindsDoNotLeakOwnership_009, TestSize.Level2)
{
    TempFile file(HLS);
    ASSERT_EQ(file.written, static_cast<ssize_t>(sizeof(HLS) - 1));
    int pipes[2] {-1, -1};
    ASSERT_EQ(pipe(pipes), 0);
    int directory = open("/data/test/media", O_RDONLY | O_DIRECTORY);
    ASSERT_NE(directory, -1);
    int closed = dup(file.fd);
    ASSERT_NE(closed, -1);
    close(closed);
    const std::vector<std::string> uris {
        "fd://-1", "fd://999999999999", "fd://" + std::to_string(closed),
        "fd://" + std::to_string(pipes[0]), "fd://" + std::to_string(directory)};
    std::vector<bool> results;
    for (const auto& uri : uris) {
        auto probe = std::make_shared<SnifferCoordinator>();
        probe->Start(uri, {}, true);
        std::shared_ptr<SniffResult> result;
        results.push_back(probe->WaitResult(result));
        EXPECT_EQ(probe->sniffer_, nullptr);
    }
    close(pipes[0]);
    close(pipes[1]);
    close(directory);
    EXPECT_EQ(results, std::vector<bool>(uris.size(), false));
}

HWTEST_F(SnifferBoundaryUnitTest, PreadFailureDoesNotCloseBorrowedFd_010, TestSize.Level2)
{
    TempFile file(HLS);
    ASSERT_EQ(file.written, static_cast<ssize_t>(sizeof(HLS) - 1));
    ASSERT_TRUE(file.MakeWriteOnly());
    auto probe = std::make_shared<SnifferCoordinator>();
    probe->Start(file.Uri(), {}, true);
    std::shared_ptr<SniffResult> result;
    EXPECT_FALSE(probe->WaitResult(result));
    EXPECT_EQ(result->bytesProbed, 0u);
    EXPECT_EQ(lseek(file.fd, 0, SEEK_CUR), 0);
    EXPECT_NE(fcntl(file.fd, F_GETFD), -1);
}
} // namespace OHOS::Media::Plugins::HttpPlugin
