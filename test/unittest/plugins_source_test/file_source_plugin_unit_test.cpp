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
    fileSourcePlugin_ = std::make_shared<FileSourcePlugin>("SourceTest");
}

void FileFdSourceUnitTest::TearDown(void)
{
    fileSourcePlugin_ = nullptr;
}

/**
 * @tc.name: FileFdSource_Prepare_0100
 * @tc.desc: FileFdSource_Prepare_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_Prepare_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Prepare());
}
/**
 * @tc.name: FileFdSource_Reset_0100
 * @tc.desc: FileFdSource_Reset_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_Reset_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Reset());
}
/**
 * @tc.name: FileFdSource_GetParameter_0100
 * @tc.desc: FileFdSource_GetParameter_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_GetParameter_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    fileSourcePlugin_->GetParameter(meta);
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileFdSource_GetAllocator_0100
 * @tc.desc: FileFdSource_GetAllocator_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_GetAllocator_0100, TestSize.Level1)
{
    fileSourcePlugin_->GetAllocator();
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileFdSource_GetAllocator_0100
 * @tc.desc: FileFdSource_GetAllocator_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_GetAllocator_0100, TestSize.Level1)
{
    fileSourcePlugin_->GetAllocator();
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileFdSource_SetSource_0100
 * @tc.desc: FileFdSource_SetSource_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetSource_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>();
    EXPECT_NE(Status::OK, fileSourcePlugin_->SetSource(source));
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileFdSource_OpenFile_0100
 * @tc.desc: FileFdSource_OpenFile_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_OpenFile_0100, TestSize.Level1)
{
    EXPECT_NE(Status::OK, fileSourcePlugin_->OpenFile());
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileFdSource_ParseFileName_0100
 * @tc.desc: FileFdSource_ParseFileName_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_ParseFileName_0100, TestSize.Level1)
{
    std::string uri = "";
    EXPECT_NE(Status::OK, fileSourcePlugin_->ParseFileName(uri));
    uri = "file://#test";
    fileSourcePlugin_->ParseFileName(uri);
    uri = "file://#";
    fileSourcePlugin_->ParseFileName(uri);
    uri = "file://#";
    fileSourcePlugin_->ParseFileName(uri);
    uri = "file:///////////#";
    fileSourcePlugin_->ParseFileName(uri);
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS