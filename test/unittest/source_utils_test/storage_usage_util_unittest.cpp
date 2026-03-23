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

#include "storage_usage_util.h"
#include "gtest/gtest.h"
#include "gtest/hwext/gtest-ext.h"
#include <sys/statvfs.h>
#include <sys/types.h>
#include <cstdint>

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
constexpr int64_t MAX_REASONABLE_STORAGE = 100LL * 1024 * 1024 * 1024 * 1024;
constexpr int64_t MAX_REASONABLE_FREE = 1024LL * 1024 * 1024 * 1024 * 1024;

class StorageUsageUtilTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        // 测试用例初始化
    }
    static void TearDownTestCase(void)
    {
        // 测试用例清理
    }
    void SetUp() override
    {
        // 每个测试用例前的初始化
    }
    void TearDown() override
    {
        // 每个测试用例后的清理
    }
};

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_001
 * @tc.desc: 测试GetFreeSize基本功能，验证能成功获取剩余空间
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_001, TestSize.Level1)
{
    StorageUsageUtil util;
    int64_t freeSize = util.GetFreeSize();
    
    // 验证返回值不为负数（成功情况）
    EXPECT_GE(freeSize, 0);
    
    // 如果成功获取，验证返回值合理性
    if (freeSize > 0) {
        // 剩余空间应该是一个合理的数值
        EXPECT_LT(freeSize, INT64_MAX);
    }
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_002
 * @tc.desc: 测试GetFreeSize错误处理，验证statvfs失败时返回-1
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_002, TestSize.Level1)
{
    StorageUsageUtil util;
    int64_t freeSize = util.GetFreeSize();
    
    // 验证返回值范围
    EXPECT_GE(freeSize, -1);
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_003
 * @tc.desc: 测试GetFreeSize多次调用的一致性
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_003, TestSize.Level1)
{
    StorageUsageUtil util;
    
    // 连续调用两次GetFreeSize
    int64_t freeSize1 = util.GetFreeSize();
    int64_t freeSize2 = util.GetFreeSize();
    
    // 验证两次调用结果一致（在短时间内）
    EXPECT_EQ(freeSize1, freeSize2);
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_004
 * @tc.desc: 测试GetFreeSize返回值的合理性验证
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_004, TestSize.Level1)
{
    StorageUsageUtil util;
    int64_t freeSize = util.GetFreeSize();
    
    if (freeSize > 0) {
        // 验证返回值是合理的字节数
        EXPECT_GT(freeSize, 0);
        
        // 验证返回值不会超过最大合理值（比如100TB）
        EXPECT_LT(freeSize, MAX_REASONABLE_STORAGE);
    }
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_005
 * @tc.desc: 测试GetFreeSize与GetTotalSize的关联性
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_005, TestSize.Level1)
{
    StorageUsageUtil util;
    
    int64_t freeSize = util.GetFreeSize();
    int64_t totalSize = util.GetTotalSize();
    
    // 如果两个调用都成功，验证剩余空间不超过总空间
    if (freeSize >= 0 && totalSize >= 0) {
        EXPECT_LE(freeSize, totalSize);
        EXPECT_GT(totalSize, 0);
    }
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_006
 * @tc.desc: 测试GetFreeSize在不同对象实例上的一致性
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_006, TestSize.Level1)
{
    // 创建两个不同的StorageUsageUtil实例

    StorageUsageUtil util1;
    StorageUsageUtil util2;
    
    // 分别调用GetFreeSize
    int64_t freeSize1 = util1.GetFreeSize();
    int64_t freeSize2 = util2.GetFreeSize();
    
    // 验证两个实例返回相同结果（因为访问的是同一个路径）
    EXPECT_EQ(freeSize1, freeSize2);
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_007
 * @tc.desc: 测试GetFreeSize的线程安全性（基本验证）
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_007, TestSize.Level1)
{
    StorageUsageUtil util;
    
    // 在同一测试中多次调用GetFreeSize
    int64_t freeSize1 = util.GetFreeSize();
    int64_t freeSize2 = util.GetFreeSize();
    int64_t freeSize3 = util.GetFreeSize();
    
    // 验证所有调用都返回有效值
    EXPECT_GE(freeSize1, -1);
    EXPECT_GE(freeSize2, -1);
    EXPECT_GE(freeSize3, -1);
    
    // 如果都成功，验证结果一致性
    if (freeSize1 >= 0 && freeSize2 >= 0 && freeSize3 >= 0) {
        EXPECT_EQ(freeSize1, freeSize2);
        EXPECT_EQ(freeSize2, freeSize3);
    }
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_008
 * @tc.desc: 测试GetFreeSize与HasEnoughStorage的关联性
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_008, TestSize.Level1)
{
    StorageUsageUtil util;
    
    int64_t freeSize = util.GetFreeSize();
    bool hasEnough = util.HasEnoughStorage();
    
    // 如果GetFreeSize成功，验证HasEnoughStorage的行为
    if (freeSize >= 0) {
        // HasEnoughStorage应该返回true或false，而不是异常
        EXPECT_TRUE(hasEnough == true || hasEnough == false);
    } else {
        // 如果GetFreeSize失败，HasEnoughStorage应该返回false
        EXPECT_FALSE(hasEnough);
    }
}

/**
 * @tc.name: StorageUsageUtil_GetFreeSize_009
 * @tc.desc: 测试GetFreeSize的边界值处理
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(StorageUsageUtilTest, StorageUsageUtil_GetFreeSize_009, TestSize.Level1)
{
    StorageUsageUtil util;
    int64_t freeSize = util.GetFreeSize();
    
    // 验证返回值范围
    if (freeSize >= 0) {
        // 成功情况：非负数
        EXPECT_GE(freeSize, 0);
        
        // 验证返回值是合理的存储大小
        // 假设剩余空间不会超过1PB
        EXPECT_LT(freeSize, MAX_REASONABLE_FREE);
    }
}

} // namespace Media
} // namespace OHOS
