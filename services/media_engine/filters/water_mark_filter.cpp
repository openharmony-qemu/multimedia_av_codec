/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "water_mark_filter.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"
#include "avcodec_log.h"
#include "external_window.h"
#include "native_window.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "WaterMarkFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {

const uint32_t MAX_WATERMARK_NUMBER = 5;
const uint64_t MAX_WATERMARK_SIZE = 80 * 1024 * 1024;
const uint32_t ROTATE_90 = 90;
const uint32_t ROTATE_180 = 180;
const uint32_t ROTATE_270 = 270;
const uint32_t MIN_VERTICES_SIZE = 16;
/**
 * @brief 顶点着色器：处理顶点位置和纹理坐标
 */
std::string vertexShader = R"delimiter(
attribute vec3 position;
attribute vec2 texCoord;

varying vec2 vTexCoord;

uniform mat4 matTransform;

void main()
{
    gl_Position = vec4(position, 1.0);
    vTexCoord = texCoord;
}
)delimiter";

/**
 * @brief 片段着色器：将视频帧与合并后的水印纹理进行Alpha混合叠加
 * 视频已通过glViewport缩放到目标尺寸，水印已在目标画布上合并，直接叠加即可
 */
std::string fragmentShader = R"delimiter(
#extension GL_OES_EGL_image_external : require
precision highp float;

varying vec2 vTexCoord;

uniform samplerExternalOES texture;
uniform sampler2D textTexture;

void main()
{
    // 翻转纹理坐标以匹配视频数据方向
    vec2 flippedTexCoord = vec2(1.0 - vTexCoord.x, 1.0 - vTexCoord.y);

    // 直接采样视频和水印纹理（视频已缩放到目标尺寸，水印已在目标画布上合并）
    vec4 videoColor = texture2D(texture, flippedTexCoord);
    vec4 watermarkColor = texture2D(textTexture, flippedTexCoord);

    // Alpha混合：水印与视频融合
    float r = watermarkColor.r * watermarkColor.a + videoColor.r * (1.0 - watermarkColor.a);
    float g = watermarkColor.g * watermarkColor.a + videoColor.g * (1.0 - watermarkColor.a);
    float b = watermarkColor.b * watermarkColor.a + videoColor.b * (1.0 - watermarkColor.a);

    gl_FragColor = vec4(r, g, b, 1.0);
}
)delimiter";

std::string fragmentShader11 = R"delimiter(
#extension GL_OES_EGL_image_external : require
precision highp float;

varying vec2 vTexCoord;

// 仅保留外部视频纹理
uniform samplerExternalOES texture;

void main()
{
    // 保留你原来的纹理坐标翻转逻辑
    vec2 flippedTexCoord = vec2(1.0 - vTexCoord.x, 1.0 - vTexCoord.y);

    // 仅采样视频纹理
    vec4 videoColor = texture2D(texture, flippedTexCoord);

    // 直接输出视频颜色
    gl_FragColor = vec4(videoColor.rgb, 1.0);
}
)delimiter";

/**
 * @brief 片段着色器：仅进行视频缩放，不进行水印叠加
 * 用于仅需要视频缩放功能的场景
 */
std::string fragmentShaderResize = R"delimiter(
#extension GL_OES_EGL_image_external : require
precision highp float;
 
varying vec2 vTexCoord;
 
uniform samplerExternalOES texture;
 
void main()
{
    vec2 flippedTexCoord = vec2(1.0 - vTexCoord.x, 1.0 - vTexCoord.y);
    vec4 videoColor = texture2D(texture, flippedTexCoord);
    gl_FragColor = vec4(videoColor.rgb, 1.0);
}
)delimiter";

GLfloat vertices[] = {
    -1.0f, 1.0f,  0.0f,    1.0f, 1.0f,  // 左上角
    -1.0f, -1.0f, 0.0f,    1.0f, 0.0f,  // 左下角
    1.0f,  -1.0f, 0.0f,    0.0f, 0.0f,  // 右下角
    1.0f,  1.0f,  0.0f,    0.0f, 1.0f,  // 右上角
};

// 索引数据：定义两个三角形组成四边形
GLuint indices[] = {
    0, 1, 2,  // 第一个三角形
    0, 2, 3   // 第二个三角形
};


static AutoRegisterFilter<WaterMarkFilter> g_registerWaterMarkFilter("builtin.transcoder.watermark",
    FilterType::WATERMARK,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<WaterMarkFilter>(name, FilterType::WATERMARK);
    });

class WaterMarkFilterLinkCallback : public FilterLinkCallback {
public:
    explicit WaterMarkFilterLinkCallback(std::shared_ptr<WaterMarkFilter> waterMarkFilter)
        : waterMarkFilter_(std::move(waterMarkFilter))
    {
    }

    ~WaterMarkFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        auto filter = waterMarkFilter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        auto filter = waterMarkFilter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        auto filter = waterMarkFilter_.lock();
        FALSE_RETURN_MSG(filter != nullptr, "filter is nullptr");
        filter->OnUpdatedResult(meta);
    }
private:
    std::weak_ptr<WaterMarkFilter> waterMarkFilter_;
};

bool WaterMarkFilter::CreateNativeImage()
{
    nativeImage_ = OH_NativeImage_Create(-1, GL_TEXTURE_EXTERNAL_OES);
    FALSE_RETURN_V_MSG(nativeImage_ != nullptr, false, "Failed to create native image");
    nativeImageWindow_ = OH_NativeImage_AcquireNativeWindow(nativeImage_);
    FALSE_RETURN_V_MSG(nativeImageWindow_ != nullptr, false, "Failed to acquire native window");
    int ret = 0;
    nativeImageFrameAvailableListener_.context = this;
    nativeImageFrameAvailableListener_.onFrameAvailable = &WaterMarkFilter::OnNativeImageFrameAvailable;
    ret = OH_NativeImage_SetOnFrameAvailableListener(nativeImage_, nativeImageFrameAvailableListener_);
    FALSE_RETURN_V_MSG(ret == 0, false, "Failed to set frame available listener: %{public}d", ret);
    return true;
}

void WaterMarkFilter::UpdateNativeWindow(void *window, uint64_t width, uint64_t height)
{
    auto nativeWindow = reinterpret_cast<OHNativeWindow *>(window);
    nativeWindow_ = nativeWindow;
    FALSE_RETURN_MSG(nativeWindow_ != nullptr, "nativeWindow_ is nullptr");
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_BUFFER_GEOMETRY, static_cast<int>(width),
        static_cast<int>(height));
}

void WaterMarkFilter::ThreadMainLoop()
{
    MEDIA_LOG_I("WaterMarkFilter::ThreadMainLoop()");
    // 初始化EGL上下文
    FALSE_RETURN_MSG(InitializeEGLContext(), "Failed to initialize EGL context");
    FALSE_RETURN_MSG(CreateGLResources(), "Failed to create GL resources");
    if (!watermarkList_.empty()) {
        FALSE_RETURN_MSG(CreateWatermarkTextures(), "Failed to create watermark textures");
        FALSE_RETURN_MSG(MergeWaterMarksWithOpenGL(), "Failed to merge watermarks with OpenGL");
    }
    CreateVideoTexture();
    if (!watermarkList_.empty()) {
        FALSE_RETURN_MSG(CreateWatermarkTextureAndCopy(), "Failed to create watermark texture and copy");
    }
    FALSE_RETURN_MSG(SetupShaderUniforms(), "Failed to setup shader uniforms");

    // 将Native Image与OpenGL纹理关联
    OH_NativeImage_AttachContext(nativeImage_, nativeImageTexId_);
    RenderLoop();
}

bool WaterMarkFilter::InitializeEGLContext()
{
    // 创建EGL渲染上下文
    renderContext_ = std::make_unique<EglRenderContext>();
    FALSE_RETURN_V_MSG(renderContext_ != nullptr, false, "Failed to create EGL render context");

    // 初始化EGL：初始化显示、配置EGL、创建上下文
    FALSE_RETURN_V_MSG(renderContext_->Init(), false, "Failed to initialize EGL render context");

    // 创建EGL表面，将OpenGL渲染输出关联到Native Window
    eglSurface_ = renderContext_->CreateEglSurface(static_cast<EGLNativeWindowType>(nativeWindow_));
    FALSE_RETURN_V_MSG(eglSurface_ != EGL_NO_SURFACE, false, "Failed to create EGL surface");

    // 将EGL上下文和表面绑定到当前线程
    renderContext_->MakeCurrent(eglSurface_);
    return true;
}

void WaterMarkFilter::DrawImage(const int64_t pts)
{
    MEDIA_LOG_I("WaterMarkFilter::DrawImage");
    // 水印已在目标画布上合并，因此视频和水印都在同一目标尺寸空间显示
    glViewport(0, 0, targetWidth_.load(), targetHeight_.load());
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(vertexArrayObject_);
    glEnable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
    GLenum error = glGetError();
    FALSE_RETURN_MSG(error == GL_NO_ERROR, "DrawImage GL error: %{public}d", error);
    FALSE_RETURN_MSG(nativeWindow_ != nullptr, "nativeWindow_ is nullptr");
    OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, SET_UI_TIMESTAMP, pts);
    renderContext_->SwapBuffers(eglSurface_);
}

/**
 * @brief 为每个水印创建OpenGL纹理
 * @return true：创建成功；false：创建失败
 */
