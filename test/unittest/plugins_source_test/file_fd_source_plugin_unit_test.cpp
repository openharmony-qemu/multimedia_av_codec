/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "file_fd_source_plugin_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileFdSource {
const std::string MEDIA_ROOT = "file:///data/test/media/";
const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
void OHOS::Media::Plugins::FileFdSource::FileFdSourceUnitTest::SetUpTestCase(void)
{
}

void FileFdSourceUnitTest::TearDownTestCase(void)
{
}

void FileFdSourceUnitTest::SetUp(void)
{
    fileFdSourcePlugin_ = std::make_shared<FileFdSourcePlugin>("testPlugin");
    size_t bufferSize = 1024;
    std::shared_ptr<RingBuffer> ringBuffer = std::make_shared<RingBuffer>(bufferSize);
    fileFdSourcePlugin_->ringBuffer_ = ringBuffer;
}

void FileFdSourceUnitTest::TearDown(void)
{
    fileFdSourcePlugin_->ringBuffer_ = nullptr;
    fileFdSourcePlugin_ = nullptr;
}

class RingBufferMock : public RingBuffer {
public:
    explicit RingBufferMock(size_t bufferSize) : RingBuffer(std::move(bufferSize)) {}

    size_t ReadBuffer(void* ptr, size_t readSize, int waitTimes = 0)
    {
        return readBufferSize_;
    }

    bool Seek(uint64_t offset)
    {
        (void)offset;
        return seekRet_;
    }

    void SetActive(bool active, bool cleanData = true)
    {
        (void)active;
        (void)cleanData;
    }

    size_t readBufferSize_ = 0;
    bool seekRet_ = false;
};

class CallbackMock : public Plugins::Callback {
public:
    void OnEvent(const PluginEvent &event)
    {
        description_ = event.description;
    }

    std::string description_;
};

class SourceCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        (void)event;
    }

    void SetSelectBitRateFlag(bool flag) override
    {
        (void)flag;
    }

    bool CanAutoSelectBitRate() override
    {
        return true;
    }
};

/**
 * @tc.name: FileFdSource_SetSource_0100
 * @tc.desc: SetSource
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetSource_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->SetSource(mediaSource));
}

/**
 * @tc.name: FileFdSource_SetSource_0200
 * @tc.desc: SetSource
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetSource_0200, TestSize.Level1)
{
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    fileFdSourcePlugin_->isCloudFile_ = true;
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->SetSource(mediaSource));
}

/**
 * @tc.name: FileFdSource_NotifyBufferingStart_0100
 * @tc.desc: FileFdSource_NotifyBufferingStart_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_NotifyBufferingStart_0100, TestSize.Level1)
{
    Plugins::Callback* sourceCallback = new SourceCallback();
    fileFdSourcePlugin_->isInterrupted_ = false;
    fileFdSourcePlugin_->NotifyBufferingStart();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->isInterrupted_ = true;
    fileFdSourcePlugin_->NotifyBufferingStart();

    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    fileFdSourcePlugin_->NotifyBufferingStart();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
    delete sourceCallback;
    sourceCallback = nullptr;
}
/**
 * @tc.name: FileFdSource_NotifyBufferingPercent_0100
 * @tc.desc: FileFdSource_NotifyBufferingPercent_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_NotifyBufferingPercent_0100, TestSize.Level1)
{
    Plugins::Callback* sourceCallback = new SourceCallback();
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    fileFdSourcePlugin_->waterLineAbove_ = 1;
    fileFdSourcePlugin_->NotifyBufferingPercent();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->NotifyBufferingPercent();
    
    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    fileFdSourcePlugin_->NotifyBufferingPercent();
    fileFdSourcePlugin_->isBuffering_ = true;
    fileFdSourcePlugin_->isInterrupted_ = false;
    fileFdSourcePlugin_->NotifyBufferingPercent();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
    delete sourceCallback;
    sourceCallback = nullptr;
}
/**
 * @tc.name: FileFdSource_NotifyBufferingEnd_0100
 * @tc.desc: FileFdSource_NotifyBufferingEnd_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_NotifyBufferingEnd_0100, TestSize.Level1)
{
    Plugins::Callback* sourceCallback = new SourceCallback();
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    fileFdSourcePlugin_->NotifyBufferingEnd();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->NotifyBufferingEnd();

    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    fileFdSourcePlugin_->NotifyBufferingEnd();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
    delete sourceCallback;
    sourceCallback = nullptr;
}
/**
 * @tc.name: FileFdSource_NotifyReadFail_0100
 * @tc.desc: FileFdSource_NotifyReadFail_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_NotifyReadFail_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->NotifyReadFail();
    Plugins::Callback* sourceCallback = new SourceCallback();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->NotifyReadFail();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
    fileFdSourcePlugin_->NotifyReadFail();
    delete sourceCallback;
    sourceCallback = nullptr;
}
/**
 * @tc.name: FileFdSource_read_0100
 * @tc.desc: FileFdSource_read_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->NotifyBufferingStart();
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_read_0200
 * @tc.desc: FileFdSource_read_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0200, TestSize.Level1)
{
    std::shared_ptr<Buffer> buffer = nullptr;
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_read_0300
 * @tc.desc: FileFdSource_read_0300
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0300, TestSize.Level1)
{
    fileFdSourcePlugin_->NotifyBufferingStart();
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    fileFdSourcePlugin_->isCloudFile_ = true;
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_read_0400
 * @tc.desc: FileFdSource_read_0400
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0400, TestSize.Level1)
{
    std::shared_ptr<Buffer> buffer = nullptr;
    fileFdSourcePlugin_->isCloudFile_ = true;
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_ParseUriInfo_0100
 * @tc.desc: FileFdSource_ParseUriInfo_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_ParseUriInfo_0100, TestSize.Level1)
{
    std::string uri = "";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    fileFdSourcePlugin_->SetSource(source);
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Reset());
}
/**
 * @tc.name: FileFdSource_ParseUriInfo_0200
 * @tc.desc: FileFdSource_ParseUriInfo_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_ParseUriInfo_0200, TestSize.Level1)
{
    std::string uri = "";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    fileFdSourcePlugin_->isCloudFile_ = true;
    fileFdSourcePlugin_->SetSource(source);
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Reset());
}
/**
 * @tc.name: FileFdSource_Reset_0100
 * @tc.desc: FileFdSource_Reset_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_Reset_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Reset());
}
/**
 * @tc.name: FileFdSource_SetBundleName_0100
 * @tc.desc: FileFdSource_SetBundleName_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetBundleName_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->Stop();
    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
}

/**
 * @tc.name: FileFdSource_ReadOnlineFile_0100
 * @tc.desc: FileFdSource_ReadOnlineFile_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_ReadOnlineFile_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->GetCacheTime(0.0);
    fileFdSourcePlugin_->isBuffering_ = true;
    std::shared_ptr<Buffer> buffer;
    EXPECT_EQ(Status::ERROR_AGAIN, fileFdSourcePlugin_->ReadOnlineFile(0, buffer, 0, 0));
}

/**
 * @tc.name: FileFdSource_SeekToOnlineFile_0100
 * @tc.desc: FileFdSource_SeekToOnlineFile_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SeekToOnlineFile_0100, TestSize.Level1)
{
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    buffer->seekRet_ = true;
    int64_t offset = 0;
    EXPECT_EQ(Status::ERROR_UNKNOWN, fileFdSourcePlugin_->SeekToOnlineFile(offset));
}

/**
 * @tc.name: FileFdSource_SeekToOnlineFile_0200
 * @tc.desc: FileFdSource_SeekToOnlineFile_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SeekToOnlineFile_0200, TestSize.Level1)
{
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    buffer->seekRet_ = false;
    int64_t offset = 0;
    std::shared_ptr<Task> task = std::make_shared<Task>("test");
    fileFdSourcePlugin_->downloadTask_ = task;
    EXPECT_EQ(Status::ERROR_UNKNOWN, fileFdSourcePlugin_->SeekToOnlineFile(offset));
    sleep(1);
}

/**
 * @tc.name: FileFdSource_SeekToOnlineFile_0300
 * @tc.desc: FileFdSource_SeekToOnlineFile_0300
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SeekToOnlineFile_0300, TestSize.Level1)
{
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    buffer->seekRet_ = false;
    int64_t offset = 0;
    EXPECT_EQ(Status::ERROR_UNKNOWN, fileFdSourcePlugin_->SeekToOnlineFile(offset));
    sleep(1);
}

/**
 * @tc.name: FileFdSource_CacheDataLoop_0100
 * @tc.desc: FileFdSource_CacheDataLoop_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_CacheDataLoop_0100, TestSize.Level1)
{
    std::shared_ptr<Task> task = std::make_shared<Task>("test");
    fileFdSourcePlugin_->downloadTask_ = task;
    fileFdSourcePlugin_->PauseDownloadTask(true);
    sleep(1);
    fileFdSourcePlugin_->PauseDownloadTask(false);
    sleep(1);
    fileFdSourcePlugin_->isInterrupted_ = true;
    fileFdSourcePlugin_->CacheDataLoop();

    fileFdSourcePlugin_->isInterrupted_ = false;
    fileFdSourcePlugin_->CacheDataLoop();

    fileFdSourcePlugin_->size_ = 10;
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    buffer->readBufferSize_ = true;
    fileFdSourcePlugin_->isBuffering_ = true;
    fileFdSourcePlugin_->waterLineAbove_ = 0;
    fileFdSourcePlugin_->CacheDataLoop();
    EXPECT_EQ(0, fileFdSourcePlugin_->ringBufferSize_);

    buffer->readBufferSize_ = false;
    fileFdSourcePlugin_->inSeek_ = false;
    fileFdSourcePlugin_->isInterrupted_ = true;
    fileFdSourcePlugin_->ringBufferSize_ = 10;
    fileFdSourcePlugin_->CacheDataLoop();

    fileFdSourcePlugin_->inSeek_ = true;
    fileFdSourcePlugin_->ringBufferSize_ = 0;
    fileFdSourcePlugin_->cachePosition_ = 1;
    fileFdSourcePlugin_->CacheDataLoop();

    fileFdSourcePlugin_->CacheDataLoop();
    EXPECT_EQ(0, fileFdSourcePlugin_->ringBufferSize_);
}

/**
 * @tc.name: FileFdSource_HandleBuffering_0100
 * @tc.desc: FileFdSource_HandleBuffering_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_HandleBuffering_0100, TestSize.Level1)
{
    EXPECT_EQ(false, fileFdSourcePlugin_->HandleBuffering());
    fileFdSourcePlugin_->isBuffering_ = true;
    EXPECT_EQ(true, fileFdSourcePlugin_->HandleBuffering());
    fileFdSourcePlugin_->isInterrupted_ = true;
    EXPECT_EQ(true, fileFdSourcePlugin_->HandleBuffering());
    fileFdSourcePlugin_->isInterrupted_ = false;
    fileFdSourcePlugin_->isReadBlocking_ = true;
    fileFdSourcePlugin_->isBuffering_ = false;
    EXPECT_EQ(false, fileFdSourcePlugin_->HandleBuffering());
}

/**
 * @tc.name: FileFdSource_NotifyBufferingPercent_0200
 * @tc.desc: FileFdSource_NotifyBufferingPercent_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_NotifyBufferingPercent_0200, TestSize.Level1)
{
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    fileFdSourcePlugin_->NotifyBufferingPercent();
    fileFdSourcePlugin_->waterLineAbove_ = 1;
    fileFdSourcePlugin_->ringBufferSize_ = 100;;
    fileFdSourcePlugin_->isBuffering_ = true;
    CallbackMock* cb = new CallbackMock();
    fileFdSourcePlugin_->callback_ = cb;
    fileFdSourcePlugin_->NotifyBufferingStart();
    EXPECT_EQ("start", cb->description_);
    fileFdSourcePlugin_->isInterrupted_ = true;;
    fileFdSourcePlugin_->NotifyBufferingPercent();

    fileFdSourcePlugin_->callback_ = nullptr;
    fileFdSourcePlugin_->NotifyBufferingPercent();

    fileFdSourcePlugin_->isBuffering_ = false;
    fileFdSourcePlugin_->NotifyBufferingPercent();

    fileFdSourcePlugin_->callback_ = cb;
    fileFdSourcePlugin_->waterLineAbove_ = 1;
    fileFdSourcePlugin_->isBuffering_ = true;
    fileFdSourcePlugin_->isInterrupted_ = false;
    fileFdSourcePlugin_->NotifyBufferingPercent();

    EXPECT_EQ("0", cb->description_);
    delete cb;
    cb = nullptr;
}

/**
 * @tc.name: FileFdSource_NotifyBufferingEnd_0200
 * @tc.desc: FileFdSource_NotifyBufferingEnd_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_NotifyBufferingEnd_0200, TestSize.Level1)
{
    std::shared_ptr<RingBufferMock> buffer = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->ringBuffer_ = buffer;
    fileFdSourcePlugin_->isBuffering_ = true;
    fileFdSourcePlugin_->NotifyBufferingEnd();
    EXPECT_EQ(false, fileFdSourcePlugin_->isBuffering_);

    fileFdSourcePlugin_->isBuffering_ = true;
    CallbackMock* cb = new CallbackMock();
    fileFdSourcePlugin_->NotifyBufferingStart();
    fileFdSourcePlugin_->callback_ = cb;
    fileFdSourcePlugin_->isInterrupted_ = true;
    fileFdSourcePlugin_->NotifyBufferingEnd();
    EXPECT_EQ(false, fileFdSourcePlugin_->isBuffering_);

    fileFdSourcePlugin_->isInterrupted_ = false;
    fileFdSourcePlugin_->NotifyBufferingEnd();
    EXPECT_EQ("end", cb->description_);
    delete cb;
    cb = nullptr;
}

/**
 * @tc.name: FileFdSource_SetInterruptState_0100
 * @tc.desc: FileFdSource_SetInterruptState_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetInterruptState_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->ringBuffer_ = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    fileFdSourcePlugin_->isInterrupted_ = true;
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    fileFdSourcePlugin_->isCloudFile_ = true;
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    std::shared_ptr<Task> task = std::make_shared<Task>("test");
    fileFdSourcePlugin_->downloadTask_ = task;
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    sleep(1);
}

/**
 * @tc.name: FileFdSource_CheckFileType_0100
 * @tc.desc: FileFdSource_CheckFileType_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_CheckFileType_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->ringBuffer_ = std::make_shared<RingBufferMock>(0);
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    fileFdSourcePlugin_->isInterrupted_ = true;
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    fileFdSourcePlugin_->isCloudFile_ = true;
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    std::shared_ptr<Task> task = std::make_shared<Task>("test");
    fileFdSourcePlugin_->downloadTask_ = task;
    fileFdSourcePlugin_->SetInterruptState(true);
    EXPECT_EQ(true, fileFdSourcePlugin_->isInterrupted_);
    sleep(1);
}

/**
 * @tc.name: FileFdSource_GetBufferPtr_0100
 * @tc.desc: FileFdSource_GetBufferPtr_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_GetBufferPtr_0100, TestSize.Level1)
{
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    EXPECT_NE(nullptr, fileFdSourcePlugin_->GetBufferPtr(buffer, 10));
}

/**
 * @tc.name: FileFdSource_GetCurrentSpeed_0100
 * @tc.desc: FileFdSource_GetCurrentSpeed_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_GetCurrentSpeed_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->GetCurrentSpeed(0);
    fileFdSourcePlugin_->downloadSize_ = 10;
    fileFdSourcePlugin_->GetCurrentSpeed(2 * 1000);
    EXPECT_EQ(5, fileFdSourcePlugin_->avgDownloadSpeed_);
}

/**
 * @tc.name: FileFdSource_GetCacheTime_0100
 * @tc.desc: FileFdSource_GetCacheTime_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_GetCacheTime_0100, TestSize.Level1)
{
    EXPECT_EQ(5, fileFdSourcePlugin_->GetCacheTime(0.25));
    EXPECT_EQ(5, fileFdSourcePlugin_->GetCacheTime(0.75));
    EXPECT_TRUE((int)(fileFdSourcePlugin_->GetCacheTime(1) * 1000000) == (int)(0.3 * 1000000));
}

/**
 * @tc.name: FileFdSource_DeleteCacheBuffer_0100
 * @tc.desc: FileFdSource_DeleteCacheBuffer_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_DeleteCacheBuffer_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->DeleteCacheBuffer(nullptr, 0);
    int32_t bufferSize = 4;
    char* cacheBuffer = new char[bufferSize];

    fileFdSourcePlugin_->DeleteCacheBuffer(cacheBuffer, 0);
    EXPECT_NE(nullptr, cacheBuffer);
}

/**
 * @tc.name: FileFdSource_CheckReadTime_0100
 * @tc.desc: FileFdSource_CheckReadTime_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_CheckReadTime_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->curReadTime_ = 10;
    fileFdSourcePlugin_->CheckReadTime();
    EXPECT_EQ(10, fileFdSourcePlugin_->lastReadTime_);

    fileFdSourcePlugin_->lastReadTime_ = 1000;
    fileFdSourcePlugin_->curReadTime_  = 2000;
    fileFdSourcePlugin_->CheckReadTime();
    EXPECT_EQ(1000, fileFdSourcePlugin_->lastReadTime_);

    fileFdSourcePlugin_->curReadTime_  = 1020;
    fileFdSourcePlugin_->CheckReadTime();
    EXPECT_EQ(1000, fileFdSourcePlugin_->lastReadTime_);

    fileFdSourcePlugin_->curReadTime_  = 1040;
    fileFdSourcePlugin_->CheckReadTime();
    EXPECT_EQ(0, fileFdSourcePlugin_->lastReadTime_);
}

/**
 * @tc.name: FileFdSource_checkReadTime_0200
 * @tc.desc: FileFdSource_checkReadTime_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_checkReadTime_0200, TestSize.Level1)
{
    fileFdSourcePlugin_->callback_ = nullptr;
    int64_t curTime = 0;
    int64_t lastTime = 0;
    auto isValidTime = fileFdSourcePlugin_->IsValidTime(curTime, lastTime);
    ASSERT_FALSE(isValidTime);
    fileFdSourcePlugin_->CheckReadTime();
}
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS