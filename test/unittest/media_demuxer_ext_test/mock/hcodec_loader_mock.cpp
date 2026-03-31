/*
 * Mock implementation of HCodecLoader for media_demuxer_ext_unit_test.
 * This avoids linking real codec engine libraries while satisfying
 * MediaDemuxer dependencies on HCodecLoader::GetCapabilityByMimeType.
 */

#include "hcodec_loader.h"

namespace OHOS {
namespace MediaAVCodec {

std::shared_ptr<CodecBase> HCodecLoader::CreateByName(const std::string &name)
{
    (void)name;
    return nullptr;
}

int32_t HCodecLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    (void)caps;
    return 0;
}

std::optional<CapabilityData> HCodecLoader::GetCapabilityByMimeType(const std::string &mimeType)
{
    (void)mimeType;
    CapabilityData cap;
    return cap;
}

} // namespace MediaAVCodec
} // namespace OHOS
