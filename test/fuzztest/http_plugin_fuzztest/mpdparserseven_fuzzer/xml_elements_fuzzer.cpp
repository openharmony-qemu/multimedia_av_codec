/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include "dash_mpd_parser.h"
#include "xml_parser.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

class FuzzXmlElementParser : public DashMpdParser {
public:
    // 暴露受保护的解析方法用于fuzz测试
    void FuzzParseSegmentTimeline(const uint8_t *data, size_t size) {
        if (size < 50) {
            return;
        }

        try {
            std::string xmlData(reinterpret_cast<const char*>(data), size);
            auto xmlParser = std::make_shared<XmlParser>();

            if (xmlParser->ParseFromBuffer(xmlData.c_str(), size) == 0) {
                auto rootElement = xmlParser->GetRootElement();
                if (rootElement) {
                    DashList<DashSegTimeline *> segTmlineList;
                    ParseSegmentTimeline(xmlParser, rootElement, segTmlineList);

                    // 清理资源
                    while (!segTmlineList.empty()) {
                        delete segTmlineList.front();
                        segTmlineList.pop_front();
                    }
                }
            }
        } catch (...) {
            // 忽略异常
        }
    }
    
    void FuzzParseContentProtection(const uint8_t *data, size_t size) {
        if (size < 30) {
            return;
        }

        try {
            std::string xmlData(reinterpret_cast<const char*>(data), size);
            auto xmlParser = std::make_shared<XmlParser>();

            if (xmlParser->ParseFromBuffer(xmlData.c_str(), size) == 0) {
                auto rootElement = xmlParser->GetRootElement();
                if (rootElement) {
                    DashList<DashDescriptor *> contentProtectionList;
                    ParseContentProtection(xmlParser, rootElement, contentProtectionList);

                    // 清理资源
                    while (!contentProtectionList.empty()) {
                        delete contentProtectionList.front();
                        contentProtectionList.pop_front();
                    }
                }
            }
        } catch (...) {
            // 忽略异常
        }
    }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 20 || size > 32 * 1024) {
        return 0;
    }

    FuzzXmlElementParser parser;

    // 根据数据的第一个字节决定测试哪个函数
    uint8_t selector = data[0] % 3;

    switch (selector) {
        case 0:
            parser.FuzzParseSegmentTimeline(data + 1, size - 1);
            break;
        case 1:
            parser.FuzzParseContentProtection(data + 1, size - 1);
            break;
        case 2:
            // 测试完整的MPD解析
            {
                std::vector<char> mpdData(size);
                memcpy(mpdData.data(), data, size);
                parser.ParseMPD(mpdData.data(), static_cast<uint32_t>(size));
                parser.Clear();
            }
            break;
    }

    return 0;
}