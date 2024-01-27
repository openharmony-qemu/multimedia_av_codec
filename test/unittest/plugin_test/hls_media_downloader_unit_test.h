#include "hls_media_downloader.h"
#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HlsMediaDownloaderUnitTest : public testing::Test { 
public:
    HlsMediaDownloaderUnitTest() {}

public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    std::make_shared<HlsMediaDownloader> GetMediaDownloader();

    std::shared_ptr<HlsMediaDownloader> hlsMediaDownloader_;

}
}
}
}
}