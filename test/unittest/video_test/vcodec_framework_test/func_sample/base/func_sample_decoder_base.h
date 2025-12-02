/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FUNC_SAMPLE_DECODER_BASE_H
#define FUNC_SAMPLE_DECODER_BASE_H
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <thread>
#include "avcc_reader.h"
#include "securec.h"
#include "openssl/sha.h"
#include "vcodec_mock.h"

namespace OHOS {
namespace MediaAVCodec {
enum VCodecDataProducerType : int32_t {
    H263_STREAM = 1 << 0,
    AVC_STREAM = 1 << 1,
    HEVC_STREAM = 1 << 2,
    MPEG2_STREAM = 1 << 3,
    MPEG4_STREAM = 1 << 4,
    VC1_STREAM = 1 << 5,
    WMV3_STREAM = 1 << 6,
    MSVIDEO1_STREAM = 1 << 7,
    AV1_STREAM = 1 << 8
};

inline std::unordered_map<std::string, int32_t> fileTypeMap = {
    {"h263", H263_STREAM},
    {"h264", AVC_STREAM},
    {"h265", HEVC_STREAM},
    {"m2v", MPEG2_STREAM},
    {"m4v", MPEG4_STREAM},
    {"vc1", VC1_STREAM},
    {"msvideo1", MSVIDEO1_STREAM},
    {"wmv3", WMV3_STREAM}
};

inline constexpr uint32_t BUFFER_COUNT = 59;
inline constexpr uint8_t SHA_AVC[SHA512_DIGEST_LENGTH] = {
    0x3d, 0xc4, 0x3f, 0x67, 0x74, 0x18, 0xc6, 0xfb, 0xf3, 0x03, 0x56, 0x52, 0xf8, 0xa9, 0xf2, 0x7f,
    0x54, 0xdb, 0xfc, 0x69, 0x82, 0xeb, 0x30, 0x34, 0x62, 0x2f, 0x87, 0x92, 0xcc, 0x31, 0xa2, 0xd3,
    0x79, 0xa8, 0xc8, 0xc1, 0xae, 0x2e, 0x93, 0x58, 0x5f, 0x65, 0xf7, 0xab, 0x64, 0x32, 0xb3, 0x40,
    0xf3, 0x3b, 0x01, 0x1a, 0x75, 0xfa, 0x0e, 0x57, 0xde, 0x48, 0x40, 0xc7, 0x92, 0x7d, 0x14, 0xe8};
inline constexpr uint8_t SHA_HEVC[SHA512_DIGEST_LENGTH] = {
    0x09, 0x48, 0x29, 0x0f, 0xe3, 0x2a, 0xe6, 0x27, 0x33, 0xb1, 0x02, 0x84, 0x57, 0xbd, 0x8a, 0x4d,
    0xd1, 0xab, 0x3b, 0xa4, 0x1a, 0x33, 0xdd, 0x53, 0x3a, 0x0f, 0x16, 0x82, 0xea, 0xa6, 0x32, 0x6b,
    0xef, 0x2f, 0x67, 0xaa, 0x70, 0xd6, 0xae, 0xd9, 0xbe, 0x87, 0x1b, 0x4e, 0xb6, 0x4b, 0x66, 0x6e,
    0xaa, 0xbb, 0x15, 0x24, 0xc1, 0xb0, 0x17, 0xd2, 0x47, 0xf0, 0x19, 0x27, 0xbd, 0xfb, 0xfa, 0x9f};
inline constexpr uint8_t SHA_H263[SHA512_DIGEST_LENGTH] = {
    0x27, 0x7c, 0x59, 0x9a, 0x73, 0x6d, 0x4f, 0xff, 0x02, 0x76, 0xac, 0xe2, 0xb5, 0x2a, 0x50, 0x1e,
    0x99, 0xd4, 0xdf, 0xd6, 0x8f, 0xa8, 0xa1, 0x22, 0x1c, 0x74, 0xf9, 0xd2, 0x8f, 0x97, 0x19, 0x8d,
    0x66, 0x9c, 0x58, 0x41, 0x6f, 0xad, 0xdd, 0xa9, 0x68, 0x1e, 0x76, 0xf2, 0x6e, 0x8e, 0x5b, 0xe5,
    0xe2, 0xc6, 0x5f, 0x5f, 0xab, 0xca, 0xc2, 0x5e, 0x8e, 0x77, 0xe6, 0xad, 0x59, 0x63, 0x40, 0x6b};

inline std::map <int32_t, std::vector<int32_t>> g_hdrDynamicMetaSize = {
    {VCodecTestParam::HW_HDR, {59, 59, 59, 59, 59}},
    {VCodecTestParam::HW_HDR_HLG_FULL, {53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
        53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
        53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
        53, 53, 53, 53, 53, 53, 53, 53, 53}}
};

inline std::map <int32_t, std::vector<int32_t>> g_hdrStaticMetaSize = {
    {VCodecTestParam::HW_HDR, {48, 48, 48, 48, 48}},
    {VCodecTestParam::HW_HDR_HLG_FULL, {48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
        48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
        48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
        48, 48, 48, 48, 48, 48, 48, 48, 48}},
    {VCodecTestParam::HW_HDR10, {48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48}}
};

inline const std::map<int32_t, std::string> g_hdrDynamicMeta = {
    {VCodecTestParam::HW_HDR, "/data/test/media/720_1280_25_avcc_hdr_dynamic_meta.bin"},
    {VCodecTestParam::HW_HDR_HLG_FULL, "/data/test/media/1920_1440_30_avcc_hdr_dynamic_meta.bin"}
};

inline const std::map<int32_t, std::string> g_hdrStaticMeta = {
    {VCodecTestParam::HW_HDR, "/data/test/media/720_1280_25_avcc_hdr_static_meta.bin"},
    {VCodecTestParam::HW_HDR_HLG_FULL, "/data/test/media/1920_1440_30_avcc_hdr_static_meta.bin"},
    {VCodecTestParam::HW_HDR10, "/data/test/media/1600_900_avcc_hdr10_hdr_static_meta.bin"}
};

inline const std::map<int32_t, OH_NativeBuffer_MetadataType> g_hdrType = {
    {VCodecTestParam::HW_HDR, OH_VIDEO_HDR_VIVID},
    {VCodecTestParam::HW_HDR_HLG_FULL, OH_VIDEO_HDR_VIVID},
    {VCodecTestParam::HW_HDR10, OH_VIDEO_HDR_HDR10}
};

inline uint8_t g_mdTest[SHA512_DIGEST_LENGTH];
inline std::atomic<uint32_t> g_shaBufferCount = 0;
inline SHA512_CTX g_ctxTest;

struct VDecSignal {
public:
    std::mutex mutex_;
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::shared_mutex syncMutex_;
    std::condition_variable cond_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIndexQueue_;
    std::queue<uint32_t> outIndexQueue_;
    std::queue<OH_AVCodecBufferAttr> outAttrQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> inMemoryQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> outMemoryQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> inBufferQueue_;
    std::queue<std::shared_ptr<AVBufferMock>> outBufferQueue_;
    int32_t errorNum = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isPreparing_ = true;

    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t wStride_ = 0;
    int32_t hStride_ = 0;
};

struct DetailedErrorCode {
    bool verification_{true};
    bool unsupportedSpecification_{false};
    bool illegalParam_ {false};
    bool missingParam_{false};

    DetailedErrorCode(bool verification = true,
                      bool unsupportedSpecification = false,
                      bool illegalParam = false,
                      bool missingParam = false)
        : verification_(verification),
          unsupportedSpecification_(unsupportedSpecification),
          illegalParam_(illegalParam),
          missingParam_(missingParam) {};

    ~DetailedErrorCode() {};
};

class VDecCallbackTest : public AVCodecCallbackMock {
public:
    explicit VDecCallbackTest(std::shared_ptr<VDecSignal> signal);
    ~VDecCallbackTest() override;
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr) override;

private:
    void CheckDetailedErrorCode(int32_t errorCode);
    std::shared_ptr<VDecSignal> signal_ = nullptr;
    DetailedErrorCode detailedErrorCode_;
};

class VDecCallbackTestExt : public MediaCodecCallbackMock {
public:
    explicit VDecCallbackTestExt(std::shared_ptr<VDecSignal> signal);
    ~VDecCallbackTestExt() override;
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data) override;

private:
    void CheckDetailedErrorCode(int32_t errorCode);
    std::shared_ptr<VDecSignal> signal_ = nullptr;
    DetailedErrorCode detailedErrorCode_;
};

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(Surface *cs, std::string_view name, bool needCheckSHA = false);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t timestamp_ = 0;
    Rect damage_ = {};
    Surface *cs_ = nullptr;
    bool needCheckSHA_ = false;
    std::unique_ptr<std::ofstream> outFile_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FUNC_SAMPLE_DECODER_BASE_H