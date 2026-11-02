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
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "http_source_plugin_stream_protocol_sniffer.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
class ProtocolSnifferUnitTest : public testing::Test {};

HWTEST_F(ProtocolSnifferUnitTest, XmlAndHlsNegativeClassification_001, TestSize.Level2)
{
    StreamProtocolSniffer sniffer;
    const std::vector<std::string> texts {
        "<MPDfake/>", "<mpd/>", "<other>MPD</other>", "MPD", "\xEF\xBB\xBF#EXTM3U", " #EXTM3U"};
    std::vector<StreamProtocolType> types;
    for (const auto& bytes : texts) {
        auto result = sniffer.Probe(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), true);
        EXPECT_FALSE(result.isFlv);
        EXPECT_FALSE(result.needMore);
        types.push_back(result.type);
    }
    EXPECT_EQ(types, std::vector<StreamProtocolType>(texts.size(), StreamProtocolType::HTTP));
}

HWTEST_F(ProtocolSnifferUnitTest, FlvVersionFlagsAndOffsetMatrix_002, TestSize.Level2)
{
    StreamProtocolSniffer sniffer;
    // Version 1, audio/video flags and a big-endian offset >= the nine-byte header are required.
    const std::vector<std::vector<uint8_t>> bytes {
        {'F', 'L', 'V', 1, 5, 0, 0, 0, 9}, {'F', 'L', 'V', 2, 5, 0, 0, 0, 9},
        {'F', 'L', 'V', 1, 0x80, 0, 0, 0, 9}, {'F', 'L', 'V', 1, 5, 0, 0, 0, 8},
        {'F', 'L', 'V', 1, 1, 0, 0, 1, 0}};
    std::vector<bool> flags;
    for (const auto& item : bytes) {
        flags.push_back(sniffer.Probe(item.data(), item.size(), true).isFlv);
    }
    EXPECT_EQ(flags, (std::vector<bool> {true, false, false, false, true}));
}

HWTEST_F(ProtocolSnifferUnitTest, XmlInstructionsDoctypeQuotesAndNamespacedRoot_003, TestSize.Level2)
{
    StreamProtocolSniffer sniffer;
    const std::vector<std::string> texts {
        "<?xml version=\"1.0\"?><?test value?><!--x--><MPD/>",
        "<!DOCTYPE MPD [<!ENTITY x \"a>b\">]><MPD/>",
        "\xEF\xBB\xBF \r\n<!-- MPD in a comment --><dash:MPD xmlns:dash=\"urn:test\"/>"};
    std::vector<StreamProtocolType> types;
    for (const auto& bytes : texts) {
        auto result = sniffer.Probe(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), true);
        EXPECT_FALSE(result.needMore);
        EXPECT_FALSE(result.isFlv);
        types.push_back(result.type);
    }
    EXPECT_EQ(types, std::vector<StreamProtocolType>(texts.size(), StreamProtocolType::DASH));
}

HWTEST_F(ProtocolSnifferUnitTest, CompleteSignaturesIgnoreMisleadingTrailingPayload_004, TestSize.Level2)
{
    StreamProtocolSniffer sniffer;
    const std::string hls = "#EXTM3U\n<!--not XML-->";
    const std::string dash = "<MPD><!--#EXTM3U--></MPD>";
    auto hlsResult = sniffer.Probe(reinterpret_cast<const uint8_t*>(hls.data()), hls.size(), true);
    auto dashResult = sniffer.Probe(reinterpret_cast<const uint8_t*>(dash.data()), dash.size(), true);
    EXPECT_EQ(hlsResult.type, StreamProtocolType::HLS);
    EXPECT_FALSE(hlsResult.needMore);
    EXPECT_EQ(dashResult.type, StreamProtocolType::DASH);
    EXPECT_FALSE(dashResult.needMore);
}
} // namespace OHOS::Media::Plugins::HttpPlugin
