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
void OHOS::Media::Plugins::FileFdSource::FileFdSourceUnitTest::SetUpTestCase(void)
{
}

void FileFdSourceUnitTest::TearDownTestCase(void)
{
}

void FileFdSourceUnitTest::SetUp(void)
{
    fileFdSourcePlugin_ = std::make_shared<FileFdSourcePlugin>("testPlugin");
}

void FileFdSourceUnitTest::TearDown(void)
{
    fileFdSourcePlugin_ = nullptr;
}

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

    bool CanDoSelectBitRate() override
    {
        return true;
    }
}

/**
 * @tc.name: FileFdSource_SetSource_0100
 * @tc.desc: SetSource
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetSource_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>("testURL");
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->SetSource(source));
}
/**
 * @tc.name: FileFdSource_SubmitBufferingStart_0100
 * @tc.desc: FileFdSource_SubmitBufferingStart_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SubmitBufferingStart_0100, TestSize.Level1)
{
    std::shared_ptr<Plugins::Callback> sourceCallback = std::make_shared<SourceCallback>();
    fileFdSourcePlugin_->SubmitBufferingStart();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->SubmitBufferingStart();

    fileFdSourcePlugin_->PauseReadTimer();
    fileFdSourcePlugin_->SubmitBufferingStart();
    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    fileFdSourcePlugin_->SubmitBufferingStart();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
}
/**
 * @tc.name: FileFdSource_SubmitReadFail_0100
 * @tc.desc: FileFdSource_SubmitReadFail_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SubmitReadFail_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->SubmitReadFail();
    std::shared_ptr<Plugins::Callback> sourceCallback = std::make_shared<SourceCallback>();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->SubmitReadFail();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
    fileFdSourcePlugin_->SubmitReadFail();
}
/**
 * @tc.name: FileFdSource_StartTimerTask_0100
 * @tc.desc: FileFdSource_StartTimerTask_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_StartTimerTask_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->StartTimerTask();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
}
/**
 * @tc.name: FileFdSource_PauseDownloadTask_0100
 * @tc.desc: FileFdSource_PauseDownloadTask_0100
 * @tc.type: FUNC
 */
// HWTEST_F(FileFdSourceUnitTest, FileFdSource_PauseDownloadTask_0100, TestSize.Level1)
// {
//     fileFdSourcePlugin_->PauseDownloadTask(false); // branches

//     fileFdSourcePlugin_->SubmitBufferingStart();
//     fileFdSourcePlugin_->PauseDownloadTask(false);

//     fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
//     fileFdSourcePlugin_->PauseDownloadTask(false);

//     fileFdSourcePlugin_->SubmitBufferingStart();
//     fileFdSourcePlugin_->PauseDownloadTask(true);
//     EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
// }
/**
 * @tc.name: FileFdSource_PauseTimerTask_0100
 * @tc.desc: FileFdSource_PauseTimerTask_0100
 * @tc.type: FUNC
 */
// HWTEST_F(FileFdSourceUnitTest, FileFdSource_PauseTimerTask_0100, TestSize.Level1)
// {
//     fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
//     fileFdSourcePlugin_->PauseTimerTask();
//     EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
// }
/**
 * @tc.name: FileFdSource_HandleBuffering_0100
 * @tc.desc: FileFdSource_HandleBuffering_0100
 * @tc.type: FUNC
 */
// HWTEST_F(FileFdSourceUnitTest, FileFdSource_HandleBuffering_0100, TestSize.Level1)
// {
//     EXPECT_EQ(false, fileFdSourcePlugin_->HandleBuffering());
//     fileFdSourcePlugin_->SubmitBufferingStart();
//     EXPECT_EQ(true, fileFdSourcePlugin_->HandleBuffering());
// }
/**
 * @tc.name: FileFdSource_read_0100
 * @tc.desc: FileFdSource_read_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->SubmitBufferingStart();
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(0, buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_read_0200
 * @tc.desc: FileFdSource_read_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0200, TestSize.Level1)
{
    std::shared_ptr<Buffer> buffer = nullptr;
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(0, buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_ParseUriInfo_0100
 * @tc.desc: FileFdSource_ParseUriInfo_0100
 * @tc.type: FUNC
 */
// HWTEST_F(FileFdSourceUnitTest, FileFdSource_ParseUriInfo_0100, TestSize.Level1)
// {
//     std::string uri;
//     EXPECT_NE(Status::OK, fileFdSourcePlugin_->ParseUriInfo(uri));
// }
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
 * @tc.name: FileFdSource_PauseReadTimer_0100
 * @tc.desc: FileFdSource_PauseReadTimer_0100
 * @tc.type: FUNC
 */
// HWTEST_F(FileFdSourceUnitTest, FileFdSource_PauseReadTimer_0100, TestSize.Level1)
// {
//     fileFdSourcePlugin_->PauseReadTimer();
//     fileFdSourcePlugin_->Stop();
//     fileFdSourcePlugin_->PauseReadTimer();
//     EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
// }
/**
 * @tc.name: FileFdSource_SetBundleName_0100
 * @tc.desc: FileFdSource_SetBundleName_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetBundleName_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
}
/**
 * @tc.name: FileFdSource_CacheData_0100
 * @tc.desc: FileFdSource_CacheData_0100
 * @tc.type: FUNC
 */
// HWTEST_F(FileFdSourceUnitTest, FileFdSource_CacheData_0100, TestSize.Level1)
// {
//     fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
//     EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
// }
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS