/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "thread_safe_container_unit_test.h"

namespace OHOS::Media::Plugins::HttpPlugin {
using namespace testing::ext;

void ThreadSafeContainerUnitTest::SetUpTestCase(void)
{}

void ThreadSafeContainerUnitTest::TearDownTestCase(void)
{}

void ThreadSafeContainerUnitTest::SetUp()
{}

void ThreadSafeContainerUnitTest::TearDown()
{}

HWTEST_F(ThreadSafeContainerUnitTest, Map_BasicOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::map<int, std::string>> mapContainer;
    
    mapContainer.Set(1, "one");
    mapContainer.Set(2, "two");
    mapContainer.Set(3, "three");
    
    EXPECT_EQ(mapContainer.Size(), 3);
    EXPECT_FALSE(mapContainer.Empty());
    
    auto value1 = mapContainer.Get(1);
    EXPECT_TRUE(value1.has_value());
    EXPECT_EQ(value1.value(), "one");
    
    auto value4 = mapContainer.Get(4);
    EXPECT_FALSE(value4.has_value());
    
    EXPECT_TRUE(mapContainer.Contains(2));
    EXPECT_FALSE(mapContainer.Contains(5));
    
    mapContainer.Remove(1);
    EXPECT_EQ(mapContainer.Size(), 2);
    EXPECT_FALSE(mapContainer.Contains(1));
}

HWTEST_F(ThreadSafeContainerUnitTest, Map_ModifyAndModifyWithResult, TestSize.Level1)
{
    ThreadSafeContainer<std::map<int, int>> mapContainer;
    
    mapContainer.Modify([](std::map<int, int>& m) {
        m[1] = 10;
        m[2] = 20;
    });
    
    EXPECT_EQ(mapContainer.Size(), 2);
    
    auto sum = mapContainer.ModifyWithResult([](std::map<int, int>& m) -> int {
        int total = 0;
        for (const auto& [key, value] : m) {
            total += value;
        }
        return total;
    });
    
    EXPECT_EQ(sum, 30);
}

HWTEST_F(ThreadSafeContainerUnitTest, Map_ForeachKV, TestSize.Level1)
{
    ThreadSafeContainer<std::map<int, std::string>> mapContainer;
    
    mapContainer.Set(1, "one");
    mapContainer.Set(2, "two");
    mapContainer.Set(3, "three");
    
    int count = 0;
    std::string result;
    mapContainer.ForeachKV([&count, &result](int key, const std::string& value) {
        count++;
        result += value;
    });
    
    EXPECT_EQ(count, 3);
    EXPECT_EQ(result, "onetwothree");
}

HWTEST_F(ThreadSafeContainerUnitTest, Map_FindIf, TestSize.Level1)
{
    ThreadSafeContainer<std::map<int, std::string>> mapContainer;
    
    mapContainer.Set(1, "one");
    mapContainer.Set(2, "two");
    mapContainer.Set(3, "three");
    
    auto found = mapContainer.FindIf([](const auto& pair) {
        return pair.second == "two";
    });
    
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), "two");
    
    auto notFound = mapContainer.FindIf([](const auto& pair) {
        return pair.second == "four";
    });
    
    EXPECT_FALSE(notFound.has_value());
}

HWTEST_F(ThreadSafeContainerUnitTest, Map_Insert, TestSize.Level1)
{
    ThreadSafeContainer<std::map<int, std::string>> mapContainer;
    
    mapContainer.Insert(1, "one");
    mapContainer.Insert(2, "two");
    
    EXPECT_EQ(mapContainer.Size(), 2);
    EXPECT_TRUE(mapContainer.Contains(1));
    
    mapContainer.Insert(1, "one_updated");
    auto value = mapContainer.Get(1);
    EXPECT_EQ(value.value(), "one");
}

HWTEST_F(ThreadSafeContainerUnitTest, Map_MoveSemantics, TestSize.Level1)
{
    ThreadSafeContainer<std::map<int, std::string>> mapContainer;
    
    std::string value = "test_value";
    mapContainer.Set(1, std::move(value));
    
    auto result = mapContainer.Get(1);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "test_value");
}

HWTEST_F(ThreadSafeContainerUnitTest, Set_BasicOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::set<int>> setContainer;
    
    setContainer.Insert(1);
    setContainer.Insert(2);
    setContainer.Insert(3);
    setContainer.Insert(2);
    
    EXPECT_EQ(setContainer.Size(), 3);
    EXPECT_FALSE(setContainer.Empty());
    
    EXPECT_TRUE(setContainer.Contains(2));
    EXPECT_FALSE(setContainer.Contains(5));
    
    setContainer.Remove(1);
    EXPECT_EQ(setContainer.Size(), 2);
    EXPECT_FALSE(setContainer.Contains(1));
}

HWTEST_F(ThreadSafeContainerUnitTest, Set_Modify, TestSize.Level1)
{
    ThreadSafeContainer<std::set<int>> setContainer;
    
    setContainer.Modify([](std::set<int>& s) {
        s.insert(10);
        s.insert(20);
        s.insert(30);
    });
    
    EXPECT_EQ(setContainer.Size(), 3);
    
    auto count = setContainer.ModifyWithResult([](std::set<int>& s) -> int {
        return s.size();
    });
    
    EXPECT_EQ(count, 3);
}

HWTEST_F(ThreadSafeContainerUnitTest, Set_Foreach, TestSize.Level1)
{
    ThreadSafeContainer<std::set<int>> setContainer;
    
    setContainer.Insert(1);
    setContainer.Insert(2);
    setContainer.Insert(3);
    
    int sum = 0;
    int count = 0;
    setContainer.Foreach([&sum, &count](int value) {
        sum += value;
        count++;
    });
    
    EXPECT_EQ(count, 3);
    EXPECT_EQ(sum, 6);
}

HWTEST_F(ThreadSafeContainerUnitTest, Set_FindIf, TestSize.Level1)
{
    ThreadSafeContainer<std::set<int>> setContainer;
    
    setContainer.Insert(1);
    setContainer.Insert(2);
    setContainer.Insert(3);
    
    auto found = setContainer.FindIf([](int value) {
        return value > 2;
    });
    
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), 3);
    
    auto notFound = setContainer.FindIf([](int value) {
        return value > 10;
    });
    
    EXPECT_FALSE(notFound.has_value());
}

HWTEST_F(ThreadSafeContainerUnitTest, Set_ForeachSnapshot, TestSize.Level1)
{
    ThreadSafeContainer<std::set<int>> setContainer;
    
    setContainer.Insert(1);
    setContainer.Insert(2);
    setContainer.Insert(3);
    
    int sum = 0;
    setContainer.ForeachSnapshot([&sum](int value) {
        sum += value;
    });
    
    EXPECT_EQ(sum, 6);
}

HWTEST_F(ThreadSafeContainerUnitTest, Vector_BasicOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::vector<int>> vectorContainer;
    
    vectorContainer.PushBack(1);
    vectorContainer.PushBack(2);
    vectorContainer.PushBack(3);
    
    EXPECT_EQ(vectorContainer.Size(), 3);
    EXPECT_FALSE(vectorContainer.Empty());
    
    auto front = vectorContainer.Front();
    EXPECT_TRUE(front.has_value());
    EXPECT_EQ(front.value(), 1);
    
    auto back = vectorContainer.Back();
    EXPECT_TRUE(back.has_value());
    EXPECT_EQ(back.value(), 3);
    
    vectorContainer.PopBack();
    EXPECT_EQ(vectorContainer.Size(), 2);
    
    auto newBack = vectorContainer.Back();
    EXPECT_EQ(newBack.value(), 2);
}

HWTEST_F(ThreadSafeContainerUnitTest, Vector_Modify, TestSize.Level1)
{
    ThreadSafeContainer<std::vector<int>> vectorContainer;
    
    vectorContainer.Modify([](std::vector<int>& v) {
        v.push_back(10);
        v.push_back(20);
        v.push_back(30);
    });
    
    EXPECT_EQ(vectorContainer.Size(), 3);
    
    auto sum = vectorContainer.ModifyWithResult([](std::vector<int>& v) -> int {
        int total = 0;
        for (int val : v) {
            total += val;
        }
        return total;
    });
    
    EXPECT_EQ(sum, 60);
}

HWTEST_F(ThreadSafeContainerUnitTest, Vector_Foreach, TestSize.Level1)
{
    ThreadSafeContainer<std::vector<int>> vectorContainer;
    
    vectorContainer.PushBack(1);
    vectorContainer.PushBack(2);
    vectorContainer.PushBack(3);
    
    int sum = 0;
    int count = 0;
    vectorContainer.Foreach([&sum, &count](int value) {
        sum += value;
        count++;
    });
    
    EXPECT_EQ(count, 3);
    EXPECT_EQ(sum, 6);
}

HWTEST_F(ThreadSafeContainerUnitTest, Vector_FindIf, TestSize.Level1)
{
    ThreadSafeContainer<std::vector<int>> vectorContainer;
    
    vectorContainer.PushBack(1);
    vectorContainer.PushBack(2);
    vectorContainer.PushBack(3);
    vectorContainer.PushBack(4);
    
    auto found = vectorContainer.FindIf([](int value) {
        return value % 2 == 0;
    });
    
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), 2);
    
    auto notFound = vectorContainer.FindIf([](int value) {
        return value > 10;
    });
    
    EXPECT_FALSE(notFound.has_value());
}

HWTEST_F(ThreadSafeContainerUnitTest, Vector_ForeachSnapshot, TestSize.Level1)
{
    ThreadSafeContainer<std::vector<int>> vectorContainer;
    
    vectorContainer.PushBack(1);
    vectorContainer.PushBack(2);
    vectorContainer.PushBack(3);
    
    int sum = 0;
    vectorContainer.ForeachSnapshot([&sum](int value) {
        sum += value;
    });
    
    EXPECT_EQ(sum, 6);
}

HWTEST_F(ThreadSafeContainerUnitTest, Vector_MoveSemantics, TestSize.Level1)
{
    ThreadSafeContainer<std::vector<std::string>> vectorContainer;
    
    std::string value = "test_value";
    vectorContainer.PushBack(std::move(value));
    
    EXPECT_EQ(vectorContainer.Size(), 1);
    auto front = vectorContainer.Front();
    EXPECT_EQ(front.value(), "test_value");
}

HWTEST_F(ThreadSafeContainerUnitTest, Vector_EmptyOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::vector<int>> vectorContainer;
    
    EXPECT_TRUE(vectorContainer.Empty());
    EXPECT_EQ(vectorContainer.Size(), 0);
    
    auto front = vectorContainer.Front();
    EXPECT_FALSE(front.has_value());
    
    auto back = vectorContainer.Back();
    EXPECT_FALSE(back.has_value());
    
    vectorContainer.PopBack();
    EXPECT_TRUE(vectorContainer.Empty());
}

HWTEST_F(ThreadSafeContainerUnitTest, Clear, TestSize.Level1)
{
    ThreadSafeContainer<std::map<int, int>> mapContainer;
    mapContainer.Set(1, 1);
    mapContainer.Set(2, 2);
    EXPECT_EQ(mapContainer.Size(), 2);
    
    mapContainer.Clear();
    EXPECT_TRUE(mapContainer.Empty());
    EXPECT_EQ(mapContainer.Size(), 0);
    
    ThreadSafeContainer<std::vector<int>> vectorContainer;
    vectorContainer.PushBack(1);
    vectorContainer.PushBack(2);
    EXPECT_EQ(vectorContainer.Size(), 2);
    
    vectorContainer.Clear();
    EXPECT_TRUE(vectorContainer.Empty());
    EXPECT_EQ(vectorContainer.Size(), 0);
}

HWTEST_F(ThreadSafeContainerUnitTest, UnorderedMap_BasicOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::unordered_map<int, std::string>> mapContainer;
    
    mapContainer.Set(1, "one");
    mapContainer.Set(2, "two");
    
    EXPECT_EQ(mapContainer.Size(), 2);
    
    auto value = mapContainer.Get(1);
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "one");
    
    EXPECT_TRUE(mapContainer.Contains(2));
    
    mapContainer.Remove(1);
    EXPECT_EQ(mapContainer.Size(), 1);
}

HWTEST_F(ThreadSafeContainerUnitTest, UnorderedSet_BasicOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::unordered_set<int>> setContainer;
    
    setContainer.Insert(1);
    setContainer.Insert(2);
    
    EXPECT_EQ(setContainer.Size(), 2);
    EXPECT_TRUE(setContainer.Contains(1));
    
    setContainer.Remove(1);
    EXPECT_EQ(setContainer.Size(), 1);
}

HWTEST_F(ThreadSafeContainerUnitTest, Deque_BasicOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::deque<int>> dequeContainer;
    
    dequeContainer.PushBack(1);
    dequeContainer.PushBack(2);
    
    EXPECT_EQ(dequeContainer.Size(), 2);
    
    auto front = dequeContainer.Front();
    EXPECT_TRUE(front.has_value());
    EXPECT_EQ(front.value(), 1);
    
    dequeContainer.PopBack();
    EXPECT_EQ(dequeContainer.Size(), 1);
}

HWTEST_F(ThreadSafeContainerUnitTest, List_BasicOperations, TestSize.Level1)
{
    ThreadSafeContainer<std::list<int>> listContainer;
    
    listContainer.Modify([](std::list<int>& l) {
        l.push_back(1);
        l.push_back(2);
    });
    
    EXPECT_EQ(listContainer.Size(), 2);
    
    int sum = 0;
    listContainer.Foreach([&sum](int value) {
        sum += value;
    });
    
    EXPECT_EQ(sum, 3);
}

HWTEST_F(ThreadSafeContainerUnitTest, InitializationWithValues, TestSize.Level1)
{
    std::map<int, std::string> initMap = {{1, "one"}, {2, "two"}};
    ThreadSafeContainer<std::map<int, std::string>> mapContainer(initMap);
    
    EXPECT_EQ(mapContainer.Size(), 2);
    EXPECT_TRUE(mapContainer.Contains(1));
    EXPECT_TRUE(mapContainer.Contains(2));
    
    std::vector<int> initVector = {1, 2, 3};
    ThreadSafeContainer<std::vector<int>> vectorContainer(initVector);
    
    EXPECT_EQ(vectorContainer.Size(), 3);
    auto front = vectorContainer.Front();
    EXPECT_EQ(front.value(), 1);
}

}
