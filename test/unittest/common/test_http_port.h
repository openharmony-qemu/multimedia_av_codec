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

#ifndef TEST_HTTP_PORT_H
#define TEST_HTTP_PORT_H

#ifndef AVCODEC_TEST_HTTP_PORT
#define AVCODEC_TEST_HTTP_PORT 46666
#endif

#define AVCODEC_TEST_HTTP_PORT_TO_STR(x) #x
#define AVCODEC_TEST_HTTP_PORT_STR_HELPER(x) AVCODEC_TEST_HTTP_PORT_TO_STR(x)
#define AVCODEC_TEST_HTTP_PORT_STR AVCODEC_TEST_HTTP_PORT_STR_HELPER(AVCODEC_TEST_HTTP_PORT)

#endif
