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

#include "pts_and_index_conversion_test_utils.h"

#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace OHOS::Media::PtsAndIndexConversionTestUtils {
namespace {

constexpr size_t BYTE_BITS = 8;
constexpr uint8_t BYTE_MASK = 0xff;
constexpr size_t BYTE_INDEX_ONE = 1;
constexpr size_t BYTE_INDEX_TWO = 2;
constexpr size_t BYTE_INDEX_THREE = 3;
constexpr size_t UINT32_WORD_COUNT_TWO = 2;
constexpr size_t UINT32_WORD_COUNT_THREE = 3;
constexpr size_t UINT32_WORD_COUNT_FOUR = 4;
constexpr size_t CONTAINER_BOX_COUNT = 5;
constexpr size_t FTYP_PAYLOAD_SIZE = 8;
constexpr size_t FTYP_BRAND_OFFSET = sizeof(uint32_t) * UINT32_WORD_COUNT_TWO;
constexpr size_t HANDLER_TYPE_OFFSET = sizeof(uint32_t) * UINT32_WORD_COUNT_TWO;
constexpr size_t LARGE_BOX_HEADER_SIZE = sizeof(uint32_t) * UINT32_WORD_COUNT_FOUR;
constexpr size_t NORMAL_BOX_HEADER_SIZE = sizeof(uint32_t) * UINT32_WORD_COUNT_TWO;
constexpr size_t FTYP_BOX_SIZE = sizeof(uint32_t) * UINT32_WORD_COUNT_FOUR;
constexpr uint32_t LARGE_BOX_SIZE_MARKER = 1;
constexpr uint8_t MDHD_VERSION_ONE = 1;
constexpr uint32_t SAMPLE_COUNT = 2;
constexpr uint32_t DEFAULT_TIME_SCALE = 1000;
constexpr size_t MDHD_V0_PAYLOAD_SIZE = 20;
constexpr size_t MDHD_V1_PAYLOAD_SIZE = 32;
constexpr size_t MDHD_V0_TIME_SCALE_OFFSET = 12;
constexpr size_t MDHD_V1_TIME_SCALE_OFFSET = 20;
constexpr int CHILD_SETUP_FAILURE = 2;

constexpr uint64_t LARGE_BOX_PAYLOAD_SIZE = 1ULL << 30;
constexpr size_t LARGE_BOX_PAYLOAD_OFFSET = 112;
constexpr size_t LARGE_BOX_PREFIX_SIZE = LARGE_BOX_PAYLOAD_OFFSET + MDHD_V0_PAYLOAD_SIZE;
constexpr rlim_t CHILD_ADDRESS_SPACE_LIMIT = 256ULL * 1024ULL * 1024ULL;
constexpr rlim_t CHILD_STACK_LIMIT = 256ULL * 1024ULL;
constexpr size_t DEEP_CONTAINER_DEPTH = 4096;

void WriteBigEndian32(std::vector<uint8_t> &data, size_t offset, uint32_t value)
{
    data[offset] = static_cast<uint8_t>((value >> (BYTE_BITS * UINT32_WORD_COUNT_THREE)) & BYTE_MASK);
    data[offset + BYTE_INDEX_ONE] = static_cast<uint8_t>((value >> (BYTE_BITS * UINT32_WORD_COUNT_TWO)) & BYTE_MASK);
    data[offset + BYTE_INDEX_TWO] = static_cast<uint8_t>((value >> BYTE_BITS) & BYTE_MASK);
    data[offset + BYTE_INDEX_THREE] = static_cast<uint8_t>(value & BYTE_MASK);
}

void WriteBigEndian64(std::vector<uint8_t> &data, size_t offset, uint64_t value)
{
    for (size_t i = 0; i < sizeof(uint64_t); ++i) {
        data[offset + i] = static_cast<uint8_t>(value >> ((sizeof(uint64_t) - BYTE_INDEX_ONE - i) * BYTE_BITS));
    }
}

void WriteBoxType(std::vector<uint8_t> &data, size_t offset, const char *type)
{
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        data[offset + i] = static_cast<uint8_t>(type[i]);
    }
}

void WriteLargeBoxHeader(std::vector<uint8_t> &data, size_t offset, const char *type, uint64_t boxSize)
{
    WriteBigEndian32(data, offset, LARGE_BOX_SIZE_MARKER);
    WriteBoxType(data, offset + sizeof(uint32_t), type);
    WriteBigEndian64(data, offset + sizeof(uint32_t) * UINT32_WORD_COUNT_TWO, boxSize);
}

