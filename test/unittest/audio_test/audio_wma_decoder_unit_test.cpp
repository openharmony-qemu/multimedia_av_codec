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

#include <cstring>
#include <vector>
#include <chrono>
#include <gtest/gtest.h>
#include <fstream>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "plugin/plugin_manager_v2.h"
#include "media_codec.h"
#include "meta/format.h"
#include "avbuffer_queue.h"
#include "avbuffer.h"
#include "avallocator.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "meta/mime_type.h"
#include "plugin/codec_plugin.h"

using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

namespace {
constexpr int32_t VALID_SR     = 44100;
constexpr int32_t INVALID_SR_0 = 0;
constexpr int32_t INVALID_SR_N = -48000;

constexpr Plugins::AudioSampleFormat S16 = Plugins::AudioSampleFormat::SAMPLE_S16LE;
constexpr Plugins::AudioSampleFormat F32 = Plugins::AudioSampleFormat::SAMPLE_F32LE;

constexpr uint32_t OUT_Q_NUM = 4;
constexpr size_t   FEED_CHUNK = 1024;
constexpr int64_t  WAIT_MS = 1500;

const std::string kWmav1Asset  = "/data/test/media/tiny_wmav1.wma";
const std::string kWmav2Asset  = "/data/test/media/tiny_wmav2.wma";
const std::string kWmaproAsset = "/data/test/media/tiny_wmapro.wma";

std::shared_ptr<CodecPlugin> CreatePluginByName(const std::string &name)
{
    auto p = PluginManagerV2::Instance().CreatePluginByName(name);
    if (!p) {
        return nullptr;
    }
    return std::reinterpret_pointer_cast<CodecPlugin>(p);
}
} // namespace

class AudioWMAPluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

/* =========================
 *  插件层：参数/状态机用例
 * ========================= */

// 1) WMAv1：缺失必填键（channel_count）
HWTEST_F(AudioWMAPluginUnitTest, WMAv1_SetParameter_MissingChannel, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    // 未设置 AUDIO_CHANNEL_COUNT
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Release());
}

// 2) WMAv2：非法采样率（<=0）
HWTEST_F(AudioWMAPluginUnitTest, WMAv2_SetParameter_InvalidSampleRate, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(INVALID_SR_0);
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Release());

    meta->Set<Tag::AUDIO_SAMPLE_RATE>(INVALID_SR_N);
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Release());
}

// 3) WMAPro：正常参数（S16 / F32 都应可通过 base_->CheckSampleFormat）
HWTEST_F(AudioWMAPluginUnitTest, WMAPro_SetParameter_ValidParams, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Release());

    auto metaF32 = std::make_shared<Meta>();
    metaF32->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    metaF32->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    metaF32->Set<Tag::AUDIO_SAMPLE_FORMAT>(F32);
    
    auto plugin2 = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME));
    ASSERT_NE(nullptr, plugin2);
    ASSERT_EQ(Status::OK, plugin2->Init());
    EXPECT_EQ(Status::OK, plugin2->SetParameter(metaF32));
}

// 4) 状态机：未配置直接 Prepare / Start
HWTEST_F(AudioWMAPluginUnitTest, WMAv2_StateMachine_Rough, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    EXPECT_EQ(Status::OK, plugin->Prepare());
    EXPECT_EQ(Status::OK, plugin->Start());
    EXPECT_EQ(Status::OK, plugin->Stop());
    EXPECT_EQ(Status::OK, plugin->Release());
}

// 5) SetParameter 后校验 GetParameter 返回 max buffer / mime
HWTEST_F(AudioWMAPluginUnitTest, WMAv1_GetParameter_CheckOutputFormat, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    ASSERT_EQ(Status::OK, plugin->SetParameter(meta));

    std::shared_ptr<Meta> got;
    EXPECT_EQ(Status::OK, plugin->GetParameter(got));
    ASSERT_NE(nullptr, got);

    int32_t inMax = 0;
    int32_t outMax = 0;
    EXPECT_TRUE(got->Get<Tag::AUDIO_MAX_INPUT_SIZE>(inMax));
    EXPECT_TRUE(got->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(outMax));

    // 与插件实现保持一致：输入不超过 8192（默认兜底），输出固定为 4 * 2048 * 8
    EXPECT_GT(inMax, 0);
    EXPECT_LE(inMax, 8192);
    EXPECT_EQ(outMax, 4 * 2048 * 8);

    // 校验 MIME_TYPE：WMAv1
    std::string mime;
    EXPECT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, Media::MimeType::AUDIO_WMAV1);
}

HWTEST_F(AudioWMAPluginUnitTest, WMAv2_GetParameter_CheckOutputFormat, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    ASSERT_EQ(Status::OK, plugin->SetParameter(meta));

    std::shared_ptr<Meta> got;
    ASSERT_EQ(Status::OK, plugin->GetParameter(got));
    std::string mime;
    ASSERT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, Media::MimeType::AUDIO_WMAV2);
    EXPECT_EQ(Status::OK, plugin->Release());
}