bool WaterMarkFilter::CreateWatermarkTextures()
{
    FALSE_RETURN_V_MSG(!watermarkList_.empty(), false, "watermarkList is empty, skip CreateWatermarkTextures");

    watermarkTexIds_.resize(watermarkList_.size());
    glGenTextures(watermarkList_.size(), watermarkTexIds_.data());

    for (size_t i = 0; i < watermarkList_.size(); i++) {
        const auto& config = watermarkList_[i];

        FALSE_RETURN_V_MSG(config.buffer != nullptr, false, "Watermark config buffer is nullptr");

        glBindTexture(GL_TEXTURE_2D, watermarkTexIds_[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.rawWidth, config.rawHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, config.buffer);

        GLenum error = glGetError();
        FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "Failed to create watermark texture");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    MEDIA_LOG_I("CreateWatermarkTextures success, created %{public}zu textures", watermarkList_.size());
    return true;
}

bool WaterMarkFilter::CreateVideoTexture()
{
    glGenTextures(1, &nativeImageTexId_);
    GLenum error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glGenTextures failed with GL error: %{public}d", error);

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBindTexture failed with GL error: %{public}d", error);

    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, 
        "Failed to create native image texture, GL error: %{public}d", error);
    return true;
}

bool WaterMarkFilter::CreateWatermarkTextureAndCopy()
{
    // 水印已经在MergeWaterMarksWithOpenGL()中直接在目标尺寸画布上合并
    textTexId_ = mergedWatermarkTexId_;
    MEDIA_LOG_I("CreateWatermarkTextureAndCopy success, target: %{public}dx%{public}d", targetWidth_.load(),
        targetHeight_.load());

    return true;
}

bool WaterMarkFilter::SetupShaderUniforms()
{
    // 设置着色器Uniform变量 - 视频纹理
    shaderProgram_->Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    shaderProgram_->SetInt("texture", 0);

    // 仅在有水印时设置水印纹理相关uniform
    if (!watermarkList_.empty()) {
        // 设置着色器Uniform变量 - 水印纹理
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textTexId_);
        shaderProgram_->SetMatrix4v("matTransform", matrix_, 16, false);
        shaderProgram_->SetInt("textTexture", 1);
    }

    MEDIA_LOG_I("SetShaderUniform video: %{public}dx%{public}d -> target: %{public}dx%{public}d, watermark: %{public}s",
        videoWidth_.load(), videoHeight_.load(), targetWidth_.load(), targetHeight_.load(),
        watermarkList_.empty() ? "no" : "yes");

    return true;
}

void WaterMarkFilter::RenderLoop()
{
    while (running_) {
        MEDIA_LOG_I("RenderLoop availableFrameCnt_");
        bool flag = false;
        std::unique_lock<std::mutex> lock(wakeUpMutex_);
        wakeUpCond_.wait(lock, [this]() { return availableFrameCnt_.load() > 0 || eosPts_.load() != -1 || !running_;});
        if (availableFrameCnt_.load() > 0) {
            availableFrameCnt_.fetch_sub(1, std::memory_order_relaxed);
            currentFrameNum_.fetch_add(1, std::memory_order_relaxed);
            flag = true;
        }

        if (flag) {
            FALSE_CONTINUE_LOGI(renderContext_ != nullptr, "renderContext_ is null");
            // 确保EGL上下文和表面在当前线程生效
            renderContext_->MakeCurrent(eglSurface_);
            OH_NativeImage_UpdateSurfaceImage(nativeImage_);
            lastPts_.store(OH_NativeImage_GetTimestamp(nativeImage_));
            // 执行渲染操作
            DrawImage(lastPts_.load());
        }

        if (eosPts_.load() == lastPts_.load()) {
            MEDIA_LOG_I("RenderLoop pts %{public}ld %{public}ld", lastPts_.load(), eosPts_.load());
            NotifyNextFilterEos();
            break;
        }
    }
}

bool WaterMarkFilter::CreateWatermarkFBO(int32_t width, int32_t height)
{
    glGenFramebuffers(1, &watermarkFBO_);
    GLenum error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glGenFramebuffers failed with GL error: %{public}d", error);

    glGenTextures(1, &mergedWatermarkTexId_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glGenTextures failed with GL error: %{public}d", error);

    glBindTexture(GL_TEXTURE_2D, mergedWatermarkTexId_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBindTexture failed with GL error: %{public}d", error);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glTexImage2D failed with GL error: %{public}d", error);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, watermarkFBO_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBindFramebuffer failed with GL error: %{public}d", error);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mergedWatermarkTexId_, 0);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glFramebufferTexture2D failed with GL error: %{public}d", error);

    FALSE_RETURN_V_MSG(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, false,
                       "Framebuffer is not complete");

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false,
                       "CreateWatermarkFBO failed with GL error: %{public}d", error);

    return true;
}

