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
#include "surface_tools.h"
#include "surface.h"
#include "sync_fence.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace MediaAVCodec {
class SurfaceToolsUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    int32_t instanceId_;
};

void SurfaceToolsUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void SurfaceToolsUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void SurfaceToolsUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
    instanceId_ = 12345; // 12345
}

void SurfaceToolsUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_get_instance_01, TestSize.Level1)
{
    SurfaceTools& instance1 = SurfaceTools::GetInstance();
    SurfaceTools& instance2 = SurfaceTools::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_clean_cache_null_surface_01, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    tools.CleanCache(instanceId_, nullptr, false);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_clean_cache_null_surface_02, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    tools.CleanCache(instanceId_, nullptr, true);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_null_surface_01, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    tools.ReleaseSurface(instanceId_, nullptr, false, false);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_null_surface_02, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    tools.ReleaseSurface(instanceId_, nullptr, true, true);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_null_surface_03, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    tools.ReleaseSurface(instanceId_, nullptr, false, true);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_null_surface_04, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    tools.ReleaseSurface(instanceId_, nullptr, true, false);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_clean_cache_wrong_instance_id_01, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.CleanCache(99999, surface, false); // 99999
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_wrong_instance_id_01, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.ReleaseSurface(99999, surface, false, false); // 99999
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_wrong_instance_id_02, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.ReleaseSurface(99999, surface, true, true); // 99999
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_clean_cache_valid_surface_not_in_map_01, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.CleanCache(instanceId_, surface, false);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_clean_cache_valid_surface_not_in_map_02, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.CleanCache(instanceId_, surface, true);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_valid_surface_not_in_map_01, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.ReleaseSurface(instanceId_, surface, false, false);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_valid_surface_not_in_map_02, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.ReleaseSurface(instanceId_, surface, true, false);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_valid_surface_not_in_map_03, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.ReleaseSurface(instanceId_, surface, false, true);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_release_surface_valid_surface_not_in_map_04, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    tools.ReleaseSurface(instanceId_, surface, true, true);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_register_release_listener_null_surface_01, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    auto callback = [](sptr<SurfaceBuffer>& buffer) -> GSError { (void)buffer; return GSERROR_OK; };
    bool result = tools.RegisterReleaseListener(instanceId_, nullptr, callback);
    EXPECT_EQ(false, result);
}

HWTEST_F(SurfaceToolsUnitTest, surface_tools_register_release_listener_null_surface_02, TestSize.Level1)
{
    SurfaceTools& tools = SurfaceTools::GetInstance();
    auto callback = [](uint32_t seq, const sptr<SyncFence>& fence) -> GSError {
        (void)seq; (void)fence; return GSERROR_OK; };
    bool result = tools.RegisterReleaseListener(instanceId_, nullptr, callback);
    EXPECT_EQ(false, result);
}
} // namespace MediaAVCodec
} // namespace OHOS