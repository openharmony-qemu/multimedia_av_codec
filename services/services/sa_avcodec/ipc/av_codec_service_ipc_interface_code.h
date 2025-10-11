/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AV_CODEC_SERVICE_IPC_INTERFACE_CODE_H
#define AV_CODEC_SERVICE_IPC_INTERFACE_CODE_H

/* SAID: 3011 */
namespace OHOS {
namespace MediaAVCodec {
enum class CodecListenerInterfaceCode {
    ON_ERROR = 0,
    ON_OUTPUT_FORMAT_CHANGED,
    ON_INPUT_BUFFER_AVAILABLE,
    ON_OUTPUT_BUFFER_AVAILABLE,
    ON_OUTPUT_BUFFER_BINDED,
    ON_OUTPUT_BUFFER_UN_BINDED
};

enum class CodecServiceInterfaceCode {
    SET_LISTENER_OBJ = 0,
    INIT,
    CONFIGURE,
    PREPARE,
    START,
    STOP,
    FLUSH,
    RESET,
    RELEASE,
    NOTIFY_EOS,
    CREATE_INPUT_SURFACE,
    SET_OUTPUT_SURFACE,
    QUEUE_INPUT_BUFFER,
    GET_OUTPUT_FORMAT,
    RELEASE_OUTPUT_BUFFER,
    SET_PARAMETER,
    SET_INPUT_SURFACE,
    DEQUEUE_INPUT_BUFFER,
    DEQUEUE_OUTPUT_BUFFER,
    GET_INPUT_FORMAT,
    GET_CODEC_INFO,
    DESTROY_STUB,
    SET_DECRYPT_CONFIG,
    RENDER_OUTPUT_BUFFER_AT_TIME,
    SET_CUSTOM_BUFFER,
    GET_CHANNEL_ID,
    SET_LPP_MODE,
    NOTIFY_MEMORY_EXCHANGE,
    NOTIFY_FREEZE,
    NOTIFY_ACTIVE,
    NOTIFY_MEMORY_RECYCLE,
    NOTIFY_MEMORY_WRITE_BACK,
    NOTIFY_SUSPEND,
    NOTIFY_RESUME,
};

enum class AVCodecListServiceInterfaceCode {
    FIND_DECODER = 0,
    FIND_ENCODER,
    GET_CAPABILITY,
    DESTROY
};

enum class AVCodecServiceInterfaceCode {
    GET_SUBSYSTEM = 0,
    FREEZE,
    ACTIVE,
    ACTIVEALL,
    GET_ACTIVE_SECURE_DECODER_PIDS
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AV_CODEC_SERVICE_IPC_INTERFACE_CODE_H