bool WaterMarkFilter::CreateWatermarkMergeShader()
{
    // 创建水印合并专用的顶点着色器
    std::string watermarkVertexShader = R"delimiter(
attribute vec2 position;      // 顶点位置（NDC坐标）
attribute vec2 texCoord;      // 纹理坐标
varying vec2 vTexCoord;       // 传递给片段着色器
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    vTexCoord = texCoord;
}
)delimiter";

    // 片段着色器：直接输出纹理颜色（不做混合，混合在GPU端通过GL_BLEND实现）
    std::string watermarkFragmentShader = R"delimiter(
precision highp float;
varying vec2 vTexCoord;
uniform sampler2D watermarkTexture;
void main()
{
    vec4 color = texture2D(watermarkTexture, vTexCoord);
    gl_FragColor = color;
}
)delimiter";

    // 创建着色器程序
    watermarkMergeShaderProgram_ = std::make_unique<ShaderProgram>(watermarkVertexShader, watermarkFragmentShader);
    FALSE_RETURN_V_MSG(watermarkMergeShaderProgram_->Valid(), false,
                       "Failed to create watermark merge shader program");

    // 创建VAO和VBO用于水印渲染
    glGenVertexArrays(1, &watermarkVAO_);
    GLenum error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glGenVertexArrays failed with GL error: %{public}d", error);

    glGenBuffers(1, &watermarkVBO_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glGenBuffers failed with GL error: %{public}d", error);

    glBindVertexArray(watermarkVAO_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBindVertexArray failed with GL error: %{public}d", error);

    glBindBuffer(GL_ARRAY_BUFFER, watermarkVBO_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBindBuffer failed with GL error: %{public}d", error);

    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false,
                       "CreateWatermarkMergeShader failed with GL error: %{public}d", error);

    return true;
}

void WaterMarkFilter::CalculateRotatedWatermarkVertices(const WaterMarkConfig& configTemp,
    int32_t width, int32_t height, float wmVertices[16], size_t wmVerticesSize)
{
    FALSE_RETURN_V_MSG(wmVerticesSize >= MIN_VERTICES_SIZE, void(), "wmVerticesSize is too small");
    WaterMarkConfig config = configTemp;
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;

    switch (rotation_.load()) {
        case ROTATE_90:
            config.top = targetHeight_.load() - configTemp.left - configTemp.watermarkWidth;
            config.left = configTemp.top;
            config.watermarkWidth = configTemp.watermarkHeight;
            config.watermarkHeight = configTemp.watermarkWidth;
            x = static_cast<float>(config.left) * 2.0f / width - 1.0f;
            y = -1.0f + static_cast<float>(config.top) * 2.0f / height;
            w = static_cast<float>(config.watermarkWidth) * 2.0f / width;
            h = static_cast<float>(config.watermarkHeight) * 2.0f / height;
            wmVertices[0] = x; wmVertices[1] = y + h; wmVertices[2] = 0.0f; wmVertices[3] = 0.0f;  // 左上
            wmVertices[4] = x; wmVertices[5] = y;     wmVertices[6] = 1.0f; wmVertices[7] = 0.0f;  // 左下
            wmVertices[8] = x + w; wmVertices[9] = y; wmVertices[10] = 1.0f; wmVertices[11] = 1.0f; // 右下
            wmVertices[12] = x + w; wmVertices[13] = y + h; wmVertices[14] = 0.0f; wmVertices[15] = 1.0f; // 上下反的
            break;
        case ROTATE_270:
            config.left = targetWidth_.load() - configTemp.top - configTemp.watermarkHeight;
            config.top = configTemp.left;
            config.watermarkWidth = configTemp.watermarkHeight;
            config.watermarkHeight = configTemp.watermarkWidth;
            x = static_cast<float>(config.left) * 2.0f / width - 1.0f;
            y = -1.0f + static_cast<float>(config.top) * 2.0f / height;
            w = static_cast<float>(config.watermarkWidth) * 2.0f / width;
            h = static_cast<float>(config.watermarkHeight) * 2.0f / height;
            wmVertices[0] = x; wmVertices[1] = y + h; wmVertices[2] = 1.0f; wmVertices[3] = 1.0f;  // 左上
            wmVertices[4] = x; wmVertices[5] = y;     wmVertices[6] = 0.0f; wmVertices[7] = 1.0f;  // 左下
            wmVertices[8] = x + w; wmVertices[9] = y; wmVertices[10] = 0.0f; wmVertices[11] = 0.0f; // 右下
            wmVertices[12] = x + w; wmVertices[13] = y + h; wmVertices[14] = 1.0f; wmVertices[15] = 0.0f; // 右上
            break;
        case ROTATE_180:
            config.left = targetWidth_.load() - configTemp.left - configTemp.watermarkWidth;
            config.top = targetHeight_.load() - configTemp.top - configTemp.watermarkHeight;
            x = static_cast<float>(config.left) * 2.0f / width - 1.0f;
            y = -1.0f + static_cast<float>(config.top) * 2.0f / height;
            w = static_cast<float>(config.watermarkWidth) * 2.0f / width;
            h = static_cast<float>(config.watermarkHeight) * 2.0f / height;
            wmVertices[0] = x; wmVertices[1] = y + h; wmVertices[2] = 1.0f; wmVertices[3] = 0.0f;  // 左上
            wmVertices[4] = x; wmVertices[5] = y;     wmVertices[6] = 1.0f; wmVertices[7] = 1.0f;  // 左下
            wmVertices[8] = x + w; wmVertices[9] = y; wmVertices[10] = 0.0f; wmVertices[11] = 1.0f; // 右下
            wmVertices[12] = x + w; wmVertices[13] = y + h; wmVertices[14] = 0.0f; wmVertices[15] = 0.0f; // 右上
            break;
        default:
            x = static_cast<float>(config.left) * 2.0f / width - 1.0f;
            y = -1.0f + static_cast<float>(config.top) * 2.0f / height;
            w = static_cast<float>(config.watermarkWidth) * 2.0f / width;
            h = static_cast<float>(config.watermarkHeight) * 2.0f / height;
            wmVertices[0] = x; wmVertices[1] = y + h; wmVertices[2] = 0.0f; wmVertices[3] = 1.0f;  // 左上
            wmVertices[4] = x; wmVertices[5] = y;     wmVertices[6] = 0.0f; wmVertices[7] = 0.0f;  // 左下
            wmVertices[8] = x + w; wmVertices[9] = y; wmVertices[10] = 1.0f; wmVertices[11] = 0.0f; // 右下
            wmVertices[12] = x + w; wmVertices[13] = y + h; wmVertices[14] = 1.0f; wmVertices[15] = 1.0f; // 右上
            break;
    }
}

bool WaterMarkFilter::RenderSingleWatermarkToFBO(const WaterMarkConfig& config,
                                                  GLuint texId,
                                                  int32_t width,
                                                  int32_t height)
{
    // 计算旋转后的顶点坐标和纹理坐标
    float wmVertices[16];
    CalculateRotatedWatermarkVertices(config, width, height, wmVertices, sizeof(wmVertices));

    // 设置顶点数据
    glBufferData(GL_ARRAY_BUFFER, sizeof(wmVertices), wmVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 绑定纹理并绘制
    glBindTexture(GL_TEXTURE_2D, texId);
    watermarkMergeShaderProgram_->SetInt("watermarkTexture", 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    GLenum error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false,
                       "RenderSingleWatermarkToFBO failed with GL error: %{public}d", error);
    return true;
}

bool WaterMarkFilter::RenderWatermarksToFBO(int32_t width, int32_t height)
{
    // 安全检查：防止除零
    FALSE_RETURN_V_MSG(width > 0 && height > 0, false,
                       "Invalid FBO dimensions: %{public}dx%{public}d", width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    watermarkMergeShaderProgram_->Use();
    glActiveTexture(GL_TEXTURE0);

    for (size_t i = 0; i < watermarkList_.size(); i++) {
        const auto& config = watermarkList_[i];
        FALSE_RETURN_V_MSG(RenderSingleWatermarkToFBO(config, watermarkTexIds_[i], width, height),
                           false, "Failed to render watermark %{public}zu", i);
    }

    // 恢复OpenGL状态
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLenum error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false,
                       "RenderWatermarksToFBO failed with GL error: %{public}d", error);
    return true;
}

bool WaterMarkFilter::MergeWaterMarksWithOpenGL()
{
    FALSE_RETURN_V_MSG(!watermarkList_.empty(), false, "watermarkList is empty, skip MergeWaterMarksWithOpenGL");

    // 计算目标尺寸
    int32_t mergedWidth = std::max(targetWidth_.load(), 1);
    int32_t mergedHeight = std::max(targetHeight_.load(), 1);

    FALSE_RETURN_V_MSG(CreateWatermarkFBO(mergedWidth, mergedHeight), false,
                       "Failed to create watermark FBO");

    FALSE_RETURN_V_MSG(CreateWatermarkMergeShader(), false,
                       "Failed to create watermark merge shader");

    FALSE_RETURN_V_MSG(RenderWatermarksToFBO(mergedWidth, mergedHeight), false,
                       "Failed to render watermarks to FBO");

    MEDIA_LOG_I("MergeWaterMarksWithOpenGL success, %{public}zu watermarks on target canvas: %{public}dx%{public}d",
        watermarkList_.size(), mergedWidth, mergedHeight);
    return true;
}

bool WaterMarkFilter::CreateGLResources()
{
    // 根据是否有水印选择不同的着色器
    if (watermarkList_.empty()) {
        shaderProgram_ = std::make_unique<ShaderProgram>(vertexShader, fragmentShaderResize);
        MEDIA_LOG_I("CreateGLResources: using resize-only shader (no watermark)");
    } else {
        shaderProgram_ = std::make_unique<ShaderProgram>(vertexShader, fragmentShader);
        MEDIA_LOG_I("CreateGLResources: using watermark shader");
    }
    FALSE_RETURN_V_MSG(shaderProgram_->Valid(), false, "shaderProgram_->Valid()");

    glGenVertexArrays(1, &vertexArrayObject_);
    GLenum error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glGenVertexArrays failed with GL error: %{public}d", error);

    glGenBuffers(1, &vertexBufferObject_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glGenBuffers failed with GL error: %{public}d", error);

    glBindVertexArray(vertexArrayObject_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBindVertexArray failed with GL error: %{public}d", error);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject_);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBindBuffer failed with GL error: %{public}d", error);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "glBufferData failed with GL error: %{public}d", error);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    // 启用纹理坐标属性
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    error = glGetError();
    FALSE_RETURN_V_MSG(error == GL_NO_ERROR, false, "CreateGLResources failed with GL error: %{public}d", error);
    return true;
}

void WaterMarkFilter::OnNativeImageFrameAvailable(void *data)
{
    MEDIA_LOG_I("OnNativeImageFrameAvailable");
    auto renderThread = reinterpret_cast<WaterMarkFilter *>(data);
    FALSE_RETURN_MSG(renderThread != nullptr, "renderThread is nullptr");
    std::unique_lock<std::mutex> lock(renderThread->wakeUpMutex_);
    renderThread->availableFrameCnt_.fetch_add(1, std::memory_order_relaxed);
    renderThread->wakeUpCond_.notify_one();
}

Status WaterMarkFilter::SetWatermark(std::shared_ptr<AVBuffer> &waterMarkBuffer, int32_t width, int32_t height)
{
    std::unique_lock<std::mutex> lock(releaseMutex_);
    FALSE_RETURN_V_MSG(watermarkList_.size() < MAX_WATERMARK_NUMBER, 
        Status::ERROR_WATERMARK_NUM_OUT_OF_RANGE, "watermarkList_ has out");
    int32_t hileft = 0;
    int32_t hitop = 0;
    int32_t hiwidth = 0; 
    int32_t hiheight = 0;
    int32_t histride = 0;
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_X>(hileft);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_Y>(hitop);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_W>(hiwidth);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_COORDINATE_H>(hiheight);
    waterMarkBuffer->meta_->Get<Tag::VIDEO_STRIDE>(histride);
    // 检查乘法溢出
    FALSE_RETURN_V_MSG(histride > 0 && hiheight > 0, Status::ERROR_INVALID_PARAMETER,
        "Invalid watermark dimensions");
    FALSE_RETURN_V_MSG(static_cast<uint64_t>(hiheight) <= MAX_WATERMARK_SIZE / static_cast<uint64_t>(histride),
        Status::ERROR_INVALID_PARAMETER, "Watermark size exceeds maximum limit");
    int64_t bufferSize = static_cast<int64_t>(hiheight) * histride;
    FALSE_RETURN_V_MSG(static_cast<uint64_t>(bufferSize) < MAX_WATERMARK_SIZE && bufferSize > 0,
        Status::ERROR_INVALID_PARAMETER, "Failed to allocate memory for image data");
    FALSE_RETURN_V_MSG(waterMarkBuffer != nullptr && waterMarkBuffer->memory_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "waterMarkBuffer or memory_ is null");
    auto dataBuffer = new uint8_t[bufferSize];
    waterMarkBuffer->memory_->Read(dataBuffer, bufferSize, 0);
    MEDIA_LOG_I("WaterMarkFilter::SetWatermark hileft: %{public}d hitop:" 
       "%{public}d hiwidth: %{public}d hiheight: %{public}d width: %{public}d height: %{public}d histride: %{public}d",
        hileft, hitop, hiwidth, hiheight, width, height, histride);
    WaterMarkConfig config;
    config.top = hitop;
    config.left = hileft;
    config.rawHeight = hiheight;
    config.rawWidth = hiwidth;
    config.size = bufferSize;
    config.buffer = dataBuffer;
    // 根据旋转角度设置目标显示尺寸
    config.watermarkWidth = (width > 0) ? width : hiwidth;
    config.watermarkHeight = (height > 0) ? height : hiheight;
    watermarkList_.emplace_back(config);
    return Status::OK;
}

Status WaterMarkFilter::SetVideoResize(int32_t width, int32_t height)
{
    FALSE_RETURN_V_MSG(width > 0 && height > 0, Status::ERROR_INVALID_PARAMETER, "Invalid resize dimensions");
    videoWidth_.store(width);
    videoHeight_.store(height);
    targetHeight_.store(height);
    targetWidth_.store(width);
    MEDIA_LOG_I("WaterMarkFilter::SetVideoResize videoWidth_: %{public}d videoHeight_: %{public}d", 
        videoWidth_.load(), videoHeight_.load());
    InitOpenGl();
    return Status::OK;
}

void WaterMarkFilter::InitOpenGl()
{
    FALSE_RETURN_MSG(nativeImage_ == nullptr, "CreateNativeImage success");
    FALSE_RETURN_MSG(CreateNativeImage(), "Failed to create native image");
}

WaterMarkFilter::WaterMarkFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    // 初始化为单位矩阵
    int matrixStep = 4;
    for (int i = 0; i < matrixStep; ++i) {
        matrix_[i * matrixStep + i] = 1.0f;
    }
    MEDIA_LOG_I("water mark filter create");
}

WaterMarkFilter::~WaterMarkFilter()
{
    MEDIA_LOG_I("water mark filter destroy");
    DoRelease();
}

Status WaterMarkFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    return Status::OK;
}

void WaterMarkFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status WaterMarkFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    return Status::OK;
}

sptr<Surface> WaterMarkFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
    FALSE_RETURN_V_MSG(nativeImageWindow_ != nullptr, nullptr, "nativeImageWindow_ is nullptr");
    return nativeImageWindow_->surface;
}

Status WaterMarkFilter::SetOutputSurface(sptr<Surface> surface, int32_t width, int32_t height, int32_t rotation)
{
    std::unique_lock<std::mutex> lock(releaseMutex_);
    MEDIA_LOG_I("WaterMarkFilter SetOutputSurface %{public}d, %{public}d", width, height);
    targetWidth_.store(width);
    targetHeight_.store(height);
    rotation_.store(rotation);
    FALSE_RETURN_V_MSG(surface != nullptr, Status::ERROR_UNKNOWN, "surface is null");
    auto window = CreateNativeWindowFromSurface(&surface);
    UpdateNativeWindow(window, width, height);

    // 只在第一次调用时创建线程，避免重复创建
    FALSE_RETURN_V_MSG(!thread_.joinable(), Status::OK, "thread_ is run");
    thread_ = std::thread([this]() {
        ThreadMainLoop();
    });
    return Status::OK;
}

Status WaterMarkFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    FALSE_RETURN_V_MSG(filterCallback_ != nullptr, Status::ERROR_UNKNOWN, "filterCallback_ is nullptr");
    switch (filterType_) {
        case FilterType::WATERMARK:
            if (isAVRecorderMode_.load()) {
                filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                    StreamType::STREAMTYPE_WATERMARK);
            } else {
                return filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                    StreamType::STREAMTYPE_RAW_VIDEO);
            }
            break;
        default:
            break;
    }
    return Status::OK;
}

