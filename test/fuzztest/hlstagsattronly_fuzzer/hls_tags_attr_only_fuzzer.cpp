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
#include <string>
#include <list>
#include <memory>
#include <vector>
#include <algorithm>
#include <sstream>
#include "fuzzer/FuzzedDataProvider.h"
#include "hls_tags.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

static inline std::string RandToken(FuzzedDataProvider& fdp, size_t maxLen = 24)
{
    std::string s = fdp.ConsumeRandomLengthString(maxLen);

    // 避免把换行塞进同一行
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
        return c == '\r' || c == '\n';
    }), s.end());
    return s;
}

static inline std::string RandQuoted(FuzzedDataProvider& fdp, size_t maxLen = 24, bool withEsc = true)
{
    std::string out = "\"";
    std::string raw = fdp.ConsumeRandomLengthString(maxLen);
    for (char c : raw) {
        if (c == '"' && withEsc) {
            out.push_back('\\');
            out.push_back('"');
        } else if (c == '\n' || c == '\r') {
            out.push_back(' ');
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

static inline std::string RandUInt(FuzzedDataProvider& fdp, uint32_t lo, uint32_t hi)
{
    if (lo > hi) {
        std::swap(lo, hi);
    }
    return std::to_string(fdp.ConsumeIntegralInRange<uint32_t>(lo, hi));
}

static inline std::string RandFloat(FuzzedDataProvider& fdp, double lo, double hi)
{
    if (lo > hi) {
        std::swap(lo, hi);
    }
    std::ostringstream os;
    os.setf(std::ios::fmtflags(0), std::ios::floatfield);
    os << fdp.ConsumeFloatingPointInRange<double>(lo, hi);
    return os.str();
}

static inline std::string RandHexIV(FuzzedDataProvider& fdp, bool oddLen = false)
{
    static const char* hex = "0123456789ABCDEF";
    int n = fdp.ConsumeIntegralInRange<int>(2, 32);
    if (oddLen) {
        n |= 1;
    }
    std::string s = "0x";
    for (int i = 0; i < n; i++) s.push_back(hex[fdp.ConsumeIntegralInRange<int>(0, 15)]);
    return s;
}

static inline std::string RandResolution(FuzzedDataProvider& fdp)
{
    const char* pool[] = { "426x240", "640x360", "854x480", "1280x720", "1920x1080", "2560x1440", "3840x2160",
        "1x1", "0x1080", "9999x1" };
    return pool[fdp.ConsumeIntegralInRange<int>(0, (int)(sizeof(pool)/sizeof(pool[0]))-1)];
}

static inline std::string RandByterange(FuzzedDataProvider& fdp)   // "len@off" or "len"
{
    if (fdp.ConsumeBool()) {
        return RandUInt(fdp, 0, 1<<22) + "@" + RandUInt(fdp, 0, 1<<22);
    }
    return RandUInt(fdp, 0, 1<<22);
}

static inline std::string RandURI(FuzzedDataProvider& fdp)
{
    switch (fdp.ConsumeIntegralInRange<int>(0, 5)) {
        case 0: return "http://example.com/" + RandToken(fdp, 8) + ".ts";
        case 1: return "https://cdn.example.com/" + RandToken(fdp, 8);
        case 2: return "/" + RandToken(fdp, 6) + "/" + RandToken(fdp, 6) + ".m4s";
        case 3: return "./" + RandToken(fdp, 6) + ".mp4";
        case 4: return "//static.example.com/" + RandToken(fdp, 6) + ".aac";
        default: return RandToken(fdp, 10) + ".ts";
    }
}

static inline std::string KV(const std::string& k, const std::string& v, FuzzedDataProvider& fdp)
{
    auto sp = [&](void) {
        return fdp.ConsumeBool() ? " " : "";
    };
    return k + sp() + "=" + sp() + v;
}

// 构造属性密集型（不走下载）
static std::string BuildAttributesLine(FuzzedDataProvider& fdp, HlsTag tagType)
{
    std::vector<std::string> parts;
    auto push = [&](const std::string& k, const std::string& v) {
        parts.emplace_back(KV(k, v, fdp));
    };

    switch (tagType) {
        case HlsTag::EXTXSTREAMINF:
            push("BANDWIDTH", RandUInt(fdp, 0, 50'000'000));
            if (fdp.ConsumeBool()) push("AVERAGE-BANDWIDTH", RandUInt(fdp, 0, 50'000'000));
            if (fdp.ConsumeBool()) push("RESOLUTION", RandResolution(fdp));
            if (fdp.ConsumeBool()) push("FRAME-RATE", fdp.ConsumeBool() ? "23.976" : RandFloat(fdp, -10, 240));
            if (fdp.ConsumeBool()) push("CODECS", RandQuoted(fdp, 32));
            if (fdp.ConsumeBool()) push("AUDIO", RandQuoted(fdp, 10));
            if (fdp.ConsumeBool()) push("CLOSED-CAPTIONS", fdp.ConsumeBool() ? "\"NONE\"" : RandQuoted(fdp, 8));
            if (fdp.ConsumeBool()) push("STRANGE_KEY", RandQuoted(fdp, 8)); // 非法键，压榨容错
            break;
        case HlsTag::EXTXIFRAMESTREAMINF:
            push("BANDWIDTH", RandUInt(fdp, 1, 100'000'000));
            if (fdp.ConsumeBool()) push("RESOLUTION", RandResolution(fdp));
            if (fdp.ConsumeBool()) push("URI", RandQuoted(fdp, 24));
            if (fdp.ConsumeBool()) push("BYTERANGE", RandQuoted(fdp, 24));
            break;
        case HlsTag::EXTXMEDIA:
            push("TYPE", RandQuoted(fdp, 6));
            push("GROUP-ID", RandQuoted(fdp, 8));
            push("NAME", RandQuoted(fdp, 12));
            if (fdp.ConsumeBool()) push("DEFAULT", fdp.ConsumeBool() ? "YES" : "NO");
            if (fdp.ConsumeBool()) push("AUTOSELECT", fdp.ConsumeBool() ? "YES" : "NO");
            if (fdp.ConsumeBool()) push("FORCED", fdp.ConsumeBool() ? "YES" : "NO");
            if (fdp.ConsumeBool()) push("LANGUAGE", RandQuoted(fdp, 6));
            if (fdp.ConsumeBool()) push("URI", RandQuoted(fdp, 20));
            break;
        case HlsTag::EXTXSTART:
            push("TIME-OFFSET", fdp.ConsumeBool() ? RandFloat(fdp, -3600, 3600) : RandUInt(fdp, 0, 10000));
            push("PRECISE", fdp.ConsumeBool() ? "YES" : "NO");
            break;
        case HlsTag::EXTXSESSIONKEY:
        case HlsTag::EXTXKEY:
            // 安全：避免出发下载（允许空URI/内联PSSH字符串）
            push("METHOD", fdp.ConsumeBool() ? "AES-128" : "SAMPLE-AES");
            if (fdp.ConsumeBool()) {
                push("URI", "\"\"");
            } else {
                push("URI", RandQuoted(fdp, 24));
            }
            if (fdp.ConsumeBool()) push("IV", fdp.ConsumeBool() ? RandHexIV(fdp, false) : RandHexIV(fdp, true));
            if (fdp.ConsumeBool()) push("KEYFORMAT", RandQuoted(fdp, 10));
            if (fdp.ConsumeBool()) push("KEYFORMATVERSIONS", RandQuoted(fdp, 6));
            break;
        case HlsTag::EXTXMAP:
            push("URI", "\"\"");    // 早退，不下载
            if (fdp.ConsumeBool()) push("BYTERANGE", RandQuoted(fdp, 18));
            break;
        default:
            break;
    }

    std::ostringstream os;
    switch (tagType) {
        case HlsTag::EXTXSTREAMINF:       os << "#EXT-X-STREAM-INF:"; break;
        case HlsTag::EXTXIFRAMESTREAMINF: os << "#EXT-X-I-FRAME-STREAM-INF:"; break;
        case HlsTag::EXTXMEDIA:           os << "#EXT-X-MEDIA:"; break;
        case HlsTag::EXTXSTART:           os << "#EXT-X-START:"; break;
        case HlsTag::EXTXSESSIONKEY:      os << "#EXT-X-SESSION-KEY:"; break;
        case HlsTag::EXTXKEY:             os << "#EXT-X-KEY:"; break;
        case HlsTag::EXTXMAP:             os << "#EXT-X-MAP:"; break;
        default:                          os << "#EXT-X-MEDIA:"; break;
    }
    for (size_t i = 0; i < parts.size(); ++i) { if (i) os <<","; os << parts[i]; }
    return os.str();
}

static inline void PoundOnAttribute(const std::shared_ptr<Attribute>& a)
{
    if (!a) {
        return;
    }
    (void)a->Decimal();
    (void)a->FloatingPoint();
    (void)a->HexSequence();
    (void)a->GetByteRange();
    (void)a->GetResolution();
    (void)a->QuotedString();
    (void)a->UnescapeQuotes();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);

    // 1) 先构造若干“增强版”文本变体
    std::vector<std::string> texts;

    // a) 原始输入
    texts.emplace_back(std::string(reinterpret_cast<const char*>(data), size));

    // b) 注入属性密集标签（不触发下载）
    {
        std::ostringstream os;
        os << "#EXTM3U\n";
        int nAttrBlocks = fdp.ConsumeIntegralInRange<int>(1, 6);
        for (int i = 0; i < nAttrBlocks; ++i) {
            HlsTag t = static_cast<HlsTag>(fdp.ConsumeIntegralInRange<int>(
                (int)HlsTag::EXTXKEY, (int)HlsTag::EXTXIFRAMESTREAMINF));
            
            // 只挑AttributesTag范围
            if (t == HlsTag::EXTXKEY || t == HlsTag::EXTXSESSIONKEY || t == HlsTag::EXTXMAP ||
                t == HlsTag::EXTXMEDIA || t == HlsTag::EXTXSTART || t == HlsTag::EXTXSTREAMINF ||
                t == HlsTag::EXTXIFRAMESTREAMINF) {
                os << BuildAttributesLine(fdp, t) << "\n";
                if (t == HlsTag::EXTXSTREAMINF && fdp.ConsumeBool()) {
                    // master特性：紧随其后的URI行
                    os << RandURI(fdp) << "\n";
                }
            }
        }

        // c) 混入一些单值与列表标签
        int nLines = fdp.ConsumeIntegralInRange<int>(1, 16);
        for (int i = 0; i < nLines; ++i) {
            switch (fdp.ConsumeIntegralInRange<int>(0, 4)) {
                case 0: os << "#EXT-X-VERSION:" << RandUInt(fdp, 0, 20) << "\n"; break;
                case 1: os << "#EXT-X-BYTERANGE:" << RandByterange(fdp) << "\n"; break;
                case 2: os << "#EXT-X-PROGRAM-DATE-TIME:" << RandToken(fdp, 24) << "\n"; break;
                case 3: os << "#EXT-X-TARGETDURATION:" << RandUInt(fdp, 0, 120) << "\n"; break;
                default:
                    os << "#EXTINF:" << (fdp.ConsumeBool() ? "-1.5" : RandFloat(fdp, -1e6, 1e6));
                    if (fdp.ConsumeBool()) os << "," << RandToken(fdp, 32);
                    os << "\n" << RandURI(fdp) << "\n";
            }
        }
        if (fdp.ConsumeBool()) {
            os << "#EXT-X-ENDLIST\n";
        }
        texts.emplace_back(os.str());
    }

    // d) 拼接原始尾巴& CRLF/LF变体
    if (fdp.ConsumeBool()) {
        std::string m = texts.back() + fdp.ConsumeRemainingBytesAsString();
        std::replace(m.begin(), m.end(), '\n', '\r');   // 制造CR-only/混合行
        texts.emplace_back(std::move(m));
    }

    // 2) 逐文本解析并强打Attribute解码器
    for (const auto& s : texts) {
        std::list<std::shared_ptr<Tag>> tags = ParseEntries(s);
        for (auto& tg : tags) {
            if (!tg) continue;
            HlsTag tp = tg->GetType();

            // a) #EXTINF -> ValuesListTag
            if (tp == HlsTag::EXTINF) {
                auto* vt = static_cast<ValuesListTag*>(tg.get());
                if (vt) {
                    // DURATION / TITLE是values list中的两个“伪属性”
                    auto dur = vt->GetAttributeByName("DURATION");
                    auto ttl = vt->GetAttributeByName("TITLE");
                    PoundOnAttribute(dur);
                    PoundOnAttribute(ttl);
                }
                continue;
            }

            // b) AttributesTag范围
            if (tp == HlsTag::EXTXKEY || tp == HlsTag::EXTXSESSIONKEY || tp == HlsTag::EXTXMAP ||
                tp == HlsTag::EXTXMEDIA || tp == HlsTag::EXTXSTART || tp == HlsTag::EXTXSTREAMINF ||
                tp == HlsTag::EXTXIFRAMESTREAMINF) {
                auto* at = static_cast<AttributesTag*>(tg.get());
                if (at) {
                    // 常见/可疑键全打
                    static const char* keys[] = {
                        "BANDWIDTH", "AVERAGE-BANDWIDTH", "RESOLUTION", "FRAME-RATE", "CODECS",
                        "URI", "BYTERANGE", "IV", "TIME-OFFSET", "LANGUAGE", "KEYFORMAT", "KEYFORMATVERSIONS",
                        "GROUP-ID", "NAME", "DEFAULT", "AUTOSELECT", "FORCED", "CLOSED-CAPTIONS", "AUDIO", "VIDEO",
                        "SUBTITLES", "STRANGE_KEY"  // 有意的未知键
                    };
                    for (auto k : keys) {
                        PoundOnAttribute(at->GetAttributeByName(k));
                    }
                }
                continue;
            }

            // c) SingleValueTag范围：URI/VERSION/BYTERANGE/PROGRAM-DATE-TIME/
            // TARGETDURATION/MEDIA-SEQUENCE/DISCONTINUITY-SEQUENCE/PLAYLIST-TYPE
            if (tp == HlsTag::URI || tp == HlsTag::EXTXVERSION || tp == HlsTag::EXTXBYTERANGE ||
                tp == HlsTag::EXTXPROGRAMDATETIME || tp == HlsTag::EXTXTARGETDURATION ||
                tp == HlsTag::EXTXMEDIASEQUENCE || tp == HlsTag::EXTXDISCONTINUITYSEQUENCE ||
                tp == HlsTag::EXTXPLAYLISTTYPE) {
                auto* sv = static_cast<SingleValueTag*>(tg.get());
                if (sv) {
                    const Attribute& a = sv->GetValue();

                    // 尽可能调用各种取值路径
                    (void)a.Decimal();
                    (void)a.FloatingPoint();
                    (void)a.HexSequence();
                    (void)a.GetByteRange();
                    (void)a.GetResolution();
                    (void)a.QuotedString();
                    (void)a.UnescapeQuotes();
                }
                continue;
            }

            // d) 其他（如 EXTXDISCONTINUITY / EXTXENDLIST / EXTXIFRAMESONLY）-> 无值标签，跳过即可
        }
    }
    return 0;
}