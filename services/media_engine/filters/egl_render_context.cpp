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
 
#include "egl_render_context.h"
#include <cstring>
#include "common/log.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "EglRenderContext" };
}
 
namespace OHOS {
namespace Media {
const std::int32_t BIT_SIZE = 8;
 
const char *GetEglErrorString()
{
    EGLint error = eglGetError();
    
    static const struct {
        EGLint code;
        const char* name;
    } errorTable[] = {
        { EGL_SUCCESS, "EGL_SUCCESS" },
        { EGL_NOT_INITIALIZED, "EGL_NOT_INITIALIZED" },
        { EGL_BAD_ACCESS, "EGL_BAD_ACCESS" },
        { EGL_BAD_ALLOC, "EGL_BAD_ALLOC" },
        { EGL_BAD_ATTRIBUTE, "EGL_BAD_ATTRIBUTE" },
        { EGL_BAD_CONTEXT, "EGL_BAD_CONTEXT" },
        { EGL_BAD_CONFIG, "EGL_BAD_CONFIG" },
        { EGL_BAD_CURRENT_SURFACE, "EGL_BAD_CURRENT_SURFACE" },
        { EGL_BAD_DISPLAY, "EGL_BAD_DISPLAY" },
        { EGL_BAD_SURFACE, "EGL_BAD_SURFACE" },
        { EGL_BAD_MATCH, "EGL_BAD_MATCH" },
        { EGL_BAD_PARAMETER, "EGL_BAD_PARAMETER" },
        { EGL_BAD_NATIVE_PIXMAP, "EGL_BAD_NATIVE_PIXMAP" },
        { EGL_BAD_NATIVE_WINDOW, "EGL_BAD_NATIVE_WINDOW" },
        { EGL_CONTEXT_LOST, "EGL_CONTEXT_LOST" },
    };
    
    for (const auto& entry : errorTable) {
        if (entry.code == error) {
            return entry.name;
        }
    }
    return "Unknown Error";
}
 
using GetPlatformDisplayExt = PFNEGLGETPLATFORMDISPLAYEXTPROC;
 
EglRenderContext::EglRenderContext() {}
 
EglRenderContext::~EglRenderContext() noexcept
{
    if (eglDisplay_ == EGL_NO_DISPLAY) {
        return;
    }
 
    eglDestroyContext(eglDisplay_, eglContext_);
    eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(eglDisplay_);
    eglReleaseThread();
 
    eglDisplay_ = EGL_NO_DISPLAY;
    eglContext_ = EGL_NO_CONTEXT;
}
 
bool EglRenderContext::Init()
{
    FALSE_RETURN_V_MSG(!IsEglContextReady(), true, "IsEglContextReady is ok");
    MEDIA_LOG_I("EglRenderContext", "EglRenderContext::Init begin.");
    eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    FALSE_RETURN_V_MSG(eglDisplay_ != EGL_NO_DISPLAY, false, "EglRenderContext::Init: failed to create eglDisplay");
    EGLint major;
    EGLint minor;
    FALSE_RETURN_V_MSG(eglInitialize(eglDisplay_, &major, &minor) != EGL_FALSE,
        false, "eglInitialize failed to create");
    FALSE_RETURN_V_MSG(eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE,
        false, "eglInitialize failed to create");
    EGLint count;
    EGLint configAttribs[] = { EGL_SURFACE_TYPE,
                               EGL_WINDOW_BIT,
                               EGL_RED_SIZE,
                               BIT_SIZE,
                               EGL_GREEN_SIZE,
                               BIT_SIZE,
                               EGL_BLUE_SIZE,
                               BIT_SIZE,
                               EGL_ALPHA_SIZE,
                               BIT_SIZE,
                               EGL_RENDERABLE_TYPE,
                               EGL_OPENGL_ES3_BIT,
                               EGL_NONE };
    if (eglChooseConfig(eglDisplay_, configAttribs, &config_, 1, &count) == EGL_FALSE) {
        MEDIA_LOG_E("EglRenderContext",
            "EglRenderContext::Init: Failed to bind choose config, error: %{public}s.", GetEglErrorString());
        return false;
    }
    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    eglContext_ = eglCreateContext(eglDisplay_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (eglContext_ == EGL_NO_CONTEXT) {
        MEDIA_LOG_E("EglRenderContext",
            "EglRenderContext::Init: Failed to bind create context, error: %{public}s.", GetEglErrorString());
        return false;
    }
 
    MEDIA_LOG_I("EglRenderContext", "EglRenderContext::Init end.");
    return true;
}

void EglRenderContext::MakeCurrent(EGLSurface surface) const
{
    EGLSurface currSurface = surface;
    if (!eglMakeCurrent(eglDisplay_, currSurface, currSurface, eglContext_)) {
        EGLint surfaceId = -1;
        eglQuerySurface(eglDisplay_, surface, EGL_CONFIG_ID, &surfaceId);

        MEDIA_LOG_E("EglRenderContext",
            "EglRenderContext::MakeCurrent: Failed to make current on EGLSurface %{public}d, error is %{public}s.",
            surfaceId, GetEglErrorString());
    }
}

void EglRenderContext::SwapBuffers(EGLSurface surface) const
{
    EGLBoolean ret = eglSwapBuffers(eglDisplay_, surface);
    if (ret == EGL_FALSE) {
        EGLint surfaceId = -1;
        eglQuerySurface(eglDisplay_, surface, EGL_CONFIG_ID, &surfaceId);
        MEDIA_LOG_W("EglRenderContext",
            "EglRenderContext::SwapBuffers: Failed to SwapBuffers on EGLSurface %{public}d, error is %{public}s.",
            surfaceId, GetEglErrorString());
    }
}
 
void EglRenderContext::DestroyEglSurface(EGLSurface surface)
{
    if (!eglDestroySurface(eglDisplay_, surface)) {
        EGLint surfaceId = -1;
        eglQuerySurface(eglDisplay_, surface, EGL_CONFIG_ID, &surfaceId);
        MEDIA_LOG_E("EglRenderContext",
            "EglRenderContext::DestroyEglWindowSurface: Failed to DestroySurface on EGLSurface %{public}d, "
            "error is %{public}s.", surfaceId, GetEglErrorString());
    }
}
 
EGLSurface EglRenderContext::CreateEglSurface(EGLNativeWindowType eglNativeWindow, const EGLint *attribList)
{
    if (!IsEglContextReady()) {
        MEDIA_LOG_E("EglRenderContext",
            "EglRenderContext::CreateEglWindowSurface: EGL context has not initialized");
        return EGL_NO_SURFACE;
    }
 
    if (!eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        MEDIA_LOG_E("EglRenderContext",
            "EglRenderContext::CreateEglWindowSurface: Failed to make current no surface, error is %{public}s.",
            GetEglErrorString());
    }
 
    EGLSurface surface = eglCreateWindowSurface(eglDisplay_, config_, eglNativeWindow, attribList);
    if (surface == EGL_NO_SURFACE) {
        MEDIA_LOG_E("EglRenderContext",
            "EglRenderContext::CreateEglWindowSurface: Failed to create eglSurface, error is %{public}s.",
            GetEglErrorString());
        return EGL_NO_SURFACE;
    }
 
    return surface;
}
}
}  // namespace OHOS::Media