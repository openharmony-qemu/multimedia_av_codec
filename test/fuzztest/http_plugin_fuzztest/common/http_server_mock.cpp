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
#include <iostream>
#include "http_server_demo.h"
#include "http_server_mock.h"
#include "http_server_fuzz_demo.h"

namespace OHOS::Media::Plugins::HttpPlugin {
    
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
std::unique_ptr<MediaAVCodec::HttpServerFuzzDemo> g_serverFuzz = nullptr;

bool InitServer(void)
{
    if (g_server != nullptr) {
        std::cout << "Server has been started error" << std::endl;
        return false;
    }
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
    std::cout << "Server start ..." << std::endl;
    return true;
}

bool CloseServer(void)
{
    if (g_server != nullptr) {
        g_server->StopServer();
        g_server = nullptr;
    }
    if (g_serverFuzz != nullptr) {
        g_serverFuzz->StopServer();
        g_serverFuzz = nullptr;
    }
    return true;
}

bool InitServer(const uint8_t *data, size_t size)
{
    if (g_serverFuzz != nullptr) {
        std::cout << "Server has been started error" << std::endl;
        return false;
    }
    if (data == nullptr || size <= sizeof(int32_t)) {
        std::cout << "No enough fuzz data, size: " << size << std::endl;
        return false;
    }
    g_serverFuzz = std::make_unique<MediaAVCodec::HttpServerFuzzDemo>(data, size);
    g_serverFuzz->StartServer();
    std::cout << "Server fuzz start ... size " << size << std::endl;
    return true;
}
}