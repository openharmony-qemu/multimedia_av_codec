#ifndef QEMU_AUDIO_PROVENANCE505_H
#define QEMU_AUDIO_PROVENANCE505_H
// Private source/input observation only. No output clock or HW onset oracle.
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <utility>

namespace Qap505 {
inline constexpr const char *KEY = "ohos.qemu.audio.source_provenance_v1";
inline constexpr size_t MAX_PREFIX = 128;
inline constexpr size_t MAX_DESCRIPTOR = 1024;
inline constexpr uint64_t MAX_ROWS = 4096;
using Emitter = void (*)(const std::string &);
template<class F> inline void Observe(F f) noexcept
{
#if defined(__cpp_exceptions)
    try { f(); } catch (...) {}
#else
    f();
#endif
}

inline uint64_t NextId() noexcept
{
    static std::atomic<uint64_t> counter{0};
    uint64_t old = counter.load();
    while (old != UINT64_MAX) {
        if (counter.compare_exchange_weak(old, old + 1)) { return old + 1; }
    }
    return 0;
}
inline std::string Hex(const uint8_t *data, size_t size)
{
    static const char digits[] = "0123456789abcdef";
    std::string value; value.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        value.push_back(digits[data[i] >> 4]); value.push_back(digits[data[i] & 15]);
    }
    return value;
}
inline uint32_t Rotate(uint32_t value, unsigned amount) noexcept
{
    return (value >> amount) | (value << (32 - amount));
}
// SHA-256 of a bounded diagnostic sample. No raw sample bytes leave this helper.
inline std::string SampleSha256(const uint8_t *data, size_t size)
{
    if (size > MAX_PREFIX || (!data && size)) { return {}; }
    const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t state[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    uint8_t padded[MAX_PREFIX + 64] = {};
    for (size_t i = 0; i < size; ++i) { padded[i] = data[i]; }
    padded[size] = 0x80; size_t total = ((size + 9 + 63) / 64) * 64;
    uint64_t bits = size * 8;
    for (unsigned i = 0; i < 8; ++i) { padded[total - 1 - i] = static_cast<uint8_t>(bits >> (8 * i)); }
    for (size_t offset = 0; offset < total; offset += 64) {
        uint32_t w[64];
        for (unsigned i = 0; i < 16; ++i) {
            const uint8_t *p = padded + offset + 4 * i;
            w[i] = (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
        }
        for (unsigned i = 16; i < 64; ++i) {
            uint32_t a = w[i - 15], b = w[i - 2];
            w[i] = w[i - 16] + (Rotate(a,7) ^ Rotate(a,18) ^ (a >> 3)) + w[i - 7] +
                (Rotate(b,17) ^ Rotate(b,19) ^ (b >> 10));
        }
        uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],hh=state[7];
        for (unsigned i = 0; i < 64; ++i) {
            uint32_t t1 = hh + (Rotate(e,6) ^ Rotate(e,11) ^ Rotate(e,25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            uint32_t t2 = (Rotate(a,2) ^ Rotate(a,13) ^ Rotate(a,22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;state[4]+=e;state[5]+=f;state[6]+=g;state[7]+=hh;
    }
    uint8_t digest[32];
    for (unsigned i = 0; i < 8; ++i) for (unsigned j = 0; j < 4; ++j) {
        digest[4 * i + j] = static_cast<uint8_t>(state[i] >> (24 - 8 * j));
    }
    return Hex(digest, sizeof(digest));
}
inline uint64_t ParseBirthTicks(const char *text, size_t size) noexcept
{
    if (!text || !size || size >= 4096) { return 0; }
    size_t close = size;
    while (close && text[close - 1] != ')') { --close; }
    if (!close || close >= size || text[close] != ' ') { return 0; }
    size_t position = close;
    for (unsigned field = 3; field <= 22; ++field) {
        while (position < size && text[position] == ' ') { ++position; }
        size_t begin = position;
        while (position < size && text[position] != ' ' && text[position] != '\n') { ++position; }
        if (begin == position || position == size) { return 0; }
        if (field == 22) {
            uint64_t value = 0;
            for (size_t i = begin; i < position; ++i) {
                unsigned digit = static_cast<unsigned char>(text[i]) - '0';
                if (digit > 9 || value > (UINT64_MAX - digit) / 10) { return 0; }
                value = value * 10 + digit;
            }
            return value;
        }
    }
    return 0;
}
inline uint64_t BirthTicks() noexcept
{
    int fd = ::open("/proc/self/stat", O_RDONLY | O_CLOEXEC);
    if (fd < 0) { return 0; }
    char text[4096]; ssize_t size = ::read(fd, text, sizeof(text) - 1); char extra;
    ssize_t tail = size > 0 ? ::read(fd, &extra, 1) : -1;
    ::close(fd);
    return size > 0 && tail == 0 ? ParseBirthTicks(text, static_cast<size_t>(size)) : 0;
}
struct Source {
    uint64_t pid = 0, birth = 0, generation = 0, dev = 0, ino = 0, offset = 0, length = 0;
    uint64_t wav = 0, dataOffset = 0, dataBytes = 0, rate = 0, channels = 0, sampleBytes = 0, prefixBytes = 0;
    std::string prefix;
};
inline std::string Encode(const Source &s)
{
    std::ostringstream out; out << "qap505v1:" << s.pid << ':' << s.birth << ':' << s.generation << ':'
        << s.dev << ':' << s.ino << ':' << s.offset << ':' << s.length << ':' << s.wav << ':'
        << s.dataOffset << ':' << s.dataBytes << ':' << s.rate << ':' << s.channels << ':' << s.sampleBytes
        << ':' << s.prefixBytes << ':' << s.prefix;
    return out.str();
}
inline bool Decode(const std::string &text, Source &s)
{
    if (text.size() > MAX_DESCRIPTOR || text.compare(0, 9, "qap505v1:") != 0) { return false; }
    std::vector<std::string> fields; size_t start = 9;
    while (true) {
        size_t end = text.find(':', start);
        fields.push_back(text.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos) { break; } start = end + 1;
    }
    if (fields.size() != 15) { return false; }
    uint64_t numbers[14];
    for (size_t i = 0; i < 14; ++i) {
        if (fields[i].empty() || fields[i].size() > 20) { return false; }
        uint64_t number = 0;
        for (char c : fields[i]) {
            if (c < '0' || c > '9' || number > (UINT64_MAX - static_cast<unsigned>(c - '0')) / 10) { return false; }
            number = number * 10 + static_cast<unsigned>(c - '0');
        }
        numbers[i] = number;
    }
    if (!numbers[0] || !numbers[2] || numbers[7] > 1 || numbers[13] > MAX_PREFIX ||
        (numbers[13] ? fields[14].size() != 64 : !fields[14].empty())) { return false; }
    for (char c : fields[14]) { if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) { return false; } }
    if (numbers[7] && (!numbers[10] || !numbers[11] || !numbers[12] || numbers[8] > numbers[6] ||
        numbers[9] > numbers[6] - numbers[8] || numbers[11] > 8 || numbers[12] > 4 ||
        numbers[9] % (numbers[11] * numbers[12]) || numbers[13] > numbers[9])) { return false; }
    s = {numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7],
        numbers[8], numbers[9], numbers[10], numbers[11], numbers[12], numbers[13], fields[14]};
    return Encode(s) == text;
}
inline uint32_t U32(const uint8_t *p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline bool ReadAt(int fd, uint64_t position, void *data, size_t size) noexcept
{
    if (position > uint64_t(INT64_MAX) || size > uint64_t(INT64_MAX) - position) { return false; }
    return ::pread(fd, data, size, static_cast<off_t>(position)) == static_cast<ssize_t>(size);
}
inline Source Probe(int fd, int64_t offset, int64_t length) noexcept
{
    Source s; s.pid = static_cast<uint64_t>(::getpid()); s.birth = BirthTicks(); s.generation = NextId();
#if defined(__cpp_exceptions)
    try {
#endif
        struct stat st{};
        if (offset < 0 || length <= 0 || ::fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 0 ||
            uint64_t(offset) > uint64_t(st.st_size) || uint64_t(length) > uint64_t(st.st_size) - uint64_t(offset)) { return s; }
        s.dev = st.st_dev; s.ino = st.st_ino; s.offset = offset; s.length = length;
        uint8_t head[12];
        if (s.length < 12 || !ReadAt(fd, s.offset, head, 12) || std::string(reinterpret_cast<char *>(head), 4) != "RIFF" ||
            std::string(reinterpret_cast<char *>(head + 8), 4) != "WAVE") { return s; }
        uint64_t limit = uint64_t(U32(head + 4)) + 8;
        if (limit < 12 || limit > s.length) { return s; }
        bool format = false, data = false; uint64_t cursor = 12;
        for (unsigned chunk = 0; chunk < 128 && cursor <= limit && limit - cursor >= 8; ++chunk) {
            uint8_t hdr[8]; if (!ReadAt(fd, s.offset + cursor, hdr, 8)) { return s; }
            uint64_t size = U32(hdr + 4), begin = cursor + 8;
            if (size > limit - begin) { return s; }
            std::string kind(reinterpret_cast<char *>(hdr), 4);
            if (kind == "fmt ") {
                uint8_t fmt[16]; if (format || size < 16 || !ReadAt(fd, s.offset + begin, fmt, 16)) { return s; }
                uint32_t encoding = fmt[0] | (uint32_t(fmt[1]) << 8), bits = fmt[14] | (uint32_t(fmt[15]) << 8);
                s.channels = fmt[2] | (uint32_t(fmt[3]) << 8); s.rate = U32(fmt + 4); s.sampleBytes = bits / 8;
                uint32_t align = fmt[12] | (uint32_t(fmt[13]) << 8);
                if (encoding != 1 || bits != 16 || !s.rate || s.rate > 192000 || !s.channels || s.channels > 8 ||
                    align != s.channels * s.sampleBytes || U32(fmt + 8) != s.rate * align) { return s; }
                format = true;
            } else if (kind == "data") {
                if (data) { return s; } s.dataOffset = begin; s.dataBytes = size; data = true;
            }
            uint64_t pad = size & 1;
            if (size + pad > limit - begin) { return s; } cursor = begin + size + pad;
        }
        if (!format || !data || !s.dataBytes || s.dataBytes % (s.channels * s.sampleBytes)) { return s; }
        uint8_t prefix[MAX_PREFIX]; size_t amount = std::min<uint64_t>(MAX_PREFIX, s.dataBytes);
        if (!ReadAt(fd, s.offset + s.dataOffset, prefix, amount)) { return s; }
        struct stat afterStat{};
        if (::fstat(fd, &afterStat) || afterStat.st_dev != st.st_dev || afterStat.st_ino != st.st_ino || afterStat.st_size != st.st_size ||
            afterStat.st_mtime != st.st_mtime || afterStat.st_ctime != st.st_ctime) { return s; }
        s.prefix = SampleSha256(prefix, amount); s.prefixBytes = amount; s.wav = 1;
#if defined(__cpp_exceptions)
    } catch (...) { s.wav = 0; }
#endif
    return s;
}
inline void EmitSource(const Source &source, Emitter emit) noexcept
{
    Observe([&] {
        if (emit) { emit("{\"schema\":\"qap505-raw-v1\",\"kind\":\"source_native\",\"source_descriptor\":\"" +
            Encode(source) + "\",\"source_frame_relation_known\":false,\"accepted\":false,\"onset_proven\":false}"); }
    });
}
inline void EmitForwarded(const std::string &descriptor, Emitter emit) noexcept
{
    Observe([&] {
        Source source;
        if (emit && Decode(descriptor, source)) {
            emit("{\"schema\":\"qap505-raw-v1\",\"kind\":\"metadata_forwarded\",\"source_descriptor\":\"" +
                descriptor + "\",\"source_frame_relation_known\":false,\"accepted\":false,\"onset_proven\":false}");
        }
    });
}
inline void EmitLifecycle(const char *kind, uint64_t generation, int32_t result, Emitter emit) noexcept
{
    Observe([&] {
        if (emit) { emit(std::string("{\"schema\":\"qap505-raw-v1\",\"kind\":\"") + kind +
            "\",\"source_generation\":" + std::to_string(generation) + ",\"native_result\":" + std::to_string(result) +
            ",\"source_frame_relation_known\":false,\"accepted\":false,\"onset_proven\":false}"); }
    });
}
struct Snapshot {
    Source source; std::string descriptor; uint64_t plugin = 0; int32_t query = -1; uint32_t session = 0;
    uint64_t accepted = 0, binding = 0; bool invalid = false;
};
struct Slice {
    std::shared_ptr<Snapshot> snapshot; uint64_t begin = 0, bytes = 0, cacheId = 0;
    Slice Subspan(uint64_t used) const noexcept
    {
        if (used > bytes || begin > UINT64_MAX - used) { return {}; }
        return {snapshot, begin + used, bytes - used, cacheId};
    }
};
struct Cached {
    std::vector<uint8_t> bytes; Slice provenance;
    Cached(std::vector<uint8_t> data, const Slice &slice) : bytes(std::move(data)), provenance(slice)
    {
        if (!provenance.cacheId) { provenance.cacheId = NextId(); }
    }
    uint8_t *data() { return bytes.data(); }
    size_t size() const { return bytes.size(); }
};
class Ledger {
public:
    explicit Ledger(Emitter emit = nullptr) : emit_(emit), plugin_(NextId()), receiverPid_(::getpid()), receiverBirth_(BirthTicks()) {}
    void SetEmitter(Emitter emit) noexcept { emit_ = emit; }
    void Bind(const std::string &descriptor, int32_t query, uint32_t session) noexcept
    {
        Safe([&] {
            std::lock_guard<std::mutex> lock(mutex_); Source source;
            if (!Decode(descriptor, source)) { current_.reset(); Record("unknown", {}, 0, 0, 0, "bad_metadata"); return; }
            if (current_ && current_->descriptor == descriptor && current_->query == query && current_->session == session) {
                Record("metadata_consumed", {current_, 0, 0}, 0, 0, 0, "repeat"); return;
            }
            current_ = std::make_shared<Snapshot>(); current_->source = source; current_->descriptor = descriptor;
            current_->plugin = plugin_; current_->binding = NextId(); current_->query = query; current_->session = session; ingress_ = 0;
            Record("metadata_consumed", {current_, 0, 0}, 0, 0, 0, "new");
        });
    }
    Slice Input(const uint8_t *data, size_t size) noexcept
    {
        Slice slice;
        Safe([&] {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!current_) { return; }
            if (!data || size > UINT64_MAX - ingress_) { Record("unknown", {}, 0, 0, 0, "input"); return; }
            slice = {current_, ingress_, size}; ingress_ += size;
            Record("input", slice, size, 0, 0, "sha256:" + SampleSha256(data, std::min<size_t>(MAX_PREFIX, size)) +
                ":bytes:" + std::to_string(std::min<size_t>(MAX_PREFIX, size)));
        });
        return slice;
    }
    void Write(const Slice &slice, size_t requested, int32_t returned, uint32_t frameBytes, int32_t query, uint32_t session) noexcept
    {
        Safe([&] {
            std::lock_guard<std::mutex> lock(mutex_); uint64_t accepted = 0;
            if (!slice.snapshot) { return; }
            bool bounded = requested <= slice.bytes && returned >= 0 && static_cast<uint64_t>(returned) <= requested;
            bool valid = bounded && query == 0 && slice.snapshot->query == 0 && session == slice.snapshot->session &&
                frameBytes && slice.begin % frameBytes == 0 && requested % frameBytes == 0 &&
                static_cast<uint64_t>(returned) % frameBytes == 0;
            if (bounded && static_cast<uint64_t>(returned) <= UINT64_MAX - slice.snapshot->accepted) {
                accepted = returned; slice.snapshot->accepted += accepted;
            } else if (returned > 0) { slice.snapshot->invalid = true; }
            if (!valid) { slice.snapshot->invalid = true; }
            Record("write", slice, requested, returned, accepted, valid ? "valid_return" : "unknown_return_or_session", query, session);
        });
    }
    void Event(const char *kind, const Slice &slice, const char *reason) noexcept
    {
        Safe([&] { std::lock_guard<std::mutex> lock(mutex_); if (slice.snapshot) { Record(kind, slice, 0, 0, 0, reason); } });
    }
    void Reset(const char *reason) noexcept
    {
        Safe([&] { std::lock_guard<std::mutex> lock(mutex_); if (current_) { Record(reason, {current_, 0, 0}, 0, 0, 0, reason); } current_.reset(); ingress_ = 0; });
    }
private:
    template<class F> void Safe(F f) noexcept
    {
#if defined(__cpp_exceptions)
        try { f(); } catch (...) {}
#else
        f();
#endif
    }
    void Record(const char *kind, const Slice &slice, uint64_t requested, int64_t returned, uint64_t accepted, const std::string &detail, int32_t writeQuery = -1, uint32_t writeSession = 0)
    {
        if (!emit_ || rows_ >= MAX_ROWS) { return; }
        if (rows_ == MAX_ROWS - 1) {
            ++rows_;
            emit_("{\"schema\":\"qap505-raw-v1\",\"kind\":\"unknown\",\"detail\":\"observation_limit\",\"source_frame_relation_known\":false,\"accepted\":false,\"onset_proven\":false}");
            return;
        }
        auto s = slice.snapshot; std::ostringstream out;
        out << "{\"schema\":\"qap505-raw-v1\",\"kind\":\"" << kind << "\",\"seq\":" << ++rows_
            << ",\"receiver_pid\":" << receiverPid_ << ",\"receiver_birth_ticks\":" << receiverBirth_
            << ",\"plugin_instance\":" << plugin_ << ",\"metadata_binding_instance\":" << (s ? s->binding : 0) << ",\"source_pid\":" << (s ? s->source.pid : 0)
            << ",\"source_birth_ticks\":" << (s ? s->source.birth : 0) << ",\"source_generation\":" << (s ? s->source.generation : 0)
            << ",\"renderer_session_known\":" << (s && s->query == 0 ? "true" : "false")
            << ",\"renderer_session_id\":" << (s ? s->session : 0) << ",\"query_status\":" << (s ? s->query : -1)
            << ",\"write_renderer_session_known\":" << (writeQuery == 0 ? "true" : "false")
            << ",\"write_renderer_session_id\":" << writeSession << ",\"write_query_status\":" << writeQuery
            << ",\"ingress_byte_begin\":" << slice.begin << ",\"byte_count\":" << slice.bytes
            << ",\"cache_id\":" << slice.cacheId << ",\"old_generation\":" << (s && s != current_ ? "true" : "false")
            << ",\"write_requested_bytes\":" << requested << ",\"raw_returned_bytes\":" << returned
            << ",\"write_accepted_bytes\":" << accepted << ",\"accepted_bytes_total\":" << (s ? s->accepted : 0)
            << ",\"mapping_invalid\":" << (s && s->invalid ? "true" : "false") << ",\"detail\":\"" << detail
            << "\",\"source_frame_relation_known\":false,\"accepted\":false,\"onset_proven\":false}";
        emit_(out.str());
    }
    Emitter emit_; uint64_t plugin_, receiverPid_, receiverBirth_, ingress_ = 0, rows_ = 0; std::mutex mutex_; std::shared_ptr<Snapshot> current_;
};
} // namespace Qap505
#endif
