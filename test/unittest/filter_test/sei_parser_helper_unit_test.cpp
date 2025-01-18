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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include "sei_parser_helper_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace {
const std::string TYPE_AVC = "video/avc";
const std::string TYPE_HEVC = "video/hevc";
const std::string MEDIA_ROOT = "/data/test/media/";
const std::string HEVC_BUFFER_ANNEXB_ONE_SEI = "hevc_0_buffer.data";
const std::string HEVC_BUFFER_ANNEXB_NO_SEI = "hevc_4266867_buffer.data";
const std::string AVC_BUFFER_ANNEXB_ONE_SEI = "avc_0_buffer.data";
const std::string AVC_BUFFER_ANNEXB_NO_SEI = "avc_56_buffer.data";
}  // namespace

namespace OHOS {
namespace Media {
namespace Pipeline {
void SeiParserHelperUnitTest::SetUpTestCase(void) {}

void SeiParserHelperUnitTest::TearDownTestCase(void) {}

std::shared_ptr<AVBuffer> ReadAVBufferFromLocalFile(const std::string &fileName)
{
    std::ifstream inputFile((MEDIA_ROOT + fileName).c_str(), std::ios::binary);
    if (!inputFile.is_open()) {
        return nullptr;
    }
    inputFile.seekg(0, std::ios::end);
    std::streampos fileSize = inputFile.tellg();

    std::cout << "fileSize: " << fileSize << std::endl;

    inputFile.seekg(0, std::ios::beg);

    AVBufferConfig config;
    config.size = fileSize;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    auto avBuffer = AVBuffer::CreateAVBuffer(config);
    if (avBuffer == nullptr || avBuffer->memory_ == nullptr || avBuffer->memory_->GetAddr() == nullptr) {
        return nullptr;
    }
    inputFile.read(reinterpret_cast<char *>(avBuffer->memory_->GetAddr()), fileSize);
    avBuffer->memory_->SetSize(fileSize);
    return avBuffer;
}

void SeiParserHelperUnitTest::SetUp(void) {}

void SeiParserHelperUnitTest::TearDown(void)
{
    seiParserHelper_ = nullptr;
}

/**
 * @tc.name: ParseSeiPayload_Hevc_001
 * @tc.desc: ParseSeiPayload_Hevc_001
 * @tc.type: FUNC
 */
HWTEST_F(SeiParserHelperUnitTest, ParseSeiPayload_Hevc_001, TestSize.Level1)
{
    seiParserHelper_ = SeiParserHelperFactory::CreateHelper(TYPE_HEVC);
    EXPECT_NE(seiParserHelper_, nullptr);

    auto buffer = ReadAVBufferFromLocalFile(HEVC_BUFFER_ANNEXB_ONE_SEI);
    EXPECT_NE(buffer, nullptr);

    seiParserHelper_->SetPayloadTypeVec({ 5 });

    std::shared_ptr<SeiPayloadInfoGroup> group = std::make_shared<SeiPayloadInfoGroup>();
    auto res = seiParserHelper_->ParseSeiPayload(buffer, group);

    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name: ParseSeiPayload_Hevc_002
 * @tc.desc: ParseSeiPayload_Hevc_002
 * @tc.type: FUNC
 */
HWTEST_F(SeiParserHelperUnitTest, ParseSeiPayload_Hevc_002, TestSize.Level1)
{
    seiParserHelper_ = SeiParserHelperFactory::CreateHelper(TYPE_HEVC);
    EXPECT_NE(seiParserHelper_, nullptr);

    auto buffer = ReadAVBufferFromLocalFile(HEVC_BUFFER_ANNEXB_ONE_SEI);
    EXPECT_NE(buffer, nullptr);

    seiParserHelper_->SetPayloadTypeVec({ 4 });

    std::shared_ptr<SeiPayloadInfoGroup> group = std::make_shared<SeiPayloadInfoGroup>();
    auto res = seiParserHelper_->ParseSeiPayload(buffer, group);

    EXPECT_NE(res, Status::OK);
}

/**
 * @tc.name: ParseSeiPayload_Hevc_003
 * @tc.desc: ParseSeiPayload_Hevc_003
 * @tc.type: FUNC
 */
HWTEST_F(SeiParserHelperUnitTest, ParseSeiPayload_Hevc_003, TestSize.Level1)
{
    seiParserHelper_ = SeiParserHelperFactory::CreateHelper(TYPE_AVC);
    EXPECT_NE(seiParserHelper_, nullptr);

    auto buffer = ReadAVBufferFromLocalFile(HEVC_BUFFER_ANNEXB_NO_SEI);
    EXPECT_TRUE(buffer != nullptr);

    seiParserHelper_->SetPayloadTypeVec({ 5 });

    std::shared_ptr<SeiPayloadInfoGroup> group = std::make_shared<SeiPayloadInfoGroup>();
    auto res = seiParserHelper_->ParseSeiPayload(buffer, group);

    EXPECT_NE(res, Status::OK);
}

/**
 * @tc.name: ParseSeiPayload_Avc_001
 * @tc.desc: ParseSeiPayload_Avc_001
 * @tc.type: FUNC
 */
HWTEST_F(SeiParserHelperUnitTest, ParseSeiPayload_Avc_001, TestSize.Level1)
{
    seiParserHelper_ = SeiParserHelperFactory::CreateHelper(TYPE_AVC);
    EXPECT_NE(seiParserHelper_, nullptr);

    auto buffer = ReadAVBufferFromLocalFile(AVC_BUFFER_ANNEXB_ONE_SEI);
    EXPECT_TRUE(buffer != nullptr);

    seiParserHelper_->SetPayloadTypeVec({ 5 });

    std::shared_ptr<SeiPayloadInfoGroup> group = std::make_shared<SeiPayloadInfoGroup>();
    auto res = seiParserHelper_->ParseSeiPayload(buffer, group);

    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name: ParseSeiPayload_Avc_002
 * @tc.desc: ParseSeiPayload_Avc_002
 * @tc.type: FUNC
 */
HWTEST_F(SeiParserHelperUnitTest, ParseSeiPayload_Avc_002, TestSize.Level1)
{
    seiParserHelper_ = SeiParserHelperFactory::CreateHelper(TYPE_AVC);
    EXPECT_TRUE(seiParserHelper_ != nullptr);

    auto buffer = ReadAVBufferFromLocalFile(AVC_BUFFER_ANNEXB_ONE_SEI);
    EXPECT_TRUE(buffer != nullptr);

    seiParserHelper_->SetPayloadTypeVec({ 4 });

    std::shared_ptr<SeiPayloadInfoGroup> group = std::make_shared<SeiPayloadInfoGroup>();
    auto res = seiParserHelper_->ParseSeiPayload(buffer, group);

    EXPECT_NE(res, Status::OK);
}

/**
 * @tc.name: ParseSeiPayload_Avc_003
 * @tc.desc: ParseSeiPayload_Avc_003
 * @tc.type: FUNC
 */
HWTEST_F(SeiParserHelperUnitTest, ParseSeiPayload_Avc_003, TestSize.Level1)
{
    seiParserHelper_ = SeiParserHelperFactory::CreateHelper(TYPE_AVC);
    EXPECT_TRUE(seiParserHelper_ != nullptr);

    auto buffer = ReadAVBufferFromLocalFile(AVC_BUFFER_ANNEXB_NO_SEI);
    EXPECT_TRUE(buffer != nullptr);

    seiParserHelper_->SetPayloadTypeVec({ 5 });

    std::shared_ptr<SeiPayloadInfoGroup> group = std::make_shared<SeiPayloadInfoGroup>();
    auto res = seiParserHelper_->ParseSeiPayload(buffer, group);

    EXPECT_NE(res, Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS