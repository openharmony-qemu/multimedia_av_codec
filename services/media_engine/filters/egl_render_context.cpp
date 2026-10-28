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

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
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

void EglRenderContext::SetHdrMode(bool hdrMode)
{
    isHdrMode_ = hdrMode;
}

EglRenderContext::~EglRenderContext() noexcept
{
    if (eglDisplay_ == EGL_NO_DISPLAY) {
        return;
    }
    // 1. 解绑当前线程的 EGL 上下文和表面
    eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // 2. 释放当前线程持有的 EGL 资源
    eglReleaseThread();

    // 3. 销毁 EGL 上下文（如果存在）
    if (eglContext_ != EGL_NO_CONTEXT) {
        eglDestroyContext(eglDisplay_, eglContext_);
        eglContext_ = EGL_NO_CONTEXT;
    }

    // 4. 终止 EGL 显示连接
    if (eglDisplay_ != EGL_NO_DISPLAY) {
        eglTerminate(eglDisplay_);
        eglDisplay_ = EGL_NO_DISPLAY;
    }
}

bool EglRenderContext::Init()
{
    FALSE_RETURN_V_MSG(!IsEglContextReady(), true, "IsEglContextReady is ok");
    MEDIA_LOG_I("EglRenderContext EglRenderContext::Init begin.");
    eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    FALSE_RETURN_V_MSG(eglDisplay_ != EGL_NO_DISPLAY, false, "EglRenderContext::Init: failed to create eglDisplay");
    EGLint major;
    EGLint minor;
    FALSE_RETURN_V_MSG(eglInitialize(eglDisplay_, &major, &minor) != EGL_FALSE, 
        false, "eglInitialize failed to create");
    FALSE_RETURN_V_MSG(eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE,
        false, "eglInitialize failed to create");
    EGLint count;
    if (isHdrMode_) {
        // HDR模式: 优先选择10bit config (RGB10_A2: 10+10+10+2=32bit)
        EGLint configAttribs10bit[] = { EGL_SURFACE_TYPE,
                                       EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
                                       EGL_RED_SIZE,
                                       10,
                                       EGL_GREEN_SIZE,
                                       10,
                                       EGL_BLUE_SIZE,
                                       10,
                                       EGL_ALPHA_SIZE,
                                       2,
                                       EGL_RENDERABLE_TYPE,
                                       EGL_OPENGL_ES3_BIT,
                                       EGL_COLOR_BUFFER_TYPE,
                                       EGL_RGB_BUFFER,
                                       EGL_NONE };
        if (eglChooseConfig(eglDisplay_, configAttribs10bit, &config_, 1, &count) && count > 0) {
            MEDIA_LOG_I("EGL: 10bit config selected for HDR mode");
        } else {
            MEDIA_LOG_W("EGL: 10bit config not available, fallback to 8bit");
            EGLint configAttribs8bit[] = { EGL_SURFACE_TYPE,
                                           EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
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
            FALSE_RETURN_V_MSG(eglChooseConfig(eglDisplay_, configAttribs8bit, &config_, 1, &count) != EGL_FALSE,
                false, "Failed to choose EGL config (8bit fallback)");
        }
    } else {
        // SDR模式: 使用8bit配置
        EGLint configAttribs[] = { EGL_SURFACE_TYPE,
                                   EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
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
        FALSE_RETURN_V_MSG(eglChooseConfig(eglDisplay_, configAttribs, &config_, 1, &count) != EGL_FALSE,
            false, "Failed to bind choose config");
    }
    
    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3,
                                       EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE };

    eglContext_ = eglCreateContext(eglDisplay_, config_, EGL_NO_CONTEXT, contextAttribs);
    FALSE_RETURN_V_MSG(eglContext_ != EGL_NO_CONTEXT, false, "Failed to eglCreateContext");
    MEDIA_LOG_I("EglRenderContext EglRenderContext::Init end.");
    return true;
}

void EglRenderContext::MakeCurrent(EGLSurface surface) const
{
    EGLSurface currSurface = surface;
    if (!eglMakeCurrent(eglDisplay_, currSurface, currSurface, eglContext_)) {
        EGLint surfaceId = -1;
        eglQuerySurface(eglDisplay_, surface, EGL_CONFIG_ID, &surfaceId);
        MEDIA_LOG_E("MakeCurrent: Failed to make current on EGLSurface %{public}d, error is %{public}s.",
            surfaceId, GetEglErrorString());
    }
}

void EglRenderContext::SwapBuffers(EGLSurface surface) const
{
    EGLBoolean ret = eglSwapBuffers(eglDisplay_, surface);
    if (ret == EGL_FALSE) {
        EGLint surfaceId = -1;
        eglQuerySurface(eglDisplay_, surface, EGL_CONFIG_ID, &surfaceId);
        MEDIA_LOG_W("SwapBuffers: Failed to SwapBuffers on EGLSurface %{public}d, error is %{public}s.",
            surfaceId, GetEglErrorString());
    }
}

void EglRenderContext::DestroyEglSurface(EGLSurface surface)
{
    if (!eglDestroySurface(eglDisplay_, surface)) {
        EGLint surfaceId = -1;
        eglQuerySurface(eglDisplay_, surface, EGL_CONFIG_ID, &surfaceId);
        MEDIA_LOG_E("DestroyEglWindowSurface: Failed to DestroySurface on EGLSurface %{public}d, error is %{public}s.",
            surfaceId, GetEglErrorString());
    }
}

EGLSurface EglRenderContext::CreateEglSurface(EGLNativeWindowType eglNativeWindow, const EGLint *attribList)
{
    FALSE_RETURN_V_MSG(IsEglContextReady(), EGL_NO_SURFACE,
        "EglRenderContext::IsEglContextReady: EGL context has not initialized");

    FALSE_RETURN_V_MSG(eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT),
        EGL_NO_SURFACE, "eglMakeCurrent::CreateEglWindowSurface: EGL context has not initialized");

    EGLSurface surface = eglCreateWindowSurface(eglDisplay_, config_, eglNativeWindow, attribList);

    FALSE_RETURN_V_MSG(surface != EGL_NO_SURFACE,
        EGL_NO_SURFACE, "EglRenderContext::CreateEglWindowSurface: Failed to create eglSurface");
    return surface;
}

EGLSurface EglRenderContext::CreatePbufferSurface(int32_t width, int32_t height)
{
    FALSE_RETURN_V_MSG(IsEglContextReady(), EGL_NO_SURFACE,
        "CreatePbufferSurface: EGL context has not initialized");

    EGLint pbufferAttribs[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_NONE
    };
    EGLSurface surface = eglCreatePbufferSurface(eglDisplay_, config_, pbufferAttribs);
    FALSE_RETURN_V_MSG(surface != EGL_NO_SURFACE, EGL_NO_SURFACE,
        "CreatePbufferSurface: Failed to create pbuffer surface, error: %{public}s", GetEglErrorString());
    return surface;
}

EGLImageKHR EglRenderContext::CreateEGLImage(OHNativeWindowBuffer *buffer)
{
    FALSE_RETURN_V_MSG(IsEglContextReady() && buffer != nullptr, EGL_NO_IMAGE_KHR,
        "CreateEGLImage: EGL context not ready or buffer is null");

    EGLint attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    EGLImageKHR image = eglCreateImageKHR(eglDisplay_, eglContext_,
        EGL_NATIVE_BUFFER_OHOS, static_cast<EGLClientBuffer>(buffer), attrs);
    if (image == EGL_NO_IMAGE_KHR) {
        MEDIA_LOG_E("eglCreateImageKHR failed: %{public}s", GetEglErrorString());
    }
    return image;
}

void EglRenderContext::DeleteEGLImage(EGLImageKHR image)
{
    if (image != EGL_NO_IMAGE_KHR) {
        eglDestroyImageKHR(eglDisplay_, image);
    }
}

void EglRenderContext::EGLImageTargetTexture2DOES(EGLImageKHR image)
{
    FALSE_RETURN_MSG(image != EGL_NO_IMAGE_KHR, "EGLImageTargetTexture2DOES: image is null");
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, static_cast<GLeglImageOES>(image));
    GLenum error = glGetError();
    FALSE_RETURN_MSG(error == GL_NO_ERROR,
        "glEGLImageTargetTexture2DOES failed: GL error %{public}d", error);
}
}
}  // namespace OHOS::Media