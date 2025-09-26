/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file expect in compliance with the License.
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
#include <fcntl.h>
#include <cstdio>
#include <sstream>
#include <sys/stat.h>

#include "meta/meta_key.h"
#include "meta/mime_type.h"
#include "meta/video_types.h"
#include "meta/audio_types.h"
#include "native_avcodec_base.h"

#include "avbuffer/avbuffer_mock.h"
#include "common_mock.h"
#include "demuxer_test_helper.h"
namespace OHOS {
namespace MediaAVCodec {
static const int64_t SOURCE_OFFSET = 0;
static const int32_t BUFFER_SIZE = 3840 * 2160;
static const std::string TEST_FILE_PATH = "/data/test/media/";
static const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const int32_t FRAME_COUNT_INDEX = 0;
static const int32_t KEY_FRAME_COUNT_INDEX = 1;
static const int32_t EOS_INDEX = 2;

namespace {
#define LOG_MAX_SIZE 200
#define CHECK_RETURN(cond, ret, fmt, ...)                                 \
    do {                                                                  \
        if (!(cond)) {                                                    \
            char ch[LOG_MAX_SIZE];                                        \
            (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);        \
            (void)printf("===> [ERROR][%s] %s\n", __func__, ch);          \
            return ret;                                                   \
        }                                                                 \
    } while (0)

#define CHECK_BREAK(cond, fmt, ...)                                       \
    if (!(cond)) {                                                        \
        char ch[LOG_MAX_SIZE];                                            \
        (void)sprintf_s(ch, LOG_MAX_SIZE, fmt, ##__VA_ARGS__);            \
        (void)printf("===> [ERROR][%s] %s\n", __func__, ch);              \
        break;                                                            \
    }

template <typename T>
std::string DumpVector(const std::vector<T>& vec)
{
    std::stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
        ss << vec[i];
        if (i != vec.size() - 1) {
            ss << ", ";
        }
    }
    return ss.str();
}

bool InfoIsEmpty(Info info)
{
    return (info.intValue.empty() && info.longValue.empty() && info.floatValue.empty() && info.doubleValue.empty() &&
            info.stringValue.empty() && info.bufferValue.empty() && info.intBufferValue.empty());
}

bool FinishRead(ReadInfo trackReadInfo)
{
    for (auto iter : trackReadInfo) {
        if (iter.second[EOS_INDEX] != 1) {
            return false;
        }
    }
    return true;
}

bool AllTrackGetEos(std::map<int32_t, bool> eosMap)
{
    for (auto iter : eosMap) {
        if (!iter.second) {
            return false;
        }
    }
    return true;
}

std::string DumpInt32Buffer(int32_t* addr, size_t size)
{
    std::stringstream ss;
    for (size_t i = 0; i < size; ++i) {
        ss << addr[i];
        if (i != size - 1) {
            ss << ",";
        }
    }
    return ss.str();
}
}

DemuxerTestHelper::~DemuxerTestHelper()
{
    if (source_ != nullptr) {
        source_->Destroy();
        source_ = nullptr;
    }
    if (demuxer_ != nullptr) {
        demuxer_->Destroy();
        demuxer_ = nullptr;
    }
    if (sourceFormat_ != nullptr) {
        sourceFormat_->Destroy();
        sourceFormat_ = nullptr;
    }
    if (buffer_ != nullptr) {
        buffer_->Destroy();
        buffer_ = nullptr;
    }
    if (memory_ != nullptr) {
        memory_->Destroy();
        memory_ = nullptr;
    }
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    if (passProcess_.size() > 0) {
        std::string passInfo = "";
        for (auto index = 0; index < passProcess_.size(); ++index) {
            passInfo += passProcess_[index];
            if (index < passProcess_.size() - 1) {
                passInfo += " | ";
            }
        }
        printf("[RESULT][PASS] %s\n", passInfo.c_str());
    }
    if (errorProcess_.size() > 0) {
        std::string errorInfo = "";
        for (auto index = 0; index < errorProcess_.size(); ++index) {
            errorInfo += errorProcess_[index];
            if (index < errorProcess_.size() - 1) {
                errorInfo += " | ";
            }
        }
        printf("[RESULT][FAIL] %s\n", errorInfo.c_str());
    }
}

int32_t DemuxerTestHelper::TestFile(std::string fileAbsPath, SourceType sourceType, FileBaseInfo info)
{
    int32_t initRet = Init(fileAbsPath, sourceType);
    CHECK_RETURN(initRet == AV_ERR_OK, initRet, "init ret[%d]", initRet);
    fileInfo_ = info;
    CHECK_RETURN(!InfoIsEmpty(fileInfo_.sourceInfo), AV_ERR_OK, "[INFO][%s] expect source format empty", __func__);
    printf("\n");

    int32_t formatRet = CheckFormat();
    printf("\n");
    if (trackCount_ <= 0) {
        return AV_ERR_OK;
    }

    int32_t readRet = AV_ERR_OK;
    if (fileInfo_.expectReadInfo.empty()) {
        printf("[INFO][%s] expect read info empty\n", __func__);
    } else {
        readRet = CheckReadSample();
        printf("\n");
        if (readRet != AV_ERR_OK) {
            errorProcess_.push_back("CheckReadSample");
        } else {
            passProcess_.push_back("CheckReadSample");
        }
    }

    int32_t seekRet = AV_ERR_OK;
    if (fileInfo_.expectSeekInfo.empty()) {
        printf("[INFO][%s] expect seek info empty\n", __func__);
    } else {
        seekRet = CheckSeek();
        if (seekRet != AV_ERR_OK) {
            errorProcess_.push_back("CheckSeek");
        } else {
            passProcess_.push_back("CheckSeek");
        }
    }

    return (formatRet == AV_ERR_OK && readRet == AV_ERR_OK && seekRet == AV_ERR_OK) ? AV_ERR_OK : AV_ERR_UNKNOWN;
}

int32_t DemuxerTestHelper::Init(std::string fileAbsPath, SourceType sourceType)
{
    errorProcess_.push_back("Init");
    CHECK_RETURN(!fileAbsPath.empty(), AV_ERR_INVALID_VAL, "file invalid");
    switch (sourceType) {
        case SourceType::LOCAL: {
            std::string localPath = TEST_FILE_PATH + fileAbsPath;
            fd_ = open(localPath.c_str(), O_RDONLY);
            CHECK_RETURN(fd_ >= 0, AV_ERR_UNKNOWN, "file open failed[%s]", localPath.c_str());
            struct stat fileStatus {};
            CHECK_RETURN(stat(localPath.c_str(), &fileStatus) == 0, AV_ERR_UNKNOWN, "get size failed");
            source_ = AVSourceMockFactory::CreateSourceWithFD(
                fd_, SOURCE_OFFSET, static_cast<int64_t>(fileStatus.st_size));
            printf("[TRACE][File] path[%s] | size[%" PRId64 "] | fd[%d]\n",
                localPath.c_str(), static_cast<int64_t>(fileStatus.st_size), fd_);
            break;
        }
        case SourceType::URI: {
            std::string remotePath = TEST_URI_PATH + fileAbsPath;
            source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(remotePath.c_str()));
            printf("[TRACE][File] uri[%s]\n", remotePath.c_str());
            break;
        }
        default: {
            printf("[INFO][%s] unsupport source type[%d]\n", __func__, static_cast<int32_t>(sourceType));
            break;
        }
    }
    CHECK_RETURN(source_ != nullptr, AV_ERR_UNKNOWN, "create source failed");
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    CHECK_RETURN(demuxer_ != nullptr, AV_ERR_UNKNOWN, "create demuxer failed");
    buffer_ = AVBufferMockFactory::CreateAVBuffer(BUFFER_SIZE);
    CHECK_RETURN(buffer_ != nullptr, AV_ERR_UNKNOWN, "create buffer failed");
    memory_ = AVMemoryMockFactory::CreateAVMemoryMock(BUFFER_SIZE);
    CHECK_RETURN(memory_ != nullptr, AV_ERR_UNKNOWN, "create memory failed");
    DumpFileInfo();
    errorProcess_.pop_back();
    passProcess_.push_back("Init");
    return AV_ERR_OK;
}

void DemuxerTestHelper::DumpFileInfo()
{
    std::shared_ptr<FormatMock> sourceFormat = source_->GetSourceFormat();
    if (sourceFormat == nullptr) {
        return;
    }
    printf("[TRACE][Format][Source] %s\n", sourceFormat->DumpInfo());
    int32_t trackCount = 0;
    bool ret = sourceFormat->GetIntValue(Tag::MEDIA_TRACK_COUNT, trackCount);
    sourceFormat->Destroy();
    if (!ret || trackCount == 0) {
        return;
    }
    for (int32_t trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        std::shared_ptr<FormatMock> trackFormat = source_->GetTrackFormat(trackIndex);
        if (trackFormat != nullptr) {
            printf("[TRACE][Format][Track%d] %s\n", trackIndex, trackFormat->DumpInfo());
            trackFormat->Destroy();
        }
    }
}

int32_t DemuxerTestHelper::CheckFormat()
{
    sourceFormat_ = source_->GetSourceFormat();
    CHECK_RETURN(sourceFormat_ != nullptr, AV_ERR_UNKNOWN, "get source format failed");
    int32_t sourceRet = CheckSourceFormat();
    if (sourceRet != AV_ERR_OK) {
        errorProcess_.push_back("CheckSourceFormat");
    } else {
        passProcess_.push_back("CheckSourceFormat");
    }
    bool ret = sourceFormat_->GetIntValue(Tag::MEDIA_TRACK_COUNT, trackCount_);
    CHECK_RETURN(ret, AV_ERR_UNKNOWN, "get track count failed");
    if (trackCount_ == 0) {
        printf("[INFO][%s] no track in file\n", __func__);
        return AV_ERR_OK;
    }

    if (fileInfo_.trackInfo.empty()) {
        printf("[INFO][%s] expect track format empty\n", __func__);
        return AV_ERR_OK;
    }
    int32_t trackRet = CheckTrackFormat();
    if (trackRet != AV_ERR_OK) {
        errorProcess_.push_back("CheckTrackFormat");
    } else {
        passProcess_.push_back("CheckTrackFormat");
    }
    if (sourceRet == AV_ERR_OK && trackRet == AV_ERR_OK) {
        return AV_ERR_OK;
    }
    return AV_ERR_UNKNOWN;
}

int32_t DemuxerTestHelper::CheckSourceFormat()
{
    return CheckInfo(fileInfo_.sourceInfo, sourceFormat_, "Source");
}

int32_t DemuxerTestHelper::CheckTrackFormat()
{
    std::map<int32_t, int32_t> retMap;
    for (int32_t trackIndex = 0; trackIndex < trackCount_; ++trackIndex) {
        if (fileInfo_.trackInfo.count(trackIndex) <= 0) {
            continue;
        }
        retMap[trackIndex] = AV_ERR_OK;

        std::shared_ptr<FormatMock> trackFormat = source_->GetTrackFormat(trackIndex);
        if (trackFormat == nullptr) {
            retMap[trackIndex] = AV_ERR_UNKNOWN;
            printf("===> [ERROR][%s] get track[%d] format failed\n", __func__, trackIndex);
            continue;
        }
        retMap[trackIndex] = CheckInfo(
            fileInfo_.trackInfo[trackIndex], trackFormat, "Track" + std::to_string(trackIndex));
    
        int32_t trackType = -1;
        if (trackFormat->GetIntValue(Tag::MEDIA_TYPE, trackType)) {
            trackReadInfo_[trackIndex] = {0, 0, 0}; // frame, keyframe, eos
        }

        trackFormat->Destroy();
    }
    for (auto iter : retMap) {
        if (iter.second != AV_ERR_OK) {
            return iter.second;
        }
    }
    for (auto iter : fileInfo_.expectReadInfo) {
        CHECK_RETURN(trackReadInfo_.count(iter.first) > 0, AV_ERR_UNKNOWN,
                "track [%d] has not been selected", iter.first);
    }
    return AV_ERR_OK;
}

int32_t DemuxerTestHelper::CheckBaseTypeInfo(Info info, std::shared_ptr<FormatMock> format, std::string logPrefix)
{
    int32_t ret = AV_ERR_OK;
    for (auto iter : info.intValue) {
        int32_t intVal = 0;
        if (!format->GetIntValue(iter.first, intVal) || intVal != iter.second) {
            ret = AV_ERR_UNKNOWN;
            printf("===> [ERROR][%s][%s] failed: key[%s] value[%d] expect[%d]\n", logPrefix.c_str(), __func__,
                iter.first.c_str(), intVal, iter.second);
        }
    }
    for (auto iter : info.longValue) {
        int64_t longVal = 0;
        if (!format->GetLongValue(iter.first, longVal) || longVal != iter.second) {
            ret = AV_ERR_UNKNOWN;
            printf("===> [ERROR][%s][%s] failed: key[%s] value[%" PRId64 "] expect[%" PRId64 "]\n",
                logPrefix.c_str(), __func__, iter.first.c_str(), longVal, iter.second);
        }
    }
    for (auto iter : info.floatValue) {
        float floatVal = 0;
        if (!format->GetFloatValue(iter.first, floatVal) || floatVal != iter.second) {
            ret = AV_ERR_UNKNOWN;
            printf("===> [ERROR][%s][%s] failed: key[%s] value[%f] expect[%f]\n", logPrefix.c_str(), __func__,
                iter.first.c_str(), floatVal, iter.second);
        }
    }
    for (auto iter : info.doubleValue) {
        double doubleVal = 0;
        if (!format->GetDoubleValue(iter.first, doubleVal) || doubleVal != iter.second) {
            ret = AV_ERR_UNKNOWN;
            printf("===> [ERROR][%s][%s] failed: key[%s] value[%f] expect[%f]\n", logPrefix.c_str(), __func__,
                iter.first.c_str(), doubleVal, iter.second);
        }
    }
    for (auto iter : info.stringValue) {
        std::string stringVal = "";
        if (!format->GetStringValue(iter.first, stringVal) || stringVal != iter.second) {
            ret = AV_ERR_UNKNOWN;
            printf("===> [ERROR][%s][%s] failed: key[%s] value[%s] expect[%s]\n", logPrefix.c_str(), __func__,
                iter.first.c_str(), stringVal.c_str(), iter.second.c_str());
        }
    }
    return ret;
}

int32_t DemuxerTestHelper::CheckBufferTypeInfo(Info info, std::shared_ptr<FormatMock> format, std::string logPrefix)
{
    int32_t ret = AV_ERR_OK;
    // std::map<std::string, {addr, size}>;
    for (auto iter : info.bufferValue) {
        uint8_t *addr = nullptr;
        size_t size = 0;
        bool pass = (format->GetBuffer(iter.first, &addr, size)) && (size == iter.second.size);
        if (!pass) {
            printf("===> [ERROR][%s][%s] key[%s] value[%zu] expect[%zu]\n", logPrefix.c_str(), __func__,
                iter.first.c_str(), size, iter.second.size);
            ret = AV_ERR_UNKNOWN;
        }
    }
    // std::map<std::string, std::vector<int32_t>>;
    for (auto iter : info.intBufferValue) {
        int32_t *addr = nullptr;
        size_t size = 0;
        bool pass = (format->GetIntBuffer(iter.first, &addr, size)) && (size == iter.second.size());
        for (int32_t i = 0; i < iter.second.size() && i < size; ++i) {
            pass = !pass ? pass : (addr[i] == iter.second[i]);
        }
        if (!pass) {
            printf("===> [ERROR][%s][%s] key[%s] value[%s] expect[%s]\n", logPrefix.c_str(), __func__,
                iter.first.c_str(), DumpInt32Buffer(addr, size).c_str(), DumpVector(iter.second).c_str());
            ret = AV_ERR_UNKNOWN;
        }
    }
    return ret;
}

int32_t DemuxerTestHelper::CheckInfo(Info info, std::shared_ptr<FormatMock> format, std::string logPrefix)
{
    int32_t ret = CheckBaseTypeInfo(info, format, logPrefix);
    ret = ret != AV_ERR_OK ? ret : CheckBufferTypeInfo(info, format, logPrefix);
    return ret;
}

void DemuxerTestHelper::CountReadFrame(int32_t trackIndex, AVBufferFlag flag)
{
    if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(AVBufferFlag::EOS)) {
        trackReadInfo_[trackIndex][EOS_INDEX] = 1;
        return;
    }
    trackReadInfo_[trackIndex][FRAME_COUNT_INDEX]++;
    if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) {
        trackReadInfo_[trackIndex][KEY_FRAME_COUNT_INDEX]++;
    }
}

