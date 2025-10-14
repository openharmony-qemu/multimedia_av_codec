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

#ifndef HTTP_SERVER_MOCK_H
#define HTTP_SERVER_MOCK_H

// 虚假m3u8文件，需通过fuzz data 喂入
#define FAKE_FUZZ_M3U8  "fakefuzz.m3u8"

namespace OHOS::Media::Plugins::HttpPlugin {
    bool InitServer(void);
    bool CloseServer(void);

    bool InitServer(const uint8_t *data, size_t size);
}
#endif