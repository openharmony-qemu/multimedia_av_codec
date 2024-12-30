#ifndef AUDIO_DECODER_ADAPTER_H
#define AUDIO_DECODER_ADAPTER_H

#include <shared_mutex>
#include <vector>
#include "surface.h"
#include "avcodec_video_decoder.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "osal/task/condition_variable.h"
#include "meta/format.h"
#include "video_sink.h"
#include "avcodec_audio_codec.h"
#include "media_codec.h"

namespace OHOS {
namespace Media {
class AudioDecoderAdapter : public std::enable_shared_from_this<AudioDecoderAdapter> {
public:
    AudioDecoderAdapter();
    virtual ~AudioDecoderAdapter();

    Status Configure(const std::shared_ptr<Meta> &parameter);

    Status Prepare();

    Status Start();

    Status Stop();

    Status Flush();

    Status Reset();

    Status Release();

    Status SetParameter(const std::shared_ptr<Meta> &parameter);

    Status SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer);

    int32_t GetOutputFormat(std::shared_ptr<Meta> &parameter);

    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue();

    int32_t NotifyEos();

    Status Init(bool isMimeType, const std::string &name);

    Status ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta);

    void SetDumpInfo(bool isDump, uint64_t instanceId);

    void OnDumpInfo(int32_t fd);

    void OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer);

    int32_t SetCodecCallback(const std::shared_ptr<AudioBaseCodecCallback> &codecCallback);

    int32_t SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag);

private:
    std::string StateToString(CodecState state);

    std::shared_ptr<MediaCodec> mediaCodec_;

    std::shared_ptr<MediaAVCodec::AVCodecAudioCodec> audiocodec_;
    
    sptr<Media::AVBufferQueueProducer> outputBufferQueueProducer_;

    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer_;

    bool isDump_ = false;

    std::string dumpPrefix_ = "";

    std::atomic<CodecState> state_;

    Mutex stateMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_DECODER_ADAPTER_H