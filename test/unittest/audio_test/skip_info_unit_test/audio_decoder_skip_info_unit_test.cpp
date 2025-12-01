/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include "avcodec_codec_name.h"
#include "audio_decoder_capi_mock.h"

using namespace std;
using namespace testing::ext;
namespace OHOS {
namespace MediaAVCodec {
#define DEMO_LOG(...) printf(__VA_ARGS__);printf("\n")
constexpr std::string_view INPUT_FILE_PATH_MP3 = "/data/test/media/mp3_2c_44100hz_60k.dat";
constexpr std::string_view INPUT_FILE_PATH_VORBIS = "/data/test/media/vorbis_2c_44100hz_320k.dat";
class AudioDecoderSkipInfoUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp();
    void TearDown() {}
protected:
    int32_t FillInputData();
    int32_t CreateMp3Decoder();
    int32_t CreateVorbisDecoder();
    std::unique_ptr<AudioDecoderMockBase> capiMock_ = nullptr;
    std::unique_ptr<std::ifstream> inputFile_ = nullptr;
    std::vector<uint8_t> inData_;
    uint32_t inDataSize_ = 0;
    int64_t pts_ = 0;
};

int32_t AudioDecoderSkipInfoUnitTest::CreateMp3Decoder()
{
    capiMock_ = AudioDecoderMockBase::CreateDecoder();
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FILE_PATH_MP3, std::ios::binary);
    if (capiMock_->CreateByName(AVCodecCodecName::AUDIO_DECODER_MP3_NAME.data()) != 0) {
        return -1;
    }
    if (capiMock_->Start(44100, 2, nullptr) != 0) {  // 44100 2
        return -2;  // -2
    }
    return 0;
}

int32_t AudioDecoderSkipInfoUnitTest::CreateVorbisDecoder()
{
    int64_t size = 0;
    // capiMock_ = std::make_unique<AudioDecoderCapiMock>();
    capiMock_ = AudioDecoderMockBase::CreateDecoder();
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FILE_PATH_VORBIS, std::ios::binary);
    inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
    std::vector<uint8_t> codecConfig(size, 0);
    inputFile_->read(reinterpret_cast<char *>(codecConfig.data()), size);

    if (capiMock_->CreateByName(AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME.data()) != 0) {
        return -1;
    }
    if (capiMock_->Start(44100, 2, &codecConfig) != 0) {  // 44100 2
        return -2;  // -2
    }
    return 0;
}


void AudioDecoderSkipInfoUnitTest::SetUp()
{
    inData_.resize(10240);  // 10240
}

int32_t AudioDecoderSkipInfoUnitTest::FillInputData()
{
    if (inputFile_ == nullptr) {
        return -1;
    }
    int64_t size = 0;
    int64_t pts = 0;
    inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_->eof() || inputFile_->gcount() == 0 || size == 0) {
        // 文件读完了，调用stop
        return 1;
    }
    if (inputFile_->gcount() != sizeof(size) || size > 10240) {  // 10240
        std::cout << "FillInputData read size failed! size:" << size << std::endl;
        return -1;
    }
    inputFile_->read(reinterpret_cast<char *>(&pts), sizeof(pts));
    if (inputFile_->gcount() != sizeof(pts)) {
        std::cout << "FillInputData read pts failed!" << std::endl;
        return -1;
    }
    inputFile_->read(reinterpret_cast<char *>(inData_.data()), size);
    if (inputFile_->gcount() != size) {
        std::cout << "FillInputData read data failed!" << std::endl;
        return -1;
    }
    inDataSize_ = static_cast<uint32_t>(size);
    pts_ = pts;
    return 0;
}

/**
 * @tc.name: mp3_skip_start_001
 * @tc.desc: skip one frame start samples
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_start_001, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    std::vector<uint8_t> skipInfo {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
            DEMO_LOG("i:%d, outSize:%d", i, outSize);
        }
        if (i == testIndex) {
            EXPECT_EQ((normalPcmSize - (0X01 * sizeof(int16_t) * 2)), outSize);  // 2
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}

/**
 * @tc.name: mp3_skip_frame_002
 * @tc.desc: skip 2 frames start samples
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_start_002, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    // 跳过2303个样点，两帧 - 1个样点
    std::vector<uint8_t> skipInfo {0xff, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // 08ff -> 1152 * 2 - 1
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        if (i == testIndex) {
            EXPECT_EQ(outSize, 0);
        } else if (i == (testIndex + 1)) {
            EXPECT_EQ((0X01 * sizeof(int16_t) * 2), outSize);
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}


/**
 * @tc.name: mp3_skip_start_003
 * @tc.desc: skip 2 frames start samples
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_start_003, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    // 先跳2303，再跳1个样点
    std::vector<uint8_t> skipInfo {0xff, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    std::vector<uint8_t> skipInfo2 {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else if (i == testIndex + 1) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo2);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        if (i == testIndex) {
            EXPECT_EQ(outSize, 0);
        } else if (i == (testIndex + 1)) {
            EXPECT_EQ(normalPcmSize - (0X01 * sizeof(int16_t) * 2), outSize);
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}

/**
 * @tc.name: mp3_skip_end_001
 * @tc.desc: skip one frame end samples
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_end_001, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    std::vector<uint8_t> skipInfo {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        if (i == testIndex) {
            EXPECT_EQ((normalPcmSize - (0X02 * sizeof(int16_t) * 2)), outSize);  // 2
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}

/**
 * @tc.name: mp3_skip_end_002
 * @tc.desc: end == 1 frame
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_end_002, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    // end == 1 frame, skip 1 frame
    std::vector<uint8_t> skipInfo {0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00};  // 08ff -> 1152 * 2 - 1
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        if (i == testIndex) {
            EXPECT_EQ(outSize, 0);
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}

/**
 * @tc.name: mp3_skip_end_003
 * @tc.desc: end > 1 frame, not skip
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_end_003, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    // end > 1 frame, not skip
    std::vector<uint8_t> skipInfo {0x00, 0x00, 0x00, 0x00, 0x81, 0x04, 0x00, 0x00, 0x00, 0x00};  // 08ff -> 1152 * 2 - 1
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        ASSERT_EQ(normalPcmSize, outSize);
    }
}

/**
 * @tc.name: mp3_skip_start_end_001
 * @tc.desc: skip one frame start and end samples
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_start_end_001, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    // start + end < 1 frame, skip start + end
    std::vector<uint8_t> skipInfo {0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        if (i == testIndex) {
            EXPECT_EQ((normalPcmSize - (0X03 * sizeof(int16_t) * 2)), outSize);  // 2
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}

/**
 * @tc.name: mp3_skip_start_end_002
 * @tc.desc: start + end > 1 frame
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_start_end_002, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    // start + end > 1 frame, only skip start
    std::vector<uint8_t> skipInfo {0x7f, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        if (i == testIndex) {
            EXPECT_EQ((0x01 * sizeof(int16_t) * 2), outSize);  // 2
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}

/**
 * @tc.name: mp3_skip_start_end_003
 * @tc.desc: start + end <= 1 frame
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, mp3_skip_start_end_003, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateMp3Decoder(), 0);
    int32_t outSize = 0;
    // start + end = 1 frame, skip start + end
    std::vector<uint8_t> skipInfo {0x7f, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    int32_t normalPcmSize = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);
        
        if (i == 1) {
            normalPcmSize = outSize;
        }
        if (i == testIndex) {
            EXPECT_EQ(0, outSize);
        } else {
            ASSERT_EQ(normalPcmSize, outSize);
        }
    }
}

/**
 * @tc.name: vorbis_skip_start_001
 * @tc.desc: skip one frame start samples
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, vorbis_skip_start_001, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateVorbisDecoder(), 0);
    int32_t outSize = 0;
    std::vector<uint8_t> skipInfo {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    int32_t normalPcmSize = 4096; // dec out len: 4096
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);

        if (i == testIndex) {
            EXPECT_EQ((normalPcmSize - (0X01 * sizeof(int16_t) * 2)), outSize);  // 2
        }
    }
}

/**
 * @tc.name: vorbis_skip_start_002
 * @tc.desc: skip 2 frames start samples
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, vorbis_skip_start_002, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateVorbisDecoder(), 0);
    int32_t outSize = 0;
    // 跳过两帧 - 1个样点, 1帧4096字节 -> 1024样点 -> 0x400
    std::vector<uint8_t> skipInfo {0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);

        if (i == testIndex) {
            EXPECT_EQ(outSize, 0);
        } else if (i == (testIndex + 1)) {
            EXPECT_EQ((0X01 * sizeof(int16_t) * 2), outSize);
        }
    }
}

/**
 * @tc.name: vorbis_skip_end_001
 * @tc.desc: skip frames end = 1 sample
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, vorbis_skip_end_001, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateVorbisDecoder(), 0);
    int32_t outSize = 0;
    // 跳过两帧 - 1个样点
    std::vector<uint8_t> skipInfo {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);

        if (i == testIndex) {
            EXPECT_EQ(outSize, 0);  // 2
        } else if (i == (testIndex + 1)) {
            EXPECT_EQ(outSize, 4096); // 4096
        }
    }
}

/**
 * @tc.name: vorbis_skip_end_002
 * @tc.desc: skip frames end > 1 sample
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, vorbis_skip_end_002, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateVorbisDecoder(), 0);
    int32_t outSize = 0;
    // 跳过两帧 - 1个样点
    std::vector<uint8_t> skipInfo {0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);

        if (i == testIndex) {
            EXPECT_EQ(outSize, 4096); // 4096
        }
    }
}

/**
 * @tc.name: vorbis_skip_all_001
 * @tc.desc: skip frames start + end = 1 sample
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, vorbis_skip_all_001, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateVorbisDecoder(), 0);
    int32_t outSize = 0;
    // 跳过两帧 - 1个样点
    std::vector<uint8_t> skipInfo {0x01, 0x00, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);

        if (i == testIndex) {
            EXPECT_EQ(outSize, 0);  // 2
        } else if (i == (testIndex + 1)) {
            EXPECT_EQ(outSize, 4096); // 4096
        }
    }
}

/**
 * @tc.name: vorbis_skip_all_002
 * @tc.desc: skip frames start + end > 1 sample
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecoderSkipInfoUnitTest, vorbis_skip_all_002, TestSize.Level0)
{
    constexpr int32_t testIndex = 500;
    ASSERT_EQ(CreateVorbisDecoder(), 0);
    int32_t outSize = 0;
    // 跳过两帧 - 1个样点
    std::vector<uint8_t> skipInfo {0x02, 0x00, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00};
    int32_t i = 0;
    while (FillInputData() == 0) {
        ++i;
        outSize = 0;
        if (i == testIndex) {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, &skipInfo);
        } else {
            capiMock_->DecodeInput(inData_.data(), inDataSize_, nullptr);
        }
        capiMock_->DecodeOutput(nullptr, outSize);

        if (i == testIndex) {
            EXPECT_EQ(outSize, 4096 - (0x02 * sizeof(int16_t) * 2)); // 4096 2
        }
    }
}
} // namespace MediaAVCodec
} // namespace OHOS