bool WriteAll(int fd, const uint8_t *data, size_t size)
{
    size_t written = 0;
    while (written < size) {
        ssize_t result = write(fd, data + written, size - written);
        if (result <= 0) {
            return false;
        }
        written += static_cast<size_t>(result);
    }
    return true;
}

bool CreateLargeSizeMedia(const char *targetType, uint64_t payloadSize, const std::string &path,
    int &fd, uint64_t &fileSize)
{
    fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }

    std::vector<uint8_t> prefix(LARGE_BOX_PREFIX_SIZE, 0);
    WriteBigEndian32(prefix, 0, FTYP_BOX_SIZE);
    WriteBoxType(prefix, sizeof(uint32_t), BOX_TYPE_FTYP);
    prefix[FTYP_BRAND_OFFSET] = 'i';
    prefix[FTYP_BRAND_OFFSET + BYTE_INDEX_ONE] = 's';
    prefix[FTYP_BRAND_OFFSET + BYTE_INDEX_TWO] = 'o';
    prefix[FTYP_BRAND_OFFSET + BYTE_INDEX_THREE] = 'm';

    constexpr const char *containerTypes[] = {BOX_TYPE_MOOV, BOX_TYPE_TRAK, BOX_TYPE_MDIA,
        BOX_TYPE_MINF, BOX_TYPE_STBL};
    constexpr size_t targetOffset = FTYP_BOX_SIZE + LARGE_BOX_HEADER_SIZE * CONTAINER_BOX_COUNT;
    uint64_t nestedBoxSize = payloadSize + LARGE_BOX_HEADER_SIZE;
    WriteLargeBoxHeader(prefix, targetOffset, targetType, nestedBoxSize);

    for (size_t index = CONTAINER_BOX_COUNT; index > 0; --index) {
        size_t parentIndex = index - 1;
        size_t offset = FTYP_BOX_SIZE + LARGE_BOX_HEADER_SIZE * parentIndex;
        nestedBoxSize += LARGE_BOX_HEADER_SIZE;
        WriteLargeBoxHeader(prefix, offset, containerTypes[parentIndex], nestedBoxSize);
    }

    size_t targetPayloadOffset = targetOffset + LARGE_BOX_HEADER_SIZE;
    if (strcmp(targetType, BOX_TYPE_CTTS) == 0 || strcmp(targetType, BOX_TYPE_STTS) == 0) {
        // version/flags and entryCount are both zero.
    } else if (strcmp(targetType, BOX_TYPE_HDLR) == 0) {
        prefix[targetPayloadOffset + HANDLER_TYPE_OFFSET] = 'v';
        prefix[targetPayloadOffset + HANDLER_TYPE_OFFSET + BYTE_INDEX_ONE] = 'i';
        prefix[targetPayloadOffset + HANDLER_TYPE_OFFSET + BYTE_INDEX_TWO] = 'd';
        prefix[targetPayloadOffset + HANDLER_TYPE_OFFSET + BYTE_INDEX_THREE] = 'e';
    } else if (strcmp(targetType, BOX_TYPE_MDHD) == 0) {
        WriteBigEndian32(prefix, targetPayloadOffset + MDHD_V0_TIME_SCALE_OFFSET, DEFAULT_TIME_SCALE);
    } else {
        close(fd);
        unlink(path.c_str());
        return false;
    }

    fileSize = LARGE_BOX_PAYLOAD_OFFSET + payloadSize;
    if (!WriteAll(fd, prefix.data(), prefix.size()) || ftruncate(fd, static_cast<off_t>(fileSize)) != 0) {
        close(fd);
        unlink(path.c_str());
        return false;
    }
    return true;
}

std::vector<uint8_t> MakeBox(const char *type, const std::vector<uint8_t> &payload)
{
    std::vector<uint8_t> box(NORMAL_BOX_HEADER_SIZE, 0);
    WriteBigEndian32(box, 0, static_cast<uint32_t>(box.size() + payload.size()));
    WriteBoxType(box, sizeof(uint32_t), type);
    box.insert(box.end(), payload.begin(), payload.end());
    return box;
}

std::vector<uint8_t> MakeContainerChain(const char *targetType, const std::vector<uint8_t> &payload)
{
    auto target = MakeBox(targetType, payload);
    auto stbl = MakeBox(BOX_TYPE_STBL, target);
    auto minf = MakeBox(BOX_TYPE_MINF, stbl);
    auto mdia = MakeBox(BOX_TYPE_MDIA, minf);
    auto trak = MakeBox(BOX_TYPE_TRAK, mdia);
    auto moov = MakeBox(BOX_TYPE_MOOV, trak);

    std::vector<uint8_t> ftypPayload(FTYP_PAYLOAD_SIZE, 0);
    ftypPayload[0] = 'i';
    ftypPayload[BYTE_INDEX_ONE] = 's';
    ftypPayload[BYTE_INDEX_TWO] = 'o';
    ftypPayload[BYTE_INDEX_THREE] = 'm';
    auto ftyp = MakeBox(BOX_TYPE_FTYP, ftypPayload);
    ftyp.insert(ftyp.end(), moov.begin(), moov.end());
    return ftyp;
}

std::vector<uint8_t> MakeDeepContainerMedia(size_t depth, const char *targetType,
    const std::vector<uint8_t> &payload)
{
    std::vector<uint8_t> nested = MakeBox(targetType, payload);
    for (size_t i = 0; i < depth; ++i) {
        nested = MakeBox(BOX_TYPE_MDIA, nested);
    }

    auto trak = MakeBox(BOX_TYPE_TRAK, nested);
    auto moov = MakeBox(BOX_TYPE_MOOV, trak);
    std::vector<uint8_t> ftypPayload(FTYP_PAYLOAD_SIZE, 0);
    ftypPayload[0] = 'i';
    ftypPayload[BYTE_INDEX_ONE] = 's';
    ftypPayload[BYTE_INDEX_TWO] = 'o';
    ftypPayload[BYTE_INDEX_THREE] = 'm';
    auto ftyp = MakeBox(BOX_TYPE_FTYP, ftypPayload);
    ftyp.insert(ftyp.end(), moov.begin(), moov.end());
    return ftyp;
}

bool RunLargeSizeCase(const char *targetType, uint64_t payloadSize, bool checkVideoTrack, bool expectVideoTrack)
{
    std::string path = "/data/local/tmp/pts_index_large_box_" + std::to_string(getpid()) + "_" + targetType;
    int fd = -1;
    uint64_t fileSize = 0;
    if (!CreateLargeSizeMedia(targetType, payloadSize, path, fd, fileSize)) {
        return false;
    }

    pid_t pid = fork();
    if (pid == 0) {
        struct rlimit limit {CHILD_ADDRESS_SPACE_LIMIT, CHILD_ADDRESS_SPACE_LIMIT};
        if (setrlimit(RLIMIT_AS, &limit) != 0) {
            _exit(CHILD_SETUP_FAILURE);
        }
        std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
        auto conversion = std::make_shared<TimeAndIndexConversion>();
        auto source = std::make_shared<MediaSource>(uri);
        Status result = conversion->SetDataSource(source);
        if (result != Status::OK) {
            _exit(1);
        }
        if (!checkVideoTrack) {
            _exit(0);
        }
        uint32_t trackIndex = 0;
        bool hasVideoTrack = conversion->GetFirstVideoTrackIndex(trackIndex) == Status::OK;
        _exit(hasVideoTrack == expectVideoTrack ? 0 : 1);
    }

    if (pid < 0) {
        close(fd);
        unlink(path.c_str());
        return false;
    }

    int status = 0;
    bool waited = waitpid(pid, &status, 0) == pid;
    close(fd);
    unlink(path.c_str());
    return waited && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace

bool RunLargeSizeReproduction(const char *targetType)
{
    return RunLargeSizeCase(targetType, LARGE_BOX_PAYLOAD_SIZE, false, false);
}

bool RunLargeSizeBoundary(uint64_t payloadSize, bool expectSkipped)
{
    constexpr uint64_t initialOffset = 16;
    TimeAndIndexConversion conversion;
    conversion.offset_ = initialOffset;

    bool skipped = conversion.SkipOversizedBox(payloadSize, "TEST");
    uint64_t expectedOffset = expectSkipped ? initialOffset + payloadSize : initialOffset;
    return skipped == expectSkipped && conversion.offset_ == expectedOffset;
}

bool RunDeepContainerReproduction(int &childStatus)
{
    childStatus = -1;
    std::string path = "/data/local/tmp/pts_index_deep_container_" + std::to_string(getpid());
    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }

    std::vector<uint8_t> media = MakeDeepContainerMedia(DEEP_CONTAINER_DEPTH, "free", {});
    bool writeSuccess = WriteAll(fd, media.data(), media.size());
    pid_t pid = fork();
    if (pid == 0) {
        struct rlimit limit {CHILD_STACK_LIMIT, CHILD_STACK_LIMIT};
        if (!writeSuccess || setrlimit(RLIMIT_STACK, &limit) != 0) {
            _exit(CHILD_SETUP_FAILURE);
        }
        std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(media.size());
        auto conversion = std::make_shared<TimeAndIndexConversion>();
        auto source = std::make_shared<MediaSource>(uri);
        Status result = conversion->SetDataSource(source);
        _exit(result == Status::OK ? 0 : 1);
    }

    if (pid < 0) {
        close(fd);
        unlink(path.c_str());
        return false;
    }

    bool waited = waitpid(pid, &childStatus, 0) == pid;
    close(fd);
    unlink(path.c_str());
    return waited && WIFEXITED(childStatus) && WEXITSTATUS(childStatus) == 0;
}

std::vector<uint8_t> MakeTimeEntryPayload(uint32_t entryCount, uint32_t secondValue)
{
    std::vector<uint8_t> payload(sizeof(uint32_t) * UINT32_WORD_COUNT_TWO +
        entryCount * sizeof(uint32_t) * UINT32_WORD_COUNT_TWO, 0);
    WriteBigEndian32(payload, sizeof(uint32_t), entryCount);
    if (entryCount > 0) {
        WriteBigEndian32(payload, sizeof(uint32_t) * UINT32_WORD_COUNT_TWO, SAMPLE_COUNT);
        WriteBigEndian32(payload, sizeof(uint32_t) * UINT32_WORD_COUNT_THREE, secondValue);
    }
    return payload;
}

std::vector<uint8_t> MakeHdlrPayload()
{
    std::vector<uint8_t> payload(sizeof(uint32_t) * UINT32_WORD_COUNT_THREE, 0);
    payload[HANDLER_TYPE_OFFSET] = 'v';
    payload[HANDLER_TYPE_OFFSET + BYTE_INDEX_ONE] = 'i';
    payload[HANDLER_TYPE_OFFSET + BYTE_INDEX_TWO] = 'd';
    payload[HANDLER_TYPE_OFFSET + BYTE_INDEX_THREE] = 'e';
    return payload;
}

std::vector<uint8_t> MakeMdhdPayload(uint8_t version)
{
    const size_t payloadSize = version == MDHD_VERSION_ONE ? MDHD_V1_PAYLOAD_SIZE : MDHD_V0_PAYLOAD_SIZE;
    std::vector<uint8_t> payload(payloadSize, 0);
    payload[0] = version;
    const size_t timeScaleOffset = version == MDHD_VERSION_ONE ? MDHD_V1_TIME_SCALE_OFFSET :
        MDHD_V0_TIME_SCALE_OFFSET;
    WriteBigEndian32(payload, timeScaleOffset, DEFAULT_TIME_SCALE);
    return payload;
}

bool ParseSmallSizeMedia(const char *targetType, const std::vector<uint8_t> &payload,
    TimeAndIndexConversion &conversion)
{
    std::string path = "/data/local/tmp/pts_index_small_box_" + std::to_string(getpid()) + "_" + targetType;
    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    std::vector<uint8_t> media = MakeContainerChain(targetType, payload);
    bool writeSuccess = WriteAll(fd, media.data(), media.size());
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(media.size());
    auto source = std::make_shared<MediaSource>(uri);
    Status result = writeSuccess ? conversion.SetDataSource(source) : Status::ERROR_UNKNOWN;
    close(fd);
    unlink(path.c_str());
    return result == Status::OK;
}

bool ParseDeepContainerMedia(size_t depth, TimeAndIndexConversion &conversion)
{
    std::string path = "/data/local/tmp/pts_index_depth_" + std::to_string(getpid()) + "_" +
        std::to_string(depth);
    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    std::vector<uint8_t> media = MakeDeepContainerMedia(depth, BOX_TYPE_HDLR, MakeHdlrPayload());
    bool writeSuccess = WriteAll(fd, media.data(), media.size());
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(media.size());
    auto source = std::make_shared<MediaSource>(uri);
    Status result = writeSuccess ? conversion.SetDataSource(source) : Status::ERROR_UNKNOWN;
    close(fd);
    unlink(path.c_str());
    return result == Status::OK;
}

} // namespace OHOS::Media::PtsAndIndexConversionTestUtils
