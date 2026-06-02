/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "avcodec_errors.h"
#include "fsurface_memory.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr int32_t TEST_WIDTH = 320;
constexpr int32_t TEST_HEIGHT = 240;
constexpr uint64_t TEST_USAGE = SURFACE_DEFAULT_USAGE;
}

namespace OHOS {
namespace MediaAVCodec {
class FSurfaceMemoryUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    SurfaceControl* sInfo_ = nullptr;
    CallerInfo callerInfo_;
};

void FSurfaceMemoryUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void FSurfaceMemoryUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void FSurfaceMemoryUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
    sInfo_ = new SurfaceControl();
    sInfo_->requestConfig = {
        .width = TEST_WIDTH,
        .height = TEST_HEIGHT,
        .strideAlignment = 8,
        .format = 0,
        .usage = TEST_USAGE,
        .timeout = 0
    };
    callerInfo_.pid = 12345; // 12345
    callerInfo_.instanceId = "test_instance_001";
    callerInfo_.mimeType = "video/avc";
}

void FSurfaceMemoryUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
    delete sInfo_;
    sInfo_ = nullptr;
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_create_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    EXPECT_NE(nullptr, fsurfaceMemory);
    EXPECT_EQ(Owner::OWNED_BY_US, fsurfaceMemory->owner);
    EXPECT_FALSE(fsurfaceMemory->isAttached);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_owner_state_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->owner = Owner::OWNED_BY_CODEC;
    EXPECT_EQ(Owner::OWNED_BY_CODEC, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_owner_state_02, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->owner = Owner::OWNED_BY_USER;
    EXPECT_EQ(Owner::OWNED_BY_USER, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_owner_state_03, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->owner = Owner::OWNED_BY_SURFACE;
    EXPECT_EQ(Owner::OWNED_BY_SURFACE, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_owner_transition_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    EXPECT_EQ(Owner::OWNED_BY_US, fsurfaceMemory->owner);
    fsurfaceMemory->owner = Owner::OWNED_BY_CODEC;
    EXPECT_EQ(Owner::OWNED_BY_CODEC, fsurfaceMemory->owner);
    fsurfaceMemory->owner = Owner::OWNED_BY_USER;
    EXPECT_EQ(Owner::OWNED_BY_USER, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_owner_transition_02, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->owner = Owner::OWNED_BY_USER;
    fsurfaceMemory->owner = Owner::OWNED_BY_SURFACE;
    EXPECT_EQ(Owner::OWNED_BY_SURFACE, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_owner_transition_03, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->owner = Owner::OWNED_BY_CODEC;
    fsurfaceMemory->owner = Owner::OWNED_BY_US;
    EXPECT_EQ(Owner::OWNED_BY_US, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_isattached_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    EXPECT_FALSE(fsurfaceMemory->isAttached);
    fsurfaceMemory->isAttached = true;
    EXPECT_TRUE(fsurfaceMemory->isAttached);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_releasesurfacebuffer_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->ReleaseSurfaceBuffer();
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_releasesurfacebuffer_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    fsurfaceMemory->ReleaseSurfaceBuffer();
    sptr<SurfaceBuffer> buffer = fsurfaceMemory->GetSurfaceBuffer();
    EXPECT_EQ(nullptr, buffer);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_releasesurfacebuffer_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    sptr<SurfaceBuffer> buffer = fsurfaceMemory->GetSurfaceBuffer();
    EXPECT_NE(nullptr, buffer);
    fsurfaceMemory->ReleaseSurfaceBuffer();
    buffer = fsurfaceMemory->GetSurfaceBuffer();
    EXPECT_EQ(nullptr, buffer);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebuffer_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> buffer = fsurfaceMemory->GetSurfaceBuffer();
    EXPECT_EQ(nullptr, buffer);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebuffer_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->isAttached = true;
    fsurfaceMemory->owner = Owner::OWNED_BY_SURFACE;
    sptr<SurfaceBuffer> buffer = fsurfaceMemory->GetSurfaceBuffer();
    EXPECT_EQ(nullptr, buffer);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebuffer_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->isAttached = true;
    fsurfaceMemory->owner = Owner::OWNED_BY_SURFACE;
    sInfo_->surface = nullptr;
    sptr<SurfaceBuffer> buffer = fsurfaceMemory->GetSurfaceBuffer();
    EXPECT_EQ(nullptr, buffer);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebuffer_04, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    sptr<SurfaceBuffer> buffer = fsurfaceMemory->GetSurfaceBuffer();
    EXPECT_NE(nullptr, buffer);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebufferstride_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t stride = fsurfaceMemory->GetSurfaceBufferStride();
    EXPECT_EQ(0, stride);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebufferstride_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    int32_t stride = fsurfaceMemory->GetSurfaceBufferStride();
    EXPECT_GE(stride, 0);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebufferstride_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC);
    int32_t stride = fsurfaceMemory->GetSurfaceBufferStride();
    EXPECT_GE(stride, 0);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsurfacebufferstride_04, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_USER);
    int32_t stride = fsurfaceMemory->GetSurfaceBufferStride();
    EXPECT_GE(stride, 0);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getbase_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    uint8_t* base = fsurfaceMemory->GetBase();
    EXPECT_EQ(nullptr, base);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getbase_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
    uint8_t* base = fsurfaceMemory->GetBase();
    EXPECT_EQ(nullptr, base);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getbase_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC);
    uint8_t* base = fsurfaceMemory->GetBase();
    EXPECT_EQ(nullptr, base);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsize_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t size = fsurfaceMemory->GetSize();
    EXPECT_EQ(-1, size);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsize_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
    int32_t size = fsurfaceMemory->GetSize();
    EXPECT_GE(size, 0);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getsize_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC);
    int32_t size = fsurfaceMemory->GetSize();
    EXPECT_GE(size, 0);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getfence_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SyncFence> fence = fsurfaceMemory->GetFence();
    EXPECT_EQ(nullptr, fence);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getfence_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> fence = SyncFence::InvalidFence();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC, fence);
    sptr<SyncFence> resultFence = fsurfaceMemory->GetFence();
    EXPECT_NE(nullptr, resultFence);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getfence_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    sptr<SyncFence> fence = fsurfaceMemory->GetFence();
    EXPECT_EQ(nullptr, fence);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getfence_04, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> fence = nullptr;
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC, fence);
    sptr<SyncFence> resultFence = fsurfaceMemory->GetFence();
    EXPECT_EQ(nullptr, resultFence);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getfence_05, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    fsurfaceMemory->SetSurfaceBuffer(nullptr, Owner::OWNED_BY_CODEC);
    sptr<SyncFence> fence = fsurfaceMemory->GetFence();
    EXPECT_EQ(nullptr, fence);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getseqnum_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    uint32_t seqNum = fsurfaceMemory->GetSurfaceBufferSeqNum();
    EXPECT_EQ(0u, seqNum);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getseqnum_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    uint32_t seqNum = fsurfaceMemory->GetSurfaceBufferSeqNum();
    EXPECT_EQ(0u, seqNum);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_getseqnum_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC);
    uint32_t seqNum = fsurfaceMemory->GetSurfaceBufferSeqNum();
    EXPECT_NE(0u, seqNum);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_callerinfo_01, TestSize.Level1)
{
    CallerInfo info;
    info.pid = 99999; // 99999
    info.instanceId = "test_id_123";
    info.mimeType = "video/hevc";
    info.calledByAvcodec = true;

    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, info);
    EXPECT_NE(nullptr, fsurfaceMemory);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setsurfacebuffer_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC, nullptr);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setsurfacebuffer_02, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> fence = SyncFence::InvalidFence();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC, fence);
    EXPECT_EQ(Owner::OWNED_BY_CODEC, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setsurfacebuffer_03, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    sptr<SyncFence> fence = SyncFence::InvalidFence();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC, fence);
    EXPECT_NE(Owner::OWNED_BY_CODEC, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setsurfacebuffer_04, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> fence = SyncFence::InvalidFence();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC, fence);
    EXPECT_EQ(Owner::OWNED_BY_CODEC, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setsurfacebuffer_05, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_USER);
    EXPECT_EQ(Owner::OWNED_BY_USER, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setsurfacebuffer_06, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_SURFACE);
    EXPECT_EQ(Owner::OWNED_BY_SURFACE, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setsurfacebuffer_07, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> surfaceBuffer2 = SurfaceBuffer::Create();
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC);
    fsurfaceMemory->SetSurfaceBuffer(surfaceBuffer2, Owner::OWNED_BY_USER);
    EXPECT_EQ(Owner::OWNED_BY_USER, fsurfaceMemory->owner);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_allocsurfacebuffer_01, TestSize.Level1)
{
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_allocsurfacebuffer_02, TestSize.Level1)
{
    sInfo_->surface = nullptr;
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_allocsurfacebuffer_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    fsurfaceMemory->isAttached = true;
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_allocsurfacebuffer_04, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_allocsurfacebuffer_05, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_allocsurfacebuffer_06, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(0, 0);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_destructor_01, TestSize.Level1)
{
    FSurfaceMemory* fsurfaceMemory = new FSurfaceMemory(sInfo_, callerInfo_);
    delete fsurfaceMemory;
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_destructor_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    FSurfaceMemory* fsurfaceMemory = new FSurfaceMemory(sInfo_, callerInfo_);
    fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    delete fsurfaceMemory;
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setcallertobuffer_01, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, callerInfo_);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
    CallerInfo info;
    info.pid = 0;
    info.instanceId = "";
    info.mimeType = "";
    fsurfaceMemory.reset();
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setcallertobuffer_02, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    CallerInfo info;
    info.pid = -1;
    info.instanceId = "test";
    info.mimeType = "video/invalid";
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, info);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setcallertobuffer_03, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    CallerInfo info;
    info.pid = 12345; // 12345
    info.instanceId = "special!@#$%chars";
    info.mimeType = "video/avc";
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, info);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setcallertobuffer_04, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    CallerInfo info;
    info.pid = 12345; // 12345
    info.instanceId = "test_instance_001";
    info.mimeType = "video/avc/h264";
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, info);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(FSurfaceMemoryUnitTest, fsurface_memory_setcallertobuffer_05, TestSize.Level1)
{
    sInfo_->surface = Surface::CreateSurfaceAsConsumer();
    CallerInfo info;
    info.pid = 12345; // 12345
    info.instanceId = "test_instance_001";
    info.mimeType = "video///hevc";
    unique_ptr<FSurfaceMemory> fsurfaceMemory = make_unique<FSurfaceMemory>(sInfo_, info);
    int32_t ret = fsurfaceMemory->AllocSurfaceBuffer(TEST_WIDTH, TEST_HEIGHT);
    EXPECT_NE(AVCS_ERR_OK, ret);
}
} // namespace MediaAVCodec
} // namespace OHOS