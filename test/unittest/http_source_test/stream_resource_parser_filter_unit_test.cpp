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

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include "http_source_plugin_stream_resource_parser.h"
#include "track_selection_filter_util.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

static const char* HLS_MASTER_3_VARIANTS =
    "#EXTM3U\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=500000,RESOLUTION=640x480,CODECS=\"avc1.42c01e\",FRAME-RATE=30\n"
    "480p/index.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=1500000,RESOLUTION=1280x720,CODECS=\"avc1.4d401f\",FRAME-RATE=30\n"
    "720p/index.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=3000000,RESOLUTION=1920x1080,CODECS=\"avc1.640028\",FRAME-RATE=30\n"
    "1080p/index.m3u8\n";

static const char* HLS_MASTER_WITH_AUDIO =
    "#EXTM3U\n"
    "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",NAME=\"English\",LANGUAGE=\"eng\","
    "URI=\"audio/eng/index.m3u8\"\n"
    "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",NAME=\"Chinese\",LANGUAGE=\"chi\","
    "URI=\"audio/chi/index.m3u8\"\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=3000000,RESOLUTION=1920x1080,CODECS=\"avc1.640028\",AUDIO=\"aud\"\n"
    "1080p/index.m3u8\n";

static const char* HLS_MASTER_WITH_SUBTITLES =
    "#EXTM3U\n"
    "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",NAME=\"English\",LANGUAGE=\"eng\","
    "URI=\"audio/eng/index.m3u8\"\n"
    "#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID=\"sub\",NAME=\"Chinese\",LANGUAGE=\"chi\","
    "URI=\"subtitle/chi/index.m3u8\"\n"
    "#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID=\"sub\",NAME=\"English\",LANGUAGE=\"eng\","
    "URI=\"subtitle/eng/index.m3u8\"\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=3000000,RESOLUTION=1920x1080,CODECS=\"avc1.640028\","
    "AUDIO=\"aud\",SUBTITLES=\"sub\"\n"
    "1080p/index.m3u8\n";

static const char* DASH_MPD_MULTI_STREAMS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\""
    " mediaPresentationDuration=\"PT60S\" minBufferTime=\"PT2S\">\n"
    "  <Period>\n"
    "    <AdaptationSet mimeType=\"video/mp4\" lang=\"und\">\n"
    "      <Representation id=\"v1\" bandwidth=\"500000\" width=\"640\" height=\"480\""
    " codecs=\"avc1.42c01e\" frameRate=\"30\">\n"
    "        <BaseURL>video/480p/</BaseURL>\n"
    "        <SegmentTemplate media=\"seg_$Number$.m4s\" startNumber=\"1\""
    " duration=\"2000000\" timescale=\"1000000\"/>\n"
    "      </Representation>\n"
    "      <Representation id=\"v2\" bandwidth=\"1500000\" width=\"1280\" height=\"720\""
    " codecs=\"avc1.4d401f\" frameRate=\"30\">\n"
    "        <BaseURL>video/720p/</BaseURL>\n"
    "        <SegmentTemplate media=\"seg_$Number$.m4s\" startNumber=\"1\""
    " duration=\"2000000\" timescale=\"1000000\"/>\n"
    "      </Representation>\n"
    "      <Representation id=\"v3\" bandwidth=\"3000000\" width=\"1920\" height=\"1080\""
    " codecs=\"avc1.640028\" frameRate=\"30\">\n"
    "        <BaseURL>video/1080p/</BaseURL>\n"
    "        <SegmentTemplate media=\"seg_$Number$.m4s\" startNumber=\"1\""
    " duration=\"2000000\" timescale=\"1000000\"/>\n"
    "      </Representation>\n"
    "    </AdaptationSet>\n"
    "    <AdaptationSet mimeType=\"audio/mp4\" lang=\"eng\">\n"
    "      <Representation id=\"a1\" bandwidth=\"128000\" codecs=\"mp4a.40.2\">\n"
    "        <BaseURL>audio/eng/</BaseURL>\n"
    "        <SegmentTemplate media=\"seg_$Number$.m4s\" startNumber=\"1\""
    " duration=\"2000000\" timescale=\"1000000\"/>\n"
    "      </Representation>\n"
    "    </AdaptationSet>\n"
    "    <AdaptationSet mimeType=\"audio/mp4\" lang=\"chi\">\n"
    "      <Representation id=\"a2\" bandwidth=\"128000\" codecs=\"mp4a.40.2\">\n"
    "        <BaseURL>audio/chi/</BaseURL>\n"
    "        <SegmentTemplate media=\"seg_$Number$.m4s\" startNumber=\"1\""
    " duration=\"2000000\" timescale=\"1000000\"/>\n"
    "      </Representation>\n"
    "    </AdaptationSet>\n"
    "  </Period>\n"
    "</MPD>\n";

class StreamResourceParserFilterTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void) {}
    void TearDown(void) {}

    std::unique_ptr<StreamResourceParser> ParseHls(const std::string& content, const TrackSelectionFilter& filter)
    {
        auto* raw = StreamResourceParser::Parse(
            reinterpret_cast<const uint8_t*>(content.data()), content.size(),
            StreamProtocolType::HLS, "https://example.com/master.m3u8", filter);
        return std::unique_ptr<StreamResourceParser>(raw);
    }

    std::unique_ptr<StreamResourceParser> ParseDash(const std::string& content, const TrackSelectionFilter& filter)
    {
        auto* raw = StreamResourceParser::Parse(
            reinterpret_cast<const uint8_t*>(content.data()), content.size(),
            StreamProtocolType::DASH, "https://example.com/manifest.mpd", filter);
        return std::unique_ptr<StreamResourceParser>(raw);
    }
};

// === P0: HLS video variant filtering ===

HWTEST_F(StreamResourceParserFilterTest, ParseHls_EmptyFilter_ReturnsAllVariants, TestSize.Level0)
{
    TrackSelectionFilter filter;
    auto parser = ParseHls(HLS_MASTER_3_VARIANTS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    EXPECT_EQ(resources.size(), 3);
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_MaxResolution720p_Returns480pAnd720p, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.maxVideoResolution = {1280, 720};
    auto parser = ParseHls(HLS_MASTER_3_VARIANTS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    EXPECT_EQ(resources.size(), 2);
    bool found480p = false;
    bool found720p = false;
    for (const auto& res : resources) {
        if (res.url.find("480p/index.m3u8") != std::string::npos) found480p = true;
        if (res.url.find("720p/index.m3u8") != std::string::npos) found720p = true;
    }
    EXPECT_TRUE(found480p);
    EXPECT_TRUE(found720p);
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_MaxBitrate1M_Returns480pOnly, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.maxVideoBitrate = 1000000;
    auto parser = ParseHls(HLS_MASTER_3_VARIANTS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    EXPECT_EQ(resources.size(), 1);
    EXPECT_NE(resources[0].url.find("480p/index.m3u8"), std::string::npos);
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_NoMatchFallback_ReturnsAll, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.maxVideoBitrate = 100;
    auto parser = ParseHls(HLS_MASTER_3_VARIANTS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    EXPECT_EQ(resources.size(), 3);
}

// === P0: HLS audio filtering + isSubPlaylist verification ===

HWTEST_F(StreamResourceParserFilterTest, ParseHls_PreferredAudioLanguageChi_ReturnsChiAudio, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.preferredAudioLanguages = {"chi"};
    auto parser = ParseHls(HLS_MASTER_WITH_AUDIO, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    bool foundChi = false;
    for (const auto& res : resources) {
        if (res.url.find("audio/chi") != std::string::npos) {
            foundChi = true;
        }
    }
    EXPECT_TRUE(foundChi);
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_PreferredAudioLanguageEng_NoChiAudio, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.preferredAudioLanguages = {"eng"};
    auto parser = ParseHls(HLS_MASTER_WITH_AUDIO, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    for (const auto& res : resources) {
        EXPECT_TRUE(res.url.find("audio/chi") == std::string::npos);
    }
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_AudioIsSubPlaylist_True, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.preferredAudioLanguages = {"chi"};
    auto parser = ParseHls(HLS_MASTER_WITH_AUDIO, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    for (const auto& res : resources) {
        if (res.url.find("audio/") != std::string::npos) {
            EXPECT_TRUE(res.isSubPlaylist) << "audio resource should have isSubPlaylist=true, url=" << res.url;
        }
    }
}

// === P0: HLS subtitle filtering + isSubPlaylist verification ===

HWTEST_F(StreamResourceParserFilterTest, ParseHls_SubtitleFiltering_ReturnsMatchingSubtitle, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.preferredSubtitleLanguages = {"chi"};
    auto parser = ParseHls(HLS_MASTER_WITH_SUBTITLES, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    bool foundChiSub = false;
    for (const auto& res : resources) {
        if (res.url.find("subtitle/chi") != std::string::npos) {
            foundChiSub = true;
        }
    }
    EXPECT_TRUE(foundChiSub);
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_SubtitleNoMatch_FallbackToAll, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.preferredSubtitleLanguages = {"jpn"};
    auto parser = ParseHls(HLS_MASTER_WITH_SUBTITLES, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    bool foundChiSub = false;
    bool foundEngSub = false;
    for (const auto& res : resources) {
        if (res.url.find("subtitle/chi") != std::string::npos) {
            foundChiSub = true;
        }
        if (res.url.find("subtitle/eng") != std::string::npos) {
            foundEngSub = true;
        }
    }
    EXPECT_TRUE(foundChiSub) << "fallback should return all subtitles including chi";
    EXPECT_TRUE(foundEngSub) << "fallback should return all subtitles including eng";
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_SubtitleIsSubPlaylist_True, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.preferredSubtitleLanguages = {"chi"};
    auto parser = ParseHls(HLS_MASTER_WITH_SUBTITLES, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    for (const auto& res : resources) {
        if (res.url.find("subtitle/") != std::string::npos) {
            EXPECT_TRUE(res.isSubPlaylist) << "subtitle resource should have isSubPlaylist=true, url=" << res.url;
        }
    }
}

// === P1: DASH filtering ===

HWTEST_F(StreamResourceParserFilterTest, ParseDash_EmptyFilter_ReturnsAllStreams, TestSize.Level1)
{
    TrackSelectionFilter filter;
    auto parser = ParseDash(DASH_MPD_MULTI_STREAMS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    EXPECT_FALSE(resources.empty());
}

HWTEST_F(StreamResourceParserFilterTest, ParseDash_FilterByMaxResolution_ReturnsMatchingVideo, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.maxVideoResolution = {1280, 720};
    auto parser = ParseDash(DASH_MPD_MULTI_STREAMS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    bool has1080p = false;
    bool has720p = false;
    bool has480p = false;
    for (const auto& res : resources) {
        if (res.url.find("video/1080p") != std::string::npos) has1080p = true;
        if (res.url.find("video/720p") != std::string::npos) has720p = true;
        if (res.url.find("video/480p") != std::string::npos) has480p = true;
    }
    EXPECT_FALSE(has1080p) << "1080p should be filtered out by maxRes=720p";
    EXPECT_TRUE(has720p) << "720p should be included";
    EXPECT_TRUE(has480p) << "480p should be included (below max)";
}

HWTEST_F(StreamResourceParserFilterTest, ParseDash_PerTypeFallback_VideoMatchAudioFallback, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.maxVideoResolution = {1280, 720};
    auto parser = ParseDash(DASH_MPD_MULTI_STREAMS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    bool hasEngAudio = false;
    bool hasChiAudio = false;
    for (const auto& res : resources) {
        if (res.url.find("audio/eng") != std::string::npos) hasEngAudio = true;
        if (res.url.find("audio/chi") != std::string::npos) hasChiAudio = true;
    }
    EXPECT_TRUE(hasEngAudio) << "audio should fallback to all (no audio filter set)";
    EXPECT_TRUE(hasChiAudio) << "audio should fallback to all (no audio filter set)";
}

HWTEST_F(StreamResourceParserFilterTest, ParseDash_NoMatchFallback_ReturnsAll, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.maxVideoBitrate = 100;
    auto parser = ParseDash(DASH_MPD_MULTI_STREAMS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    EXPECT_FALSE(resources.empty());
    bool hasVideo = false;
    bool hasAudio = false;
    for (const auto& res : resources) {
        if (res.url.find("video/") != std::string::npos) hasVideo = true;
        if (res.url.find("audio/") != std::string::npos) hasAudio = true;
    }
    EXPECT_TRUE(hasVideo) << "video should fallback to all";
    EXPECT_TRUE(hasAudio) << "audio should fallback to all";
}

// === P1: MatchPreferredMimeTypes ===

HWTEST_F(StreamResourceParserFilterTest, MatchPreferredMimeTypes_CodecPrefixMatch, TestSize.Level1)
{
    StreamInfo si;
    si.originCodecs = "avc1.640028";
    si.mimeType = "";
    std::vector<std::string> preferred = {"avc1"};
    EXPECT_TRUE(MatchPreferredMimeTypes(preferred, si));
}

HWTEST_F(StreamResourceParserFilterTest, MatchPreferredMimeTypes_ExactMimeMatch, TestSize.Level1)
{
    StreamInfo si;
    si.mimeType = "video/avc";
    si.originCodecs = "";
    std::vector<std::string> preferred = {"video/avc"};
    EXPECT_TRUE(MatchPreferredMimeTypes(preferred, si));
}

HWTEST_F(StreamResourceParserFilterTest, MatchPreferredMimeTypes_NoMatch, TestSize.Level1)
{
    StreamInfo si;
    si.mimeType = "video/avc";
    si.originCodecs = "avc1.640028";
    std::vector<std::string> preferred = {"video/hevc"};
    EXPECT_FALSE(MatchPreferredMimeTypes(preferred, si));
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_VideoNoMatchFallback_ReturnsAllVariants, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.maxVideoBitrate = 100;
    auto parser = ParseHls(HLS_MASTER_3_VARIANTS, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    EXPECT_EQ(resources.size(), 3);
    bool found480p = false;
    bool found720p = false;
    bool found1080p = false;
    for (const auto& res : resources) {
        if (res.url.find("480p/index.m3u8") != std::string::npos) found480p = true;
        if (res.url.find("720p/index.m3u8") != std::string::npos) found720p = true;
        if (res.url.find("1080p/index.m3u8") != std::string::npos) found1080p = true;
    }
    EXPECT_TRUE(found480p) << "fallback should return all variants including 480p";
    EXPECT_TRUE(found720p) << "fallback should return all variants including 720p";
    EXPECT_TRUE(found1080p) << "fallback should return all variants including 1080p";
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_AudioNoMatchFallback_ReturnsAllAudio, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.preferredAudioLanguages = {"jpn"};
    auto parser = ParseHls(HLS_MASTER_WITH_AUDIO, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    bool foundEng = false;
    bool foundChi = false;
    for (const auto& res : resources) {
        if (res.url.find("audio/eng") != std::string::npos) foundEng = true;
        if (res.url.find("audio/chi") != std::string::npos) foundChi = true;
    }
    EXPECT_TRUE(foundEng) << "fallback should return all audio including eng";
    EXPECT_TRUE(foundChi) << "fallback should return all audio including chi";
}

HWTEST_F(StreamResourceParserFilterTest, ParseHls_VideoMatch_AudioNoFilter_ReturnsAllAudio, TestSize.Level0)
{
    TrackSelectionFilter filter;
    filter.maxVideoResolution = {1280, 720};
    auto parser = ParseHls(HLS_MASTER_WITH_AUDIO, filter);
    ASSERT_NE(parser, nullptr);
    auto resources = parser->GetAll();
    bool foundAudio = false;
    for (const auto& res : resources) {
        if (res.url.find("audio/") != std::string::npos) {
            foundAudio = true;
        }
    }
    EXPECT_TRUE(foundAudio) << "audio has no filter, should fallback to all";
}

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS
