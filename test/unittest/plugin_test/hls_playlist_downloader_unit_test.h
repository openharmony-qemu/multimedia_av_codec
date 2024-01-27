#include "hls_playlist_downloader.h"
#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HlsPlayListDownloaderUnitTest : public testing::Test {
public:
    HlsPlayListDownloaderUnitTest() {}

public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    std::make_shared<HlsPlayListDownloader> GetMediaDownloader();

    std::shared_ptr<PlayListDownloader> playListDownloader_;

}
}
}
}