HWTEST_F(AudioWMAPluginUnitTest, WMAPro_GetParameter_CheckOutputFormat, TestSize.Level1)
{
    auto plugin = CreatePluginByName(std::string(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME));
    ASSERT_NE(nullptr, plugin);
    ASSERT_EQ(Status::OK, plugin->Init());
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    ASSERT_EQ(Status::OK, plugin->SetParameter(meta));

    std::shared_ptr<Meta> got;
    ASSERT_EQ(Status::OK, plugin->GetParameter(got));
    std::string mime;
    ASSERT_TRUE(got->Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, Media::MimeType::AUDIO_WMAPRO);
    EXPECT_EQ(Status::OK, plugin->Release());
}

/* =======================================
 *  真实解码（存在样本才执行）
 *  使用 MediaCodec 作轻量集成验证
 * =======================================*/

class WMAOutputObserver : public Media::AudioBaseCodecCallback {
public:
    void OnError(Media::CodecErrorType, int32_t) override {}
    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &buffer) override
    {
        if (!buffer || !buffer->memory_) {
            return;
        }
        auto sz = buffer->memory_->GetSize();
        if (sz > 0) {
            {
                std::lock_guard<std::mutex> lk(m_);
                totalBytes_ += sz;
            }
            cv_.notify_all();
        }
    }
    void OnOutputFormatChanged(const std::shared_ptr<Meta> &) override {}

    bool WaitForBytes(size_t atLeast, int64_t timeoutMs)
    {
        std::unique_lock<std::mutex> lk(m_);
        return cv_.wait_for(lk, std::chrono::milliseconds(timeoutMs),
            [&] { return totalBytes_ >= atLeast; });
    }

    size_t Total() const
    {
        return totalBytes_;
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    size_t totalBytes_ {0};
};

static void FeedFileToCodec(const std::string &path, std::shared_ptr<MediaCodec> &mc)
{
    std::ifstream ifs(path, std::ios::binary);
    ASSERT_TRUE(ifs.is_open());

    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::vector<char> chunk(FEED_CHUNK);

    while (ifs) {
        ifs.read(chunk.data(), chunk.size());
        const auto n = static_cast<int>(ifs.gcount());
        if (n <= 0) {
            break;
        }
        auto in = AVBuffer::CreateAVBuffer(alloc, n);
        errno_t ret = memcpy_s(in->memory_->GetAddr(),
                               in->memory_->GetCapacity(),
                               chunk.data(),
                               n);
        if (ret != 0) {
            MEDIA_LOG_E("memcpy_s failed, ret=%d, path=%s", ret, path.c_str());
            return;
        }
        ASSERT_EQ(0, ret);
        in->memory_->SetSize(n);
        ASSERT_EQ(static_cast<int>(Status::OK), mc->QueueInputBuffer(in));
    }

    // 送 EOS
    auto eos = AVBuffer::CreateAVBuffer(alloc, 0);
    eos->flag_ = AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(static_cast<int>(Status::OK), mc->QueueInputBuffer(eos));
}


// 通用真解码用例（codecName + 文件路径）
static void RealDecodeOnce(const std::string &codecName, const std::string &assetPath)
{
    std::ifstream test(assetPath, std::ios::binary);
    if (!test.is_open()) {
        GTEST_SKIP() << "No test asset: " << assetPath;
    }

    auto mc = std::make_shared<MediaCodec>();
    ASSERT_EQ(0, mc->Init(codecName));

    // 设置输出缓冲队列
    auto outQ = Media::AVBufferQueue::Create(OUT_Q_NUM, Media::MemoryType::SHARED_MEMORY, "UT-WMA");
    ASSERT_EQ(0, mc->SetOutputBufferQueue(outQ->GetProducer()));

    // 基本参数
    constexpr int32_t DEFAULT_CHANNEL_COUNT = 2;
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(DEFAULT_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(VALID_SR);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(S16);
    ASSERT_EQ(0, mc->Configure(meta));

    // 回调统计输出字节
    auto cb = std::make_shared<WMAOutputObserver>();
    ASSERT_EQ(0, mc->SetCodecCallback(std::static_pointer_cast<Media::AudioBaseCodecCallback>(cb)));

    ASSERT_EQ((int)Status::OK, mc->Prepare());
    ASSERT_EQ((int)Status::OK, mc->Start());

    // 喂数据
    FeedFileToCodec(assetPath, mc);

    // 等待有 PCM 产生
    EXPECT_TRUE(cb->WaitForBytes(1, WAIT_MS)) << "No PCM produced within timeout";

    EXPECT_EQ((int)Status::OK, mc->Stop());
    EXPECT_EQ((int)Status::OK, mc->Release());
}

// WMAv1 真实解码
HWTEST_F(AudioWMAPluginUnitTest, WMAv1_RealDecode_Sanity, TestSize.Level1)
{
    RealDecodeOnce(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME), kWmav1Asset);
}

// WMAv2 真实解码
HWTEST_F(AudioWMAPluginUnitTest, WMAv2_RealDecode_Sanity, TestSize.Level1)
{
    RealDecodeOnce(std::string(AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME), kWmav2Asset);
}

// WMAPro 真实解码
HWTEST_F(AudioWMAPluginUnitTest, WMAPro_RealDecode_Sanity, TestSize.Level1)
{
    RealDecodeOnce(std::string(AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME), kWmaproAsset);
}