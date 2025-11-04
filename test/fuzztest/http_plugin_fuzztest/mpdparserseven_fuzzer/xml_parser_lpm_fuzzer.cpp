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
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>
#include <sys/stat.h>
#include "xml.pb.h"
#include "xml/xml_parser.h"
#include "xml/xml_element.h"
#include "src/libfuzzer/libfuzzer_macro.h"

using namespace OHOS::Media::Plugins::HttpPlugin;
namespace p = ohos::xml;

namespace {
void WalkNode(const std::shared_ptr<XmlElement>& start,
    const std::vector<std::string>& attrs, uint32_t budget)
{
    auto cur = start;
    uint32_t visited = 0;
    while (cur && visited < budget) {
        (void)cur->GetName();
        (void)cur->GetText();
        for (size_t i = 0; i < attrs.size() && i < 8; ++i) {
            (void)cur->GetAttribute(attrs[i]);
        }
        auto child = cur->GetChild();
        if (child) {
            (void)child->GetName();
            (void)child->GetText();
        }
        cur = cur->GetSiblingNext();
        ++visited;
    }
}

std::string TmpDir()
{
    struct stat st{};
    if (::stat("/dev/shm", &st) == 0 && S_ISDIR(st.st_mode)) {
        return "/dev/shm";
    }
    if (::stat("/tmp", &st) == 0 && S_ISDIR(st.st_mode)) {
        return "/tmp";
    }
    return ".";
}

bool WriteFileAll(const std::string& path, const std::string& data)
{
    FILE* f = ::fopen(path.c_str(), "wb");
    if (!f) {
        return false;
    }
    if (!data.empty()) {
        (void)::fwrite(data.data(), 1, data.size(), f);
    }
    ::fclose(f);
    return true;
}
} // namespace


DEFINE_PROTO_FUZZER(const p::XmlInput& in)
{
    if (!in.has_xml()) {
        return;
    }
    std::string xml(in.xml().begin(), in.xml().end());
    uint32_t budget = in.has_max_walk() ? in.max_walk() : 1000;

    std::vector<std::string> attrs(in.attr_name().begin(), in.attr_name().end());
    if (attrs.empty()) {
        static const char* kAttrSeeds[] = {
            "id", "bandwidth", "width", "height", "frameRate", "codecs", "mimeType",
            "duration", "startNumber", "timescale", "presentationTimeOffset",
            "indexRange", "indexRangeExact", "schemeIdUri", "value", "media", "lang"
        };
        attrs.assign(std::begin(kAttrSeeds), std::end(kAttrSeeds));
    }

    // Buffer
    {
        XmlParser p;
        p.ParseFromBuffer(xml.c_str(), static_cast<int32_t>(xml.size()));
        auto root = p.GetRootElement();
        if (root) {
            WalkNode(root, attrs, budget);
        }
        p.DestroyDoc();
    }

    // String
    if (!in.has_parse_twice() || in.parse_twice()){
        XmlParser p;
        p.ParseFromString(xml);
        auto root = p.GetRootElement();
        if (root) {
            WalkNode(root, attrs, budget);
        }
        p.DestroyDoc();
    }

    // File
    if (!in.has_use_file() || in.use_file()) {
        XmlParser p;
        const std::string dir = TmpDir();
        std::string path = dir + "/xml_lpm_" + std::to_string(::getpid()) + ".xml";

        if (in.has_use_bad_path() && in.use_bad_path()) {
            path = dir + "/no_such_dir_/bad.xml";
        }

        const bool empty = in.has_empty_file() && in.empty_file();
        if (!in.has_use_bad_path() || !in.use_bad_path()) {
            (void)WriteFileAll(path, empty ? std::string() : xml);
            if (in.has_perm_deny() && in.perm_deny()) {
                ::chmod(path.c_str(), 0000);
            }
        }

        p.ParseFromFile(path);
        auto root = p.GetRootElement();
        if (root) {
            WalkNode(root, attrs, budget);
        }
        p.DestroyDoc();

        if (!in.has_use_bad_path() || !in.use_bad_path()) {
            if (in.has_perm_deny() && in.perm_deny()) {
                ::chmod(path.c_str(), 0600);
            }
            ::unlink(path.c_str());
        }
    }
}