Status WaterMarkFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    return Status::OK;
}

Status WaterMarkFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status WaterMarkFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status WaterMarkFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    return Status::OK;
}

Status WaterMarkFilter::DoFlush()
{
    return Status::OK;
}

Status WaterMarkFilter::DoRelease()
{
    MEDIA_LOG_I("DoRelease enter");
    std::unique_lock<std::mutex> lock(releaseMutex_);
    // 使用标志位防止重复释放
    if (hasReleased_) {
        MEDIA_LOG_W("DoRelease already called, skip");
        return Status::OK;
    }
    hasReleased_ = true;
    isAVRecorderMode_.store(false);

    // 停止渲染线程
    {
        running_ = false;
    }
    wakeUpCond_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }
    ReleaseCpuResources();

    // 释放Native Image
    if (nativeImage_ != nullptr) {
        OH_NativeImage_UnsetOnFrameAvailableListener(nativeImage_);
        OH_NativeImage_Destroy(&nativeImage_);
        nativeImage_ = nullptr;
    }
    nativeImageWindow_ = nullptr;

    // 释放GPU侧资源
    ReleaseGpuResources();

    shaderProgram_.reset();
    watermarkMergeShaderProgram_.reset();
    ReleaseEGLResources();
    nativeWindow_ = nullptr;

    MEDIA_LOG_I("DoRelease success");
    return Status::OK;
}

