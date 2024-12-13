/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef UNITTEST_UTILS_H
#define UNITTEST_UTILS_H

#include <cstdint>
#include <string>

#define AVCODEC_GTEST_RUN_TASK(testSuit, testCase)                                                                     \
    do {                                                                                                               \
        MultiThreadTest _test(g_thread_count);                                                                         \
        _test.runTask(_test.m_threadNum, TC_##testSuit##_##testCase, #testSuit, #testCase);                            \
    } while (0)

#define AVCODEC_MTEST_P(testSuit, testCase, level, tnum)                                                               \
    void TC_##testSuit##_##testCase();                                                                                 \
    HWTEST_P(testSuit, testCase, level)                                                                                \
    {                                                                                                                  \
        SET_THREAD_NUM(tnum);                                                                                          \
        AVCODEC_GTEST_RUN_TASK(testSuit, testCase);                                                                    \
    }                                                                                                                  \
    void TC_##testSuit##_##testCase()

namespace OHOS {
namespace MediaAVCodec {
[[maybe_unused]] void DecArgv(int &index, int &argc, char **argv);
[[maybe_unused]] int32_t GetNum(const char *str, const std::string key);

} // namespace MediaAVCodec
} // namespace OHOS
#endif // UNITTEST_UTILS_H