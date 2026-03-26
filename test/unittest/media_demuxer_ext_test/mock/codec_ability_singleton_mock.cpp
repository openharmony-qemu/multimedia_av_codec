#include "codec_ability_singleton.h"

#include <optional>

namespace OHOS {
namespace MediaAVCodec {

CodecAbilitySingleton &CodecAbilitySingleton::GetInstance()
{
    static CodecAbilitySingleton instance;
    return instance;
}

CodecAbilitySingleton::CodecAbilitySingleton() = default;

CodecAbilitySingleton::~CodecAbilitySingleton() = default;

std::optional<CapabilityData> CodecAbilitySingleton::GetCapabilityByName(const std::string &name)
{
    CapabilityData cap;
    cap.mimeType = name;
    return cap;
}

} // namespace MediaAVCodec
} // namespace OHOS