void WaterMarkFilter::ReleaseCpuResources()
{
    // 释放水印列表中每个配置的buffer内存
    for (auto& config : watermarkList_) {
        if (config.buffer != nullptr) {
            delete[] config.buffer;
            config.buffer = nullptr;
        }
    }

    watermarkList_.clear();
}

void WaterMarkFilter::ReleaseGpuResources()
{
    if (vertexArrayObject_ != 0) {
        glDeleteVertexArrays(1, &vertexArrayObject_);
        vertexArrayObject_ = 0;
    }

    if (vertexBufferObject_ != 0) {
        glDeleteBuffers(1, &vertexBufferObject_);
        vertexBufferObject_ = 0;
    }

    if (nativeImageTexId_ != 0 && nativeImageTexId_ != DEFINED_VALUE) {
        glDeleteTextures(1, &nativeImageTexId_);
        nativeImageTexId_ = 0;
    }

    if (textTexId_ != 0 && textTexId_ != DEFINED_VALUE) {
        glDeleteTextures(1, &textTexId_);
        textTexId_ = 0;
    }

    if (mergedWatermarkTexId_ != 0) {
        glDeleteTextures(1, &mergedWatermarkTexId_);
        mergedWatermarkTexId_ = 0;
    }

    if (!watermarkTexIds_.empty()) {
        glDeleteTextures(watermarkTexIds_.size(), watermarkTexIds_.data());
        watermarkTexIds_.clear();
    }

    if (watermarkFBO_ != 0) {
        glDeleteFramebuffers(1, &watermarkFBO_);
        watermarkFBO_ = 0;
    }

    if (watermarkVAO_ != 0) {
        glDeleteVertexArrays(1, &watermarkVAO_);
        watermarkVAO_ = 0;
    }

    if (watermarkVBO_ != 0) {
        glDeleteBuffers(1, &watermarkVBO_);
        watermarkVBO_ = 0;
    }
}

void WaterMarkFilter::ReleaseEGLResources()
{
    if (renderContext_ != nullptr && eglSurface_ != EGL_NO_SURFACE) {
        renderContext_->DestroyEglSurface(eglSurface_);
        eglSurface_ = EGL_NO_SURFACE;
    }
    renderContext_.reset();
}

Status WaterMarkFilter::NotifyNextFilterEos()
{
    MEDIA_LOG_I("NotifyNextFilterEos");
    for (const auto &iter : nextFiltersMap_) {
        for (const auto &filter : iter.second) {
            std::shared_ptr<Meta> eosMeta = std::make_shared<Meta>();
            eosMeta->Set<Tag::MEDIA_END_OF_STREAM>(true);
            eosMeta->Set<Tag::USER_FRAME_PTS>(eosPts_.load());
            FALSE_RETURN_V_MSG(filter != nullptr, Status::ERROR_UNKNOWN, "filter is nullptr");
            filter->SetParameter(eosMeta);
        }
    }
    return Status::OK;
}

void WaterMarkFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    std::lock_guard<std::mutex> lock(wakeUpMutex_);
    FALSE_RETURN_MSG(parameter != nullptr, "parameter is nullptr");
    int64_t pts = 0;
    parameter->Get<Tag::USER_FRAME_PTS>(pts);
    eosPts_.store(pts);    
    parameter->Get<Tag::USER_PUSH_DATA_TIME>(frameNum_);
    MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64 " frameNum: " PUBLIC_LOG_D64, eosPts_.load(), frameNum_);
    wakeUpCond_.notify_one();
}

void WaterMarkFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status WaterMarkFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    FALSE_RETURN_V_MSG(nextFilter != nullptr, Status::ERROR_UNKNOWN, "nextFilter");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<WaterMarkFilterLinkCallback>(shared_from_this());
    auto ret = nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    if (ret != Status::OK && eventReceiver_) {
        eventReceiver_->OnEvent({"WaterMarkFilter::LinkNext error", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_UNKNOWN)});
    }
    return ret;
}

Status WaterMarkFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status WaterMarkFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

Status WaterMarkFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status WaterMarkFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status WaterMarkFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void WaterMarkFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult enter");
    if (onLinkedResultCallback_) {
        onLinkedResultCallback_->OnLinkedResult(nullptr, meta);
    }
}

void WaterMarkFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
    (void) meta;
}

void WaterMarkFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
    (void) meta;
}

void WaterMarkFilter::OnOutputBufferAvailable(uint32_t index, uint32_t flag)
{
    MEDIA_LOG_D("OnOutputBufferAvailable enter. index: " PUBLIC_LOG_U32, index);
}

void WaterMarkFilter::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    MEDIA_LOG_I("SetFaultEvent");
}

void WaterMarkFilter::SetFaultEvent(const std::string &errMsg)
{
    MEDIA_LOG_I("SetFaultEvent");
}

void WaterMarkFilter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;   
}

FilterType WaterMarkFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
}

void WaterMarkFilter::SetAVRecorderMode()
{
    MEDIA_LOG_I("SetAVRecorderMode");
    isAVRecorderMode_.store(true);
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS