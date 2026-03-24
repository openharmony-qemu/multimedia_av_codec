/*
 * Local mock header for HCodecLoader used only by media_demuxer_ext_unit_test.
 * It mirrors the public interface of the real HCodecLoader so that
 * media_demuxer.cpp can include "hcodec_loader.h" without pulling in
 * the full codec engine dependency.
 */

#ifndef HCODEC_LOADER_MOCK_H
#define HCODEC_LOADER_MOCK_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace OHOS {
namespace MediaAVCodec {

class CodecBase;
struct CapabilityData {
    std::string mimeType;
};

class HCodecLoader {
public:
    static std::shared_ptr<CodecBase> CreateByName(const std::string &name);
    static int32_t GetCapabilityList(std::vector<CapabilityData> &caps);
    static std::optional<CapabilityData> GetCapabilityByMimeType(const std::string &mimeType);
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // HCODEC_LOADER_MOCK_H
