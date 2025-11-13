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
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "xml/xml_parser.h"
#include "xml/xml_element.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

namespace {

constexpr int kMaxWalk = 1000;

void WalkNode(const std::shared_ptr<XmlElement>& start,
    const std::vector<std::string>& attrs, int budget)
{
    auto cur = start;
    int visited = 0;
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

std::string PickTmpDir()
{
    static const char* kDirs[] = { "/dev/shm", "/tmp", "." };
    struct stat st{};
    for (auto d : kDirs) {
        if (::stat(d, &st) == 0 && S_ISDIR(st.st_mode)) {
            return std::string(d);
        }
    }
    return std::string(".");
}

bool WriteFileAll(const std::string& path, const std::string& data)
{
    int fd = ::open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (fd < 0) {
        return false;
    }
    ssize_t off = 0;
    while (off < (ssize_t)data.size()) {
        ssize_t w = ::write(fd, data.data() + off, data.size() - off);
        if (w <= 0) {
            ::close(fd);
            return false;
        }
        off += w;
    }
    ::close(fd);
    return true;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return 0;
    }
    if (size > (1 << 20)) { // 1MB 上限，防 OOM
        return 0;
    }

    uint8_t mode = data[0];
    const char* body = reinterpret_cast<const char*>(data + 1);
    size_t body_size = size - 1;

    // 组装一个 XML 缓冲
    std::string xml;
    switch (mode % 4) {
        case 0: xml.assign(body, body_size); break;
        case 1: 
            xml = "<r>";
            xml.append(body, body_size);
            xml += "</r>";
            break;
        case 2:
            xml = "<?xml version=\"1.0\"?><root><![CDATA[";
            xml.append(body, body_size);
            xml += "]]></root>";
            break;
        default:
            xml = "<mpd:MPD xmlns:mpd=\"urn:mpeg:dash:schema:mpd:2011\"><Period";
            xml += " start=\"PT10S\"><AdaptationSet mimeType=\"video/mp4\"><Representation bandwidth=\"";
            xml += std::to_string((unsigned)mode * 1000);
            xml += "\"/></AdaptationSet></Period></mpd:MPD>";
            break;
    }

    // 常见属性名
    static const char* kAttrSeeds[] = {
        "id", "bandwidth", "width", "height", "frameRate", "codecs", "mimeType",
        "duration", "startNumber", "timescale", "presentationTimeOffset",
        "indexRange", "indexRangeExact", "schemeIdUri", "value", "media",
        "mediaRange", "index", "BaseURL", "profiles", "lang"
    };
    std::vector<std::string> attrs(std::begin(kAttrSeeds), std::end(kAttrSeeds));

    // 1) ParseFromBuffer
    {
        XmlParser p;
        p.ParseFromBuffer(xml.c_str(), static_cast<int32_t>(xml.size()));
        auto root = p.GetRootElement();
        if (root) {
            WalkNode(root, attrs, kMaxWalk);
        }
        p.DestroyDoc();
    }

    // 2) ParseFromString
    {
        XmlParser p;
        p.ParseFromString(xml);
        auto root = p.GetRootElement();
        if (root) {
            WalkNode(root, attrs, kMaxWalk);
        }
        p.DestroyDoc();
    }

    // 3) ParseFromFile —— 覆盖文件路径/权限/空文件等分支
    {
        const std::string tmpdir = PickTmpDir();
        std::string good_path = tmpdir + "/xmlfuzz_" + std::to_string(::getpid()) + "_" + std::to_string((unsigned)mode) + ".xml";
        std::string bad_path  = tmpdir + "/no_such_dir_" + std::to_string((unsigned)mode) + "/bad.xml"; // 大概率打开失败

        XmlParser p;

        // (a) 正常路径
        if (WriteFileAll(good_path, xml)) {
            p.ParseFromFile(good_path);
            auto root = p.GetRootElement();
            if (root) {
                WalkNode(root, attrs, kMaxWalk);
            }
            p.DestroyDoc();

            // (b) 权限拒绝
            ::chmod(good_path.c_str(), 0000);
            p.ParseFromFile(good_path); // 期望走 open 失败/读失败分支
            p.DestroyDoc();
            ::chmod(good_path.c_str(), 0600);
            ::unlink(good_path.c_str());
        }

        // (c) 空文件
        std::string empty_path = tmpdir + "/xmlfuzz_empty_" + std::to_string(::getpid()) + ".xml";
        if (WriteFileAll(empty_path, "")) {
            p.ParseFromFile(empty_path);
            (void)p.GetRootElement(); // 看空文件错误处理
            p.DestroyDoc();
            ::unlink(empty_path.c_str());
        }

        // (d) 明确的坏路径（不存在目录）
        p.ParseFromFile(bad_path);
        p.DestroyDoc();
    }

    return 0;
}