int32_t DemuxerTestHelper::ReadAllSample()
{
    while (!FinishRead(trackReadInfo_)) {
        for (auto iter : fileInfo_.expectReadInfo) {
            int32_t ret = demuxer_->ReadSampleBuffer(iter.first, buffer_);
            CHECK_RETURN(ret == AV_ERR_OK, ret, "read sample buffer failed, ret[%d]", ret);
            OH_AVCodecBufferAttr attr;
            ret = buffer_->GetBufferAttr(attr);
            CHECK_RETURN(ret == AV_ERR_OK, ret, "get sample attribute failed, ret[%d]", ret);
            CountReadFrame(iter.first, static_cast<AVBufferFlag>(attr.flags));
            AVCodecBufferInfo info;
            uint32_t flag;
            ret = demuxer_->ReadSample(iter.first, memory_, &info, flag);
            CHECK_RETURN(ret == AV_ERR_OK, ret, "read sample failed, ret[%d]", ret);
            CountReadFrame(iter.first, static_cast<AVBufferFlag>(flag));
        }
    }
    return AV_ERR_OK;
}

int32_t DemuxerTestHelper::CheckReadSample()
{
    int32_t ret = AV_ERR_OK;
    for (auto iter : fileInfo_.expectReadInfo) {
        ret = demuxer_->SelectTrackByID(iter.first);
        CHECK_RETURN(ret == AV_ERR_OK, ret, "select track failed");
    }
    ret = ReadAllSample();
    for (auto iter : trackReadInfo_) {
        printf("[TRACE][Read][Track%d] frame[%" PRId64 "] | keyFrame[%" PRId64 "]\n",
            iter.first, iter.second[0], iter.second[1]);
    }
    CHECK_RETURN(ret == AV_ERR_OK, ret, "read sample failed");
    bool checkValuePass = true;
    for (auto iter : fileInfo_.expectReadInfo) {
        for (int32_t i = 0; i < iter.second.size(); ++i) {
            std::string frameType = "default";
            if (i == 0) {
                frameType = "frame";
            } else if (i == 1) {
                frameType = "keyFrame";
            }
            if (trackReadInfo_.count(iter.first) <= 0 || i >= trackReadInfo_[iter.first].size()) {
                checkValuePass = false;
                printf("===> [ERROR][%s] check count failed: type[%s] info record error\n",
                    __func__, frameType.c_str());
                continue;
            }
            if (iter.second[i] != trackReadInfo_[iter.first][i]) {
                checkValuePass = false;
                printf("===> [ERROR][%s] check count failed: type[%s] value[%" PRId64 "] expect[%" PRId64 "]\n",
                    __func__, frameType.c_str(), trackReadInfo_[iter.first][i], iter.second[i]);
            }
        }
    }
    return checkValuePass ? AV_ERR_OK : AV_ERR_UNKNOWN;
}

bool DemuxerTestHelper::CheckSeekPos(int64_t seekTime, SeekMode seekMode)
{
    std::map<int32_t, int64_t> seekFrameInfo {};
    std::map<int32_t, bool> eosMap {};
    SeekInfoRecord expectSeekInfo = fileInfo_.expectSeekInfo[seekTime][seekMode];
    for (auto expectSeekTrackMap : expectSeekInfo.seekFrameInfo) {
        if (expectSeekTrackMap.first > trackCount_) {
            break;
        }
        seekFrameInfo[expectSeekTrackMap.first] = 0;
        eosMap[expectSeekTrackMap.first] = false;
    }
    
    while (!AllTrackGetEos(eosMap)) {
        for (auto iter : seekFrameInfo) {
            int32_t trackIndex = iter.first;
            AVCodecBufferInfo info;
            uint32_t flag;
            if (eosMap[trackIndex]) {
                continue;
            }
            if (demuxer_->ReadSample(trackIndex, memory_, &info, flag) != AV_ERR_OK) {
                break;
            }
            if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(AVBufferFlag::EOS)) {
                eosMap[trackIndex] = true;
                continue;
            }
            seekFrameInfo[trackIndex]++;
        }
    }
    for (auto iter : seekFrameInfo) {
        printf("[TRACE][Seek][PosCheck] track[%d] | frame[%" PRId64 "]\n", iter.first, iter.second);
    }
    bool checkSeekPass = true;
    for (auto expectSeekTrackMap : expectSeekInfo.seekFrameInfo) {
        int32_t trackIndex = expectSeekTrackMap.first;
        if (seekFrameInfo[trackIndex] != expectSeekTrackMap.second) {
            printf("===> [ERROR][%s] seekTime[%" PRId64 "] seekMode[%d] trackIndex[%d]"
                " value[%" PRId64 "] expect[%" PRId64 "]\n", __func__,
                seekTime, seekMode, trackIndex, seekFrameInfo[trackIndex], expectSeekTrackMap.second);
            checkSeekPass = false;
        }
    }
    return checkSeekPass;
}

