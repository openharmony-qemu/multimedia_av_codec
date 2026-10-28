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

#ifndef NATIVEIMAGEDEMO_EGL_RENDER_CONTEXT_H
#define NATIVEIMAGEDEMO_EGL_RENDER_CONTEXT_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include "external_window.h"

namespace OHOS {
namespace Media {
class EglRenderContext {
public:
    EglRenderContext();
    ~EglRenderContext() noexcept;

    // disallow copy and move
    EglRenderContext(const EglRenderContext &) = delete;
    EglRenderContext &operator=(const EglRenderContext &) = delete;
    void operator==(const EglRenderContext &) = delete;
    EglRenderContext(const EglRenderContext &&) = delete;
    EglRenderContext &operator=(EglRenderContext &&) = delete;
    void operator==(const EglRenderContext &&) = delete;

    bool Init();
    bool IsEglContextReady() const
    {
        return eglContext_ != EGL_NO_CONTEXT;
    }

    void SetHdrMode(bool hdrMode);

    EGLSurface CreateEglSurface(EGLNativeWindowType surface, const EGLint *attribList = nullptr);
    EGLSurface CreatePbufferSurface(int32_t width = 1, int32_t height = 1);
    void DestroyEglSurface(EGLSurface surface);
    void MakeCurrent(EGLSurface surface) const;
    void SwapBuffers(EGLSurface surface) const;
    bool SetPresentationTime(EGLSurface surface, EGLnsecsANDROID time);
    EGLImageKHR CreateEGLImage(OHNativeWindowBuffer *buffer);
    void DeleteEGLImage(EGLImageKHR image);
    void EGLImageTargetTexture2DOES(EGLImageKHR image);

protected:
    EGLDisplay eglDisplay_ = EGL_NO_DISPLAY;
    EGLContext eglContext_ = EGL_NO_CONTEXT;
    EGLConfig config_ = EGL_NO_CONFIG_KHR;
    bool isHdrMode_ = false;
};
}
}
#endif