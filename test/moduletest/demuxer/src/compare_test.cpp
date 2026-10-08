/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
 
#include "gtest/gtest.h"
#include <fcntl.h>
#include "native_avdemuxer.h"
#include <fstream>
#include "nlohmann/json.hpp"
#include "syspara/parameters.h"
#include "openssl/sha.h"
#include "openssl/crypto.h"
#include <iomanip>
 
namespace OHOS {
namespace Media {
class DemuxerCompareNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
private:
    bool useMpeg4_ = true;
};
 
std::map<uint32_t, int32_t> frames;
std::map<uint32_t, int32_t> kFrames;
std::map<uint32_t, bool> eosFlag;
std::vector<uint32_t> selectedTrackIds;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
 
static int32_t g_trackCount;
uint8_t g_md[SHA512_DIGEST_LENGTH];
SHA512_CTX g_c;
constexpr int32_t THOUSAND = 1000.0;
static int32_t g_sixteen = 16;
 
void DemuxerCompareNdkTest::SetUpTestCase() {}
void DemuxerCompareNdkTest::TearDownTestCase() {}
void DemuxerCompareNdkTest::SetUp()
{
    useMpeg4_ = (OHOS::system::GetParameter("Mpeg4DemuxerPlugin.skip", "0") != "1");
    g_trackCount = 0;
}
void DemuxerCompareNdkTest::TearDown()
{
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
 
    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (avBuffer != nullptr) {
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    if (!frames.empty()) {
        frames.clear();
    }
    if (!kFrames.empty()) {
        kFrames.clear();
    }
    if (!eosFlag.empty()) {
        eosFlag.clear();
    }
    if (!selectedTrackIds.empty()) {
        selectedTrackIds.clear();
    }
}
} // namespace Media
} // namespace OHOS
 
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
 
static int64_t GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}
 
struct seekInfo {
    const char *fileName;
    OH_AVSeekMode seekmode;
    int64_t millisecond;
    int32_t videoCount;
    int32_t audioCount;
    int32_t subtitleCount;
};
 
struct FrameNum {
    int32_t videoFrameNum;
    int32_t audioFrameNum;
    int32_t metaFrameNum;
    int32_t dataFrameNum;
};
 
 
static void CheckSeekMode(seekInfo seekInfo)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    int fd = open(seekInfo.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo.fileName);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        avBuffer = OH_AVBuffer_Create(size);
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
        bool readEnd = false;
        int32_t frameNum = 0;
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                readEnd = true;
                break;
            }
            frameNum++;
        }
        if (tarckType == MEDIA_TYPE_VID) {
            ASSERT_EQ(seekInfo.videoCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(seekInfo.audioCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_SUBTITLE) {
            ASSERT_EQ(seekInfo.subtitleCount, frameNum);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    close(fd);
    fd = -1;
}
 
static bool isEos(map<uint32_t, bool>&eosMap)
{
    for (auto iter = eosMap.begin(); iter != eosMap.end(); ++iter) {
        if (!iter->second) {
            return false;
        }
    }
    return true;
}
 
 
static bool Init(const char *file, int32_t trackNum, int &fd, int64_t &size)
{
    fd = open(file, O_RDONLY);
    size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        return false;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (demuxer == nullptr) {
        return false;
    }
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    if (sourceFormat == nullptr) {
        return false;
    }
    if (!OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount)) {
        return false;
    }
    if (trackNum != g_trackCount) {
        return false;
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        if (trackFormat == nullptr) {
            return false;
        }
        const char* codeMime = "";
        OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &codeMime);
        if (OH_AVDemuxer_SelectTrackByID(demuxer, index) != AV_ERR_OK) {
            if (std::string(codeMime) != "invalid") {
                return false;
            } else {
                continue;
            }
        }
        selectedTrackIds.push_back(static_cast<uint32_t>(index));
        frames[index] = 0;
        kFrames[index] = 0;
        eosFlag[index] = false;
    }
    return true;
}
 
std::string ExtractFileName(const std::string& path)
{
    size_t found = path.find_last_of("/\\");
    if (found != std::string::npos) {
        return path.substr(found + 1);
    }
    return path;
}
 
bool EosFlag(OH_AVCodecBufferAttr &attr, uint32_t &index)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        SHA512_Final(g_md, &g_c);
        OPENSSL_cleanse(&g_c, sizeof(g_c));
        eosFlag[index] = true;
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
        return false;
    } else {
        SHA512_Update(&g_c, OH_AVBuffer_GetAddr(avBuffer), attr.size);
        frames[index]++;
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            kFrames[index]++;
        }
    }
    return true;
}
 
void FrameCount(OH_AVCodecBufferAttr &attr, bool &readEnd, int &tarckType, FrameNum& num)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        readEnd = true;
        return;
    }
    if (tarckType == MEDIA_TYPE_VID) {
        num.videoFrameNum++;
    } else if (tarckType == MEDIA_TYPE_AUD) {
        num.audioFrameNum++;
    } else if (tarckType == MEDIA_TYPE_TIMED_METADATA) {
        num.metaFrameNum++;
    }
}
 
void FrameDataVideoCount(OH_AVCodecBufferAttr &attr, bool &readEnd, int &tarckType, FrameNum& num)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        readEnd = true;
        return;
    }
    if (tarckType == MEDIA_TYPE_TIMED_METADATA) {
        num.dataFrameNum++;
    } else {
        num.videoFrameNum++;
    }
}
 
bool Read(int &fd, int64_t &size, std::string& fileName)
{
    OH_AVCodecBufferAttr attr;
    std::ifstream f("/data/test/media/hashValue.json", ios::in);
    nlohmann::json data = nlohmann::json::parse(f);
    int32_t attrValueIndex = 0;
    while (!isEos(eosFlag)) {
        for (auto index : selectedTrackIds) {
            avBuffer = OH_AVBuffer_Create(size);
            if (avBuffer == nullptr) {
                cout << "avBuffer is nullptr" << endl;
                return false;
            }
            if (OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer) != AV_ERR_OK) {
                cout << "OH_AVDemuxer_ReadSampleBuffer is faile" << endl;
                return false;
            }
            if (OH_AVBuffer_GetBufferAttr(avBuffer, &attr) != AV_ERR_OK) {
                cout << "OH_AVBuffer_GetBufferAttr is faile" << endl;
                return false;
            }
            if (data[fileName.c_str()]["bufferAttr"][attrValueIndex]["pts"] != attr.pts) {
                return false;
            }
            if (data[fileName.c_str()]["bufferAttr"][attrValueIndex]["size"] != attr.size) {
                return false;
            }
            if (data[fileName.c_str()]["bufferAttr"][attrValueIndex]["offset"] != attr.offset) {
                return false;
            }
            if (data[fileName.c_str()]["bufferAttr"][attrValueIndex]["flags"] != attr.flags) {
                return false;
            }
            attrValueIndex++;
            if (!EosFlag(attr, index)) {
                continue;
            }
            OH_AVBuffer_Destroy(avBuffer);
            avBuffer = nullptr;
        }
    }
    close(fd);
    fd = -1;
    return true;
}
 
std::vector<uint8_t> LoadHashValue(std::string filename)
{
    std::ifstream f("/data/test/media/hashValue.json", ios::in);
    std::vector<uint8_t> ret;
    if (f) {
        nlohmann::json data =  nlohmann::json::parse(f);
        std::string hashValue = data[filename.c_str()]["hashValue"];
        std::stringstream ss(hashValue);
        std::string item;
        while (getline(ss, item, ',')) {
            if (!item.empty()) {
                ret.push_back(stol(item, nullptr, g_sixteen));
            }
        }
    }
    return ret;
}
 
static void DumpHashValue(std::vector<uint8_t> &srcHashVal, uint8_t outputHashVal[])
{
    printf("-----------output hash value-----------\n");
    for (int i = 1; i < SHA512_DIGEST_LENGTH + 1; i++) {
        printf("%02x,", outputHashVal[i - 1]);
        if (i % g_sixteen == 0) {
            printf("\n");
        }
    }
    printf("-----------standard hash value-----------\n");
    for (int i = 1; i < SHA512_DIGEST_LENGTH + 1; i++) {
        printf("%02x,", srcHashVal[i - 1]);
        if (i % g_sixteen == 0) {
            printf("\n");
        }
    }
}
 
bool MdCompare(uint8_t hash[], std::string filename)
{
    std::vector<uint8_t> srcHashVal = LoadHashValue(filename);
    DumpHashValue(srcHashVal, hash);
    if (srcHashVal.size() != SHA512_DIGEST_LENGTH) {
        cout << "get hash value failed, size " << srcHashVal.size() << endl;
        return false;
    }
    for (int32_t i = 0; i < SHA512_DIGEST_LENGTH; i++) {
        if (hash[i] != srcHashVal[i]) {
            cout << "hash value mismatch" << endl;
            return false;
        }
    }
    return true;
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0001
 * @tc.name      : aac_mpeg4_subtitle.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0001, TestSize.Level0)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/aac_mpeg4_subtitle.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 3, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0002
 * @tc.name      : AV_FLIP_H_90.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0002, TestSize.Level1)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/rotation/AV_FLIP_H_90.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 2, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0003
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0003, TestSize.Level1)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_2_layer_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 1, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0004
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0004, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/Muxer_Auxiliary.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 4, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0005
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0005, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_3_layer_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 1, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0006
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0006, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_007
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0007, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_4_layer_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 1, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_008
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0008, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_009
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0009, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_all_i_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 2, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0010
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0010, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_all_i_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 2, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0011
 * @tc.name      : demuxer_parser_2_layer_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0011, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_hdr_1_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 2, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0012
 * @tc.name      : demuxer_parser_hdr_2_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0012, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_hdr_2_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 2, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0013
 * @tc.name      : demuxer_parser_hdr_vivid.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0013, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_hdr_vivid.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0014
 * @tc.name      : demuxer_parser_hdr_vivid_gltf.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0014, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_hdr_vivid_gltf.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0015
 * @tc.name      : demuxer_parser_ipb_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0015, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_ipb_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 2, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0016
 * @tc.name      : demuxer_parser_one_i_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0016, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_one_i_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 2, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0017
 * @tc.name      : demuxer_parser_ltr_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0017, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_ltr_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 1, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0018
 * @tc.name      : demuxer_parser_ltr_frame_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0018, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_ltr_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0019
 * @tc.name      : demuxer_parser_ipb_frame_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0019, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_ipb_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 2, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0020
 * @tc.name      : demuxer_parser_one_i_frame_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0020, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_one_i_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 2, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0021
 * @tc.name      : demuxer_parser_one_i_frame_no_audio_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0021, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 1, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0022
 * @tc.name      : demuxer_parser_one_i_frame_no_audio_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0022, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0023
 * @tc.name      : demuxer_parser_recording.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0023, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_recording.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 2, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0024
 * @tc.name      : demuxer_parser_sdtp_extended_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0024, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_sdtp_extended_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 3, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0025
 * @tc.name      : demuxer_parser_sdtp_frame_avc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0025, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/demuxer_parser_sdtp_frame_avc.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 2, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0026
 * @tc.name      : demuxer_parser_sdtp_frame_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0026, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 2, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0027
 * @tc.name      : record_from_ios.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0027, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/record_from_ios.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 5, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0028
 * @tc.name      : sarnot1.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0028, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/sarnot1.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 2, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0029
 * @tc.name      : vvc_1280_720_8.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0029, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/vvc_1280_720_8.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0030
 * @tc.name      : Timedmetadata1Track2.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0030, TestSize.Level2)
{
    SHA512_Init(&g_c);
    int fd = 0;
    int64_t size = 0;
    const char* file = "/data/test/media/Timedmetadata1Track2.mp4";
    std::string fileName = ExtractFileName(std::string(file));
    ASSERT_TRUE(Init(file, 3, fd, size));
    ASSERT_TRUE(Read(fd, size, fileName));
    MdCompare(g_md, fileName);
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0031
 * @tc.name      : video_9audio.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0031, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/video_9audio.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 10, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_COMPARE_TEST_0032
 * @tc.name      : /demuxer_parser_2_layer_frame_hevc.mp4 cpmpare hash, attr value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_COMPARE_TEST_0032, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        SHA512_Init(&g_c);
        int fd = 0;
        int64_t size = 0;
        const char* file = "/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4";
        std::string fileName = ExtractFileName(std::string(file));
        ASSERT_TRUE(Init(file, 1, fd, size));
        ASSERT_TRUE(Read(fd, size, fileName));
        MdCompare(g_md, fileName);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0001
 * @tc.name      : 精准seek, SEEK_MODE_NEXT_SYNC, aac_mpeg4_subtitle.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0001, TestSize.Level0)
{
    seekInfo fileTest{"/data/test/media/aac_mpeg4_subtitle.mp4", SEEK_MODE_NEXT_SYNC, 455667, 348, 491, 14};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0002
 * @tc.name      : 精准seek, SEEK_MODE_PREVIOUS_SYNC, AV_FLIP_H_90.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0002, TestSize.Level1)
{
    seekInfo fileTest{"/data/test/media/rotation/AV_FLIP_H_90.mp4", SEEK_MODE_PREVIOUS_SYNC, 1332889, 74, 112, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0003
 * @tc.name      : 精准seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_2_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0003, TestSize.Level1)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 10033333,
        30, 0, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0004
 * @tc.name      : 首帧seek, SEEK_MODE_NEXT_SYNC, Muxer_Auxiliary.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0004, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        int tarckType = 0;
        OH_AVCodecBufferAttr attr;
        int fd = open("/data/test/media/Muxer_Auxiliary.mp4", O_RDONLY);
        int64_t size = GetFileSize("/data/test/media/Muxer_Auxiliary.mp4");
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        cout << "g_trackCount----" << g_trackCount << endl;
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        FrameNum num{0, 0, 0, 0};
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, 0, SEEK_MODE_NEXT_SYNC));
            bool readEnd = false;
            
            while (!readEnd) {
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
                ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
                FrameDataVideoCount(attr, readEnd, tarckType, num);
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            OH_AVBuffer_Destroy(avBuffer);
            avBuffer = nullptr;
        }
        ASSERT_EQ(109, num.videoFrameNum);
        ASSERT_EQ(16, num.dataFrameNum);
        close(fd);
        fd = -1;
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0005
 * @tc.name      : 首帧seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_3_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0005, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0003
 * @tc.name      : 首帧seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_3_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0006, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330,
            0, 0};
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0007
 * @tc.name      : 末帧seek, SEEK_MODE_NEXT_SYNC, demuxer_parser_4_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0007, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 9700000, 30, 0, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0008
 * @tc.name      : 末帧seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_4_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0008, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 10966667,
            30, 0, 0};
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0009
 * @tc.name      : 末帧seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_all_i_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0009, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 6966666, 1, 2, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0010
 * @tc.name      : 超范围seek, SEEK_MODE_NEXT_SYNC, demuxer_parser_all_i_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0010, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        int fd = open("/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", O_RDONLY);
        int64_t size = GetFileSize("/data/test/media/demuxer_parser_all_i_frame_hevc.mp4");
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        cout << "g_trackCount----" << g_trackCount << endl;
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_UNKNOWN, OH_AVDemuxer_SeekToTime(demuxer, 7000, SEEK_MODE_NEXT_SYNC));
        }
        close(fd);
        fd = -1;
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0011
 * @tc.name      : 超范围seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_hdr_1_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0011, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        int fd = open("/data/test/media/demuxer_parser_hdr_1_hevc.mp4", O_RDONLY);
        int64_t size = GetFileSize("/data/test/media/demuxer_parser_hdr_1_hevc.mp4");
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        cout << "g_trackCount----" << g_trackCount << endl;
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_SeekToTime(demuxer, 5100, SEEK_MODE_PREVIOUS_SYNC));
        }
        close(fd);
        fd = -1;
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0012
 * @tc.name      : 超范围seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_hdr_2_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0012, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        int fd = open("/data/test/media/demuxer_parser_hdr_2_hevc.mp4", O_RDONLY);
        int64_t size = GetFileSize("/data/test/media/demuxer_parser_hdr_2_hevc.mp4");
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        cout << "g_trackCount----" << g_trackCount << endl;
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_SeekToTime(demuxer, 5100, SEEK_MODE_CLOSEST_SYNC));
        }
        close(fd);
        fd = -1;
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0013
 * @tc.name      : 精准seek, SEEK_MODE_NEXT_SYNC, demuxer_parser_hdr_vivid.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0013, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_hdr_vivid.mp4", SEEK_MODE_NEXT_SYNC, 3268500, 80, 0, 0};
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0014
 * @tc.name      : 精准seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_hdr_vivid_gltf.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0014, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_hdr_vivid_gltf.mp4", SEEK_MODE_PREVIOUS_SYNC,
            33356, 2, 0, 0};
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0015
 * @tc.name      : 精准seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_ipb_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0015, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 12033333, 12, 19, 0};
    if (useMpeg4_) {
        fileTest = {"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 12033333, 12, 16, 0};
    }
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0016
 * @tc.name      : 首帧seek, SEEK_MODE_NEXT_SYNC, demuxer_parser_one_i_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0016, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0017
 * @tc.name      : 首帧seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_ltr_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0017, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0018
 * @tc.name      : 首帧seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_ltr_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0018, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0019
 * @tc.name      : 末帧seek, SEEK_MODE_NEXT_SYNC, demuxer_parser_ipb_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0019, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC,
            11556333, 22, 32, 0};
        if (useMpeg4_) {
            fileTest = {"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 11556333, 22, 29, 0};
        }
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0020
 * @tc.name      : 末帧seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_one_i_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0020, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
            12356333, 372, 525, 0};
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0021
 * @tc.name      : 末帧seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_one_i_frame_no_audio_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0021, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
        12366667, 372, 3, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0022
 * @tc.name      : 超范围seek, SEEK_MODE_NEXT_SYNC, demuxer_parser_one_i_frame_no_audio_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0022, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        int fd = open("/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", O_RDONLY);
        int64_t size = GetFileSize("/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4");
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        cout << "g_trackCount----" << g_trackCount << endl;
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_UNKNOWN, OH_AVDemuxer_SeekToTime(demuxer, 12323, SEEK_MODE_NEXT_SYNC));
        }
        close(fd);
        fd = -1;
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0023
 * @tc.name      : 超范围seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_recording.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0023, TestSize.Level2)
{
    int fd = open("/data/test/media/demuxer_parser_recording.mp4", O_RDONLY);
    int64_t size = GetFileSize("/data/test/media/demuxer_parser_recording.mp4");
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_SeekToTime(demuxer, 7200, SEEK_MODE_PREVIOUS_SYNC));
    }
    close(fd);
    fd = -1;
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0024
 * @tc.name      : 超范围seek, SEEK_MODE_CLOSEST_SYNC, demuxer_parser_sdtp_extended_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0024, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        int fd = open("/data/test/media/demuxer_parser_sdtp_extended_hevc.mp4", O_RDONLY);
        int64_t size = GetFileSize("/data/test/media/demuxer_parser_sdtp_extended_hevc.mp4");
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        cout << "g_trackCount----" << g_trackCount << endl;
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_SeekToTime(demuxer, 8000, SEEK_MODE_CLOSEST_SYNC));
        }
        close(fd);
        fd = -1;
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0025
 * @tc.name      : 精准seek, SEEK_MODE_NEXT_SYNC, demuxer_parser_sdtp_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0025, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 8200000, 122, 175, 0};
    if (useMpeg4_) {
        fileTest = {"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 8200000, 122, 173, 0};
    }
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0026
 * @tc.name      : 精准seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_sdtp_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0026, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
            8366667, 126, 179, 0};
        if (useMpeg4_) {
            fileTest = {"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
                8366667, 126, 174, 0};
        }
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0027
 * @tc.name      : 精准seek, SEEK_MODE_CLOSEST_SYNC, record_from_ios.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0027, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    int fd = open("/data/test/media/record_from_ios.mp4", O_RDONLY);
    int64_t size = GetFileSize("/data/test/media/record_from_ios.mp4");
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
    for (int32_t index = 0; index < 2; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int32_t videoFrameNum = 0;
    int32_t audioFrameNum = 0;
    for (int32_t index = 0; index < 2; index++) {
        avBuffer = OH_AVBuffer_Create(size);
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, 66, SEEK_MODE_CLOSEST_SYNC));
        bool readEnd = false;
        
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                readEnd = true;
                break;
            }
            if (tarckType == MEDIA_TYPE_VID) {
                videoFrameNum++;
            } else if (tarckType == MEDIA_TYPE_AUD) {
                audioFrameNum++;
            }
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    ASSERT_EQ(324, videoFrameNum);
    ASSERT_EQ(468, audioFrameNum);
 
    close(fd);
    fd = -1;
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0028
 * @tc.name      : 首帧seek, SEEK_MODE_NEXT_SYNC, sarnot1.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0028, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/sarnot1.mp4", SEEK_MODE_NEXT_SYNC, 0, 60, 95, 0};
    CheckSeekMode(fileTest);
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0029
 * @tc.name      : 首帧seek, SEEK_MODE_PREVIOUS_SYNC, vvc_1280_720_8.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0029, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/vvc_1280_720_8.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 254, 0, 0};
        CheckSeekMode(fileTest);
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0030
 * @tc.name      : 首帧seek, SEEK_MODE_CLOSEST_SYNC, Timedmetadata1Track2.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0030, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    int fd = open("/data/test/media/Timedmetadata1Track2.mp4", O_RDONLY);
    int64_t size = GetFileSize("/data/test/media/Timedmetadata1Track2.mp4");
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    FrameNum num{0, 0, 0, 0};
    for (int32_t index = 0; index < g_trackCount; index++) {
        avBuffer = OH_AVBuffer_Create(size);
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, 0, SEEK_MODE_CLOSEST_SYNC));
        bool readEnd = false;
        
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            FrameCount(attr, readEnd, tarckType, num);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    ASSERT_EQ(210, num.videoFrameNum);
    ASSERT_EQ(211, num.audioFrameNum);
    ASSERT_EQ(210, num.metaFrameNum);
    close(fd);
    fd = -1;
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0031
 * @tc.name      : 末帧seek, SEEK_MODE_NEXT_SYNC, video_9audio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0031, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        int tarckType = 0;
        OH_AVCodecBufferAttr attr;
        int fd = open("/data/test/media/video_9audio.mp4", O_RDONLY);
        int64_t size = GetFileSize("/data/test/media/video_9audio.mp4");
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        cout << "g_trackCount----" << g_trackCount << endl;
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        FrameNum num{0, 0, 0, 0};
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, 4983, SEEK_MODE_NEXT_SYNC));
            bool readEnd = false;
            
            while (!readEnd) {
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
                ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
                FrameCount(attr, readEnd, tarckType, num);
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            OH_AVBuffer_Destroy(avBuffer);
            avBuffer = nullptr;
        }
        if (!useMpeg4_) {
            ASSERT_EQ(105, num.videoFrameNum);
            ASSERT_EQ(684, num.audioFrameNum);
        } else {
            ASSERT_EQ(105, num.videoFrameNum);
            ASSERT_EQ(657, num.audioFrameNum);
        }
 
        close(fd);
        fd = -1;
    }
}
 
/**
 * @tc.number    : DEMUXER_PRECISE_SEEK_0032
 * @tc.name      : 末帧seek, SEEK_MODE_PREVIOUS_SYNC, demuxer_parser_2_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCompareNdkTest, DEMUXER_PRECISE_SEEK_0032, TestSize.Level2)
{
    if (!access("/system/lib64/libav_codec_hevc_parser.z.so", 0)) {
        seekInfo fileTest{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
            10100000, 30, 0, 0};
        CheckSeekMode(fileTest);
    }
}