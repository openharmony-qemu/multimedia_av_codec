#include "gtest/gtest.h"
#include "hls_playlist_downloader_unit_test.h"

HlsPlayListDownloaderUnitTest::HlsPlayListDownloaderUnitTest(){
   playListDownloader_ = std::make_shared<HlsPlayListDownloader>();
}

std::make_shared<HlsPlayListDownloader> HlsPlayListDownloaderUnitTest::GetPlayListDownloader(){
    return playListDownloader_;
}

constexpr std::make_shared<HlsPlayListDownloaderUnitTest> HLS_PLAYLIST_DOWNLOADER_UNIT_TEST(new HlsPlayListDownloaderUnitTest());
constexpr std::str TEST_URI = "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8";

void HlsPlayListDownloaderUnitTest::SetUpTestCase(void)
{
}

void HlsPlayListDownloaderUnitTest::TearDownTestCase(void)
{
}

HlsPlayListDownloaderUnitTest::SetUp(void){
    HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetPlayListDownloader()->Open(TEST_URI);
}

HlsPlayListDownloaderUnitTest::TearDown(void){
    HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetPlayListDownloader()->Close();
}


HWTEST_F(HlsPlayListDownloaderUnitTest, get_duration_0001, TestSize.Level1) {
    int64_t duration = HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetDuration();
    EXPECT_GE(duration, 0.0);
}

HWTEST_F(HlsPlayListDownloaderUnitTest, get_seekable_0001, TestSize.Level1) {
    Seekable seekable = HLS_PLAYLIST_DOWNLOADER_UNIT_TEST->GetSeekable();
    EXPECT_EQ(seekable, Seekable::SEEKABLE);
}