// {seekTime, {seekMode, {seekSuccess, {trackIndex, count}}}}
int32_t DemuxerTestHelper::CheckSeek()
{
    int32_t ret = AV_ERR_OK;
    bool checkValuePass = true;
    for (auto seekTimeMap : fileInfo_.expectSeekInfo) {
        int64_t seekTime = seekTimeMap.first;
        for (auto seekModeMap : seekTimeMap.second) {
            SeekMode seekMode = seekModeMap.first;
            ret = demuxer_->SeekToTime(seekTime, seekMode);
            printf("[TRACE][Seek] seekTime[%" PRId64 "] | seekMode[%d] | seekRes[%d]\n",
                    seekTime, static_cast<int32_t>(seekMode), ret);
            if ((seekModeMap.second.seekSuccess && ret != AV_ERR_OK) ||
                (!seekModeMap.second.seekSuccess && ret == AV_ERR_OK)) {
                checkValuePass = false;
                printf("===> [ERROR][%s] seek exec exec[%s] expect[%s]\n",
                    __func__, ret == AV_ERR_OK ? "pass" : "fail", seekModeMap.second.seekSuccess ? "pass" : "fail");
                continue;
            }
            if (ret != AV_ERR_OK) {
                continue;
            }
            checkValuePass = !checkValuePass ? checkValuePass : CheckSeekPos(seekTime, seekMode);
        }
    }
    return checkValuePass ? AV_ERR_OK : AV_ERR_UNKNOWN;
}
} // namespace MediaAVCodec
} // namespace OHOS