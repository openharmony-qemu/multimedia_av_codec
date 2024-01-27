#ifndef M3U8_UNIT_TEST_H
#define M3U8_UNIT_TEST_H

#include "gtest/gtest.h"
#include "m3u8.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class M3u8UnitTest : public testing::Test {
public:
    M3u8UnitTest() {}

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
    M3U8 *m3u8_ = nullptr;
}
}
}
}