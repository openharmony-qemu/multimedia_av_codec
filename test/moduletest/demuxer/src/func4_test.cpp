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

#include "gtest/gtest.h"

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <cmath>
#include <thread>
namespace OHOS {
namespace Media {
class DemuxerFunc4NdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};

static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
static OH_AVFormat *metaFormat = nullptr;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
void DemuxerFunc4NdkTest::SetUpTestCase() {}
void DemuxerFunc4NdkTest::TearDownTestCase() {}
void DemuxerFunc4NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerFunc4NdkTest::TearDown()
{
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }

    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
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
    if (metaFormat != nullptr) {
        OH_AVFormat_Destroy(metaFormat);
        metaFormat = nullptr;
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

void GetMetaInfo(string metaKeyAdd, string metaKey)
{
    int32_t extraSize = 30;
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, metaKey.c_str(), &codecConfig, &bufferSize));
        ASSERT_EQ(bufferSize, extraSize);
        for (int j = 0; j < bufferSize; j++) {
            ASSERT_EQ(3, codecConfig[j]);
        }
        extraSize = extraSize + 2;
    }
}

/**
 * @tc.number    : DEMUXER_META_0110
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0110, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_03.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0120
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0120, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_03.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_FALSE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.buffervalval", &codecConfig, &bufferSize));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0130
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0130, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_03.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.bufferval", &metaStringValue));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.bufferval", &metaIntValue));
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.bufferval", &metaFloatValue));
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : DEMUXER_META_0140
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0140, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_01.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval.max", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 1048576);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0150
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0150, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_04.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    string metaVal = "aaaaa";
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.stringval", &metaStringValue));
    ASSERT_EQ(metaStringValue, metaVal);
    float floatValue = 252.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval", &metaFloatValue));
    ASSERT_EQ(floatValue, metaFloatValue);
    int32_t intValue = 10000;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.intval.intvalintval", &metaIntValue));
    ASSERT_EQ(metaIntValue, intValue);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0160
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0160, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/metaval_05.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    string metaKeyAdd = "a";
    string metaKey = "com.openharmony.stringval.";
    string metaVal = "aaaaa";
    string metaValAdd = "b";
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaVal.append(metaValAdd);
        ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, metaKey.c_str(), &metaStringValue));
        ASSERT_EQ(metaStringValue, metaVal);
    }

    metaKeyAdd = "a";
    metaKey = "com.openharmony.floatval.";
    float metaFloatVal = 123.5;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaFloatVal = metaFloatVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, metaKey.c_str(), &metaFloatValue));
        ASSERT_EQ(metaFloatValue, metaFloatVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.Intval.";
    int32_t metaIntVal = 123;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaIntVal = metaIntVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, metaKey.c_str(), &metaIntValue));
        ASSERT_EQ(metaIntValue, metaIntVal);
    }
    
    metaKeyAdd = "a";
    metaKey = "com.openharmony.bufferval.";
    GetMetaInfo(metaKeyAdd, metaKey);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0170
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0170, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_03.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0180
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0180, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_03.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_FALSE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.buffervalval", &codecConfig, &bufferSize));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0190
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0190, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_03.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.bufferval", &metaStringValue));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.bufferval", &metaIntValue));
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.bufferval", &metaFloatValue));
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : DEMUXER_META_0200
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0200, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_01.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval.max", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 1048576);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0210
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0210, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_04.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    string metaVal = "aaaaa";
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.stringval", &metaStringValue));
    ASSERT_EQ(metaStringValue, metaVal);
    float floatValue = 252.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval", &metaFloatValue));
    ASSERT_EQ(floatValue, metaFloatValue);
    int32_t intValue = 10000;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.intval.intvalintval", &metaIntValue));
    ASSERT_EQ(metaIntValue, intValue);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0220
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0220, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/audio/metaval_05.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    string metaKeyAdd = "a";
    string metaKey = "com.openharmony.stringval.";
    string metaVal = "aaaaa";
    string metaValAdd = "b";
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaVal.append(metaValAdd);
        ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, metaKey.c_str(), &metaStringValue));
        ASSERT_EQ(metaStringValue, metaVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.floatval.";
    float metaFloatVal = 123.5;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaFloatVal = metaFloatVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, metaKey.c_str(), &metaFloatValue));
        ASSERT_EQ(metaFloatValue, metaFloatVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.Intval.";
    int32_t metaIntVal = 123;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaIntVal = metaIntVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, metaKey.c_str(), &metaIntValue));
        ASSERT_EQ(metaIntValue, metaIntVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.bufferval.";
    GetMetaInfo(metaKeyAdd, metaKey);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0230
 * @tc.name      : demuxer meta info, file with no meta
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0230, TestSize.Level2)
{
    const char *file = "/data/test/media/audio/M4A_48000_1.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* language = OH_AVFormat_DumpInfo(metaFormat);
    ASSERT_EQ(language, nullptr);
    close(fd);
    fd = -1;
}