/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "alpha_video_post_processor.h"

#include "buffer/avbuffer.h"
#include "common/log.h"
#include "common/media_core.h"
#include "meta/format.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "AlphaVideoPostProcessor" };

// 顶点着色器: 全屏 quad, texCoord∈[0,1]² 覆盖整屏归一化坐标; matTransform FlipV(仅翻 Y)修正 OES 上下颠倒
// 颜色半/alpha 半的采样由片段着色器按 uCompositeMode 在 vTexCoord 上重映射, 故顶点数据与旋转阵对各模式通用
const std::string ALPHA_VERTEX_SHADER = R"delimiter(
attribute vec3 position;
attribute vec2 texCoord;
varying vec2 vTexCoord;
uniform mat4 matTransform;
void main()
{
    gl_Position = matTransform * vec4(position, 1.0);
    vTexCoord = texCoord;            // 全屏归一化坐标, 颜色/alpha 半由片段着色器按模式取
}
)delimiter";

// 片段着色器: 按 uCompositeMode 在 vTexCoord 上重映射颜色半/alpha 半采样坐标,
// mask 亮度(BT.601)经 S 曲线归一化后作为 alpha, 输出乘 alpha 的预乘结果(配合 surface SetAlphaType PREMUL + compositor SRCOVER)
// 模式(对齐公开接口定义): 0=左彩右透明 1=上彩下透明 2=左透明右彩 3=上透明下彩
const std::string ALPHA_FRAGMENT_SHADER = R"delimiter(
#extension GL_OES_EGL_image_external : require
precision highp float;
varying vec2 vTexCoord;
uniform samplerExternalOES texture;
uniform int uCompositeMode;
void main()
{
    vec2 colorCoord, alphaCoord;
    if (uCompositeMode == 0) {                 // 左彩右透明(接口 0)
        colorCoord = vTexCoord * vec2(0.5, 1.0);
        alphaCoord = vTexCoord * vec2(0.5, 1.0) + vec2(0.5, 0.0);
    } else if (uCompositeMode == 1) {           // 上彩下透明(接口 1)
        colorCoord = vTexCoord * vec2(1.0, 0.5);
        alphaCoord = vTexCoord * vec2(1.0, 0.5) + vec2(0.0, 0.5);
    } else if (uCompositeMode == 2) {           // 左透明右彩(接口 2)
        colorCoord = vTexCoord * vec2(0.5, 1.0) + vec2(0.5, 0.0);
        alphaCoord = vTexCoord * vec2(0.5, 1.0);
    } else {                                    // 上透明下彩(接口 3)
        colorCoord = vTexCoord * vec2(1.0, 0.5) + vec2(0.0, 0.5);
        alphaCoord = vTexCoord * vec2(1.0, 0.5);
    }
    vec4 color = texture2D(texture, colorCoord);                            // 颜色半
    vec4 aColor = texture2D(texture, alphaCoord);                           // alpha 遮罩半
    float alpha = 0.299 * aColor.r + 0.587 * aColor.g + 0.114 * aColor.b;  // mask 半原始亮度(BT.601)
    // 颜色半存储约定与打包方向联动选择公式(实测两类片源约定恰好如此; 待格式规范统一约定后收敛为单公式):
    // 上下打包(接口奇数值 1/3)片源为直线色(颜色半背景为原图场景):
    //   乘归一化 alpha——不乘则原图背景色被直加(白底层); S 曲线把 mask 主体推满 1、背景压回 0
    // 左右打包(接口偶数值 0/2)——按通道拆分预乘/直线色成分, 兼容两种颜色半存储:
    //   数学不变量: 预乘内容恒满足 rgb<=alpha(编码时乘过覆盖度), 超 alpha 的通道必为直线色杂量。
    //   预乘部分: min(rgb, alpha) 原样保留, 输出 alpha 直用 raw——数学精确, 软覆盖内容(阴影/渐变/
    //     半透明笔画)零削弱(反预乘因子方案会把它削弱 ~30% 即偏黑回归, 此为折中方案的根源)
    //   直线色杂量(亮背景): (rgb - alpha) 部分乘 S 曲线归一化 alpha——透明区归零(白色图层根除),
    //     主体区(a=1)全保留
    if (uCompositeMode == 0 || uCompositeMode == 2) {
        vec3 premulPart = min(color.rgb, vec3(alpha));
        vec3 excess = max(color.rgb - vec3(alpha), vec3(0.0));
        float t = (alpha - 0.1) / 0.7;
        if (t < 0.0) {
            t = 0.0;
        }
        if (t > 1.0) {
            t = 1.0;
        }
        float a = t * t * (3.0 - 2.0 * t); // 手写 S 曲线, 与 smoothstep(0.1, 0.8) 数学等价
        gl_FragColor = vec4(premulPart + excess * a, alpha);
    } else {
        float t = (alpha - 0.1) / 0.7;
        if (t < 0.0) {
            t = 0.0;
        }
        if (t > 1.0) {
            t = 1.0;
        }
        float a = t * t * (3.0 - 2.0 * t); // 手写 S 曲线, 与 smoothstep(0.1, 0.8) 数学等价
        gl_FragColor = vec4(color.rgb * a, a);
    }
}
)delimiter";

// 不透明直出着色器: SetPostProcessorOn(false) 时走此分支, 按 uCompositeMode 取颜色半, alpha 强制为 1(整帧不透明, 背景不透出)
// 模式(对齐公开接口定义): 0=左彩右透明 1=上彩下透明 2=左透明右彩 3=上透明下彩
const std::string ALPHA_OPAQUE_FRAGMENT_SHADER = R"delimiter(
#extension GL_OES_EGL_image_external : require
precision highp float;
varying vec2 vTexCoord;
uniform samplerExternalOES texture;
uniform int uCompositeMode;
void main()
{
    vec2 colorCoord;
    if (uCompositeMode == 0) {                 // 左彩右透明(接口 0)
        colorCoord = vTexCoord * vec2(0.5, 1.0);
    } else if (uCompositeMode == 1) {           // 上彩下透明(接口 1)
        colorCoord = vTexCoord * vec2(1.0, 0.5);
    } else if (uCompositeMode == 2) {           // 左透明右彩(接口 2)
        colorCoord = vTexCoord * vec2(0.5, 1.0) + vec2(0.5, 0.0);
    } else {                                    // 上透明下彩(接口 3)
        colorCoord = vTexCoord * vec2(1.0, 0.5) + vec2(0.0, 0.5);
    }
    vec4 color = texture2D(texture, colorCoord);                            // 当前模式颜色半
    gl_FragColor = vec4(color.rgb, 1.0);                                     // 不透明直出: alpha 强制为 1
}
)delimiter";

// 全屏 quad: position(x,y,z) + texCoord(u,v), texCoord∈[0,1]² 覆盖整屏归一化坐标;
// 颜色半/alpha 半由片段着色器按模式在 vTexCoord 上重映射, 故四顶点 texCoord 取满 [0,1]²
// 顶点属性 location 与顶点着色器 attribute 声明顺序对应: position=0, texCoord=1
constexpr GLuint ALPHA_ATTR_POSITION = 0;
constexpr GLuint ALPHA_ATTR_TEXCOORD = 1;
constexpr GLint ALPHA_POSITION_SIZE = 3;  // vec3(x, y, z)
constexpr GLint ALPHA_TEXCOORD_SIZE = 2;  // vec2(u, v)
// 每顶点紧凑排布 5 个 float, texCoord 起始于第 3 个 float(position 之后)
constexpr GLsizei ALPHA_VERTEX_STRIDE = (ALPHA_POSITION_SIZE + ALPHA_TEXCOORD_SIZE) * sizeof(float);
constexpr GLsizei ALPHA_TEXCOORD_OFFSET = ALPHA_POSITION_SIZE * sizeof(float);
const GLfloat ALPHA_VERTICES[] = {
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
    1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
};
const GLuint ALPHA_INDICES[] = { 0, 1, 2, 0, 2, 3 };
constexpr GLsizei ALPHA_INDEX_COUNT = sizeof(ALPHA_INDICES) / sizeof(ALPHA_INDICES[0]);
// matTransform 为 4x4 列主序矩阵, 元素共 16 个
constexpr int ALPHA_MAT4_ELEMENT_COUNT = 4 * 4;

} // namespace

namespace OHOS {
namespace Media {
namespace {
bool IsAlphaVideoSupported(const std::shared_ptr<Meta> &meta)
{
    FALSE_RETURN_V(meta != nullptr, false);
    int32_t width = 0;
    int32_t height = 0;
    bool isDrmProtected = false;
    meta->GetData(Tag::VIDEO_WIDTH, width);
    meta->GetData(Tag::VIDEO_HEIGHT, height);
    meta->GetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, isDrmProtected);
    // alpha 视频帧宽为双倍(颜色+alpha 并排), 仅要求尺寸合法且非 DRM
    bool sizeValid = (width > 0 && height > 0);
    if (!sizeValid || isDrmProtected) {
        MEDIA_LOG_E("invalid input for alpha video! w:%{public}d h:%{public}d drm:%{public}d",
            width, height, isDrmProtected);
    }
    return sizeValid && !isDrmProtected;
}

// 静态自动注册: 工厂单例在进程启动时收集, 无需改工厂代码
AutoRegisterPostProcessor<AlphaVideoPostProcessor> g_registerAlphaVideoPostProcessor(
    VideoPostProcessorType::ALPHA_VIDEO,
    []() -> std::shared_ptr<BaseVideoPostProcessor> {
        return std::make_shared<AlphaVideoPostProcessor>();
    },
    &IsAlphaVideoSupported);
} // namespace

AlphaVideoPostProcessor::AlphaVideoPostProcessor()
{
    MEDIA_LOG_I("AlphaVideoPostProcessor create");
}

AlphaVideoPostProcessor::~AlphaVideoPostProcessor()
{
    Release();
}

sptr<Surface> AlphaVideoPostProcessor::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface in");
    if (nativeImage_ == nullptr) {
        FALSE_RETURN_V_MSG(CreateNativeImage(), nullptr, "CreateNativeImage failed");
    }
    FALSE_RETURN_V_MSG(nativeImageWindow_ != nullptr, nullptr, "nativeImageWindow_ is nullptr");
    return nativeImageWindow_->surface; // OHNativeWindow.surface 即解码器写入的消费面(零拷贝)
}

Status AlphaVideoPostProcessor::SetOutputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SetOutputSurface in");
    FALSE_RETURN_V_MSG(surface != nullptr, Status::ERROR_INVALID_PARAMETER, "output surface is null");
    UpdateOutputWindow(surface);
    return Status::OK;
}

Status AlphaVideoPostProcessor::Init()
{
    MEDIA_LOG_I("Init in");
    if (thread_.joinable()) {
        return Status::OK; // 已起线程
    }
    running_ = true;   // 线程存活标志, 仅 Release 清零退出循环
    started_ = false;  // 绘制开关, 由 Start/Pause/Stop 切换
    thread_ = std::thread([this]() { RenderLoop(); });
    return Status::OK;
}

Status AlphaVideoPostProcessor::Start()
{
    MEDIA_LOG_I("Start in");
    started_ = true;
    wakeCond_.notify_all();
    return Status::OK;
}

Status AlphaVideoPostProcessor::Pause()
{
    MEDIA_LOG_I("Pause in");
    started_ = false;
    wakeCond_.notify_all();
    return Status::OK;
}

Status AlphaVideoPostProcessor::Stop()
{
    MEDIA_LOG_I("Stop in");
    started_ = false; // 停止绘制, 但线程与 EGL/GL 资源保留以备重启
    eosPending_ = false; // 停止时清掉待回传 EOS, 防重启后误触发
    wakeCond_.notify_all();
    return Status::OK;
}

Status AlphaVideoPostProcessor::Flush()
{
    MEDIA_LOG_I("Flush in");
    {
        std::lock_guard<std::mutex> lock(wakeMutex_);
        // 修复 seek 卡死: 不能清零 availableFrameCnt_ —— NativeImage 队列中已入队的帧仍占着 buffer 位
        // (解码器 flush 只清自己一侧, 收不回已入队帧)。若把信号清零, seek 后将互等死锁:
        // 解码器等队列空位 <-> 渲染线程等新帧信号(计数恒 0), 播放卡死(必现)。
        // 改为置 flushDraining_, 由渲染线程把残留帧逐个消费掉(不上报不绘制), 腾出空位给解码器。
        frameState_ = static_cast<int>(FrameState::IDLE);
        renderRequested_ = false;
        renderTimeNs_ = -1;
        eosPending_ = false; // 切源/重播清掉待回传 EOS, 防误触发
        flushDraining_.store(true);
        flushEpoch_.fetch_add(1, std::memory_order_acq_rel); // 与 ConsumeAndReport 的消费中上报竞态闸门
    }
    wakeCond_.notify_all();
    return Status::OK;
}

Status AlphaVideoPostProcessor::Release()
{
    MEDIA_LOG_I("Release in");
    if (hasReleased_.exchange(true)) {
        return Status::OK;
    }
    started_ = false;
    running_ = false;
    wakeCond_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
    ReleaseGpuResources();
    ReleaseEglResources();
    DestroyNativeImage();
    outputWindow_ = nullptr;
    outputSurface_ = nullptr;
    return Status::OK;
}

Status AlphaVideoPostProcessor::NotifyEos(int64_t eosPts)
{
    MEDIA_LOG_I("NotifyEos in, eosPts:%{public}" PRId64, eosPts);
    {
        std::lock_guard<std::mutex> lock(wakeMutex_);
        eosPts_ = eosPts;
        eosPending_ = true;          // 标记有 EOS 待回传, RenderLoop drain 完最后一帧后据此回传 EOS buffer
    }
    wakeCond_.notify_all();
    return Status::OK;
}

Status AlphaVideoPostProcessor::ReleaseOutputBuffer(uint32_t index, bool render)
{
    MEDIA_LOG_D("ReleaseOutputBuffer idx:%{public}u render:%{public}u", index, render);
    {
        std::lock_guard<std::mutex> lock(wakeMutex_);
        if (render) {
            renderTimeNs_ = -1; // 无指定时间, 立即上屏
            renderRequested_ = true;
        } else {
            // 丢弃当前持有帧, 不上屏
            renderRequested_ = false;
            frameState_ = static_cast<int>(FrameState::IDLE);
        }
    }
    wakeCond_.notify_one();
    return Status::OK;
}

Status AlphaVideoPostProcessor::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    MEDIA_LOG_D("RenderOutputBufferAtTime idx:%{public}u ts:%{public}" PRId64, index, renderTimestampNs);
    {
        std::lock_guard<std::mutex> lock(wakeMutex_);
        renderTimeNs_ = renderTimestampNs;
        renderRequested_ = true;
    }
    wakeCond_.notify_one();
    return Status::OK;
}

Status AlphaVideoPostProcessor::SetCallback(const std::shared_ptr<PostProcessorCallback> callback)
{
    filterCallback_ = callback;
    return Status::OK;
}

Status AlphaVideoPostProcessor::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    eventReceiver_ = receiver;
    return Status::OK;
}

Status AlphaVideoPostProcessor::SetParameter(const Format &format)
{
    // 记录解码帧(打包帧)宽高: configFormat_ 携带, 供首帧自适应输出 buffer 几何使用
    // (日志实测 GET_BUFFER_GEOMETRY 查不到——解码器未在 NativeImage 窗口上设几何, 以此为主来源)
    int32_t w = 0;
    int32_t h = 0;
    if (format.GetIntValue(Tag::VIDEO_WIDTH, w) && format.GetIntValue(Tag::VIDEO_HEIGHT, h)) {
        frameWidth_.store(w);
        frameHeight_.store(h);
        MEDIA_LOG_I("SetParameter frame %{public}dx%{public}d", w, h);
        // 起播时延优化: 几何自适应提前到 prepare 阶段执行(InitPostProcessor 在 Init 前调用本接口,
        // 且 SetVideoWindowSize 已先行把 1920x1080 默认值下发生效), 输出 buffer 按颜色半尺寸提前完成
        // 分配, 避免把 SET_BUFFER_GEOMETRY 触发的重分配压到首帧关键路径(原实现挂在首帧消费时)
        if (!geometryAdjusted_) {
            AdjustOutputGeometryToFrame();
        }
    }
    return Status::OK;
}

Status AlphaVideoPostProcessor::SetPostProcessorOn(bool isPostProcessorOn)
{
    MEDIA_LOG_I("SetPostProcessorOn:%{public}d", isPostProcessorOn);
    isPostProcessorOn_ = isPostProcessorOn;
    return Status::OK;
}

Status AlphaVideoPostProcessor::SetTransparentVideoLayoutMode(int32_t mode)
{
    MEDIA_LOG_I("AlphaVideoPostProcessor SetTransparentVideoLayoutMode in, mode: %{public}d", mode);
    FALSE_RETURN_V_MSG(mode >= static_cast<int32_t>(AlphaVideoCompositeMode::ALPHA_VIDEO_LEFT_COLOR_RIGHT_ALPHA) &&
        mode <= static_cast<int32_t>(AlphaVideoCompositeMode::ALPHA_VIDEO_TOP_ALPHA_BOTTOM_COLOR),
        Status::ERROR_INVALID_PARAMETER, "invalid alpha video mode:%{public}d", mode);
    MEDIA_LOG_I("SetTransparentVideoLayoutMode:%{public}d", mode);
    // GL 调用必须在 render 线程: 此处仅写入模式 + 置 dirty, 由 DrawImage 在 render 线程上 uniform
    compositeMode_.store(mode, std::memory_order_relaxed);
    modeDirty_.store(true, std::memory_order_release);
    wakeCond_.notify_one(); // 唤醒 render 线程, 下次 DrawImage 检测 dirty 并 glUniform
    return Status::OK;
}

Status AlphaVideoPostProcessor::SetVideoWindowSize(int32_t width, int32_t height)
{
    MEDIA_LOG_I("SetVideoWindowSize w:%{public}d h:%{public}d", width, height);
    viewWidth_ = width;
    viewHeight_ = height;
    if (outputWindow_ != nullptr && width > 0 && height > 0) {
        OH_NativeWindow_NativeWindowHandleOpt(outputWindow_, SET_BUFFER_GEOMETRY, width, height);
    }
    return Status::OK;
}

bool AlphaVideoPostProcessor::CreateNativeImage()
{
    nativeImage_ = OH_NativeImage_Create(-1, GL_TEXTURE_EXTERNAL_OES);
    FALSE_RETURN_V_MSG(nativeImage_ != nullptr, false, "OH_NativeImage_Create failed");
    nativeImageWindow_ = OH_NativeImage_AcquireNativeWindow(nativeImage_);
    FALSE_RETURN_V_MSG(nativeImageWindow_ != nullptr, false, "AcquireNativeWindow failed");
    frameAvailableListener_.context = this;
    frameAvailableListener_.onFrameAvailable = &AlphaVideoPostProcessor::OnNativeImageFrameAvailable;
    int32_t ret = OH_NativeImage_SetOnFrameAvailableListener(nativeImage_, frameAvailableListener_);
    FALSE_RETURN_V_MSG(ret == 0, false, "SetOnFrameAvailableListener failed ret:%{public}d", ret);
    return true;
}

void AlphaVideoPostProcessor::DestroyNativeImage()
{
    if (nativeImage_ != nullptr) {
        (void)OH_NativeImage_UnsetOnFrameAvailableListener(nativeImage_);
        if (nativeImageTexId_ != 0) {
            // 纹理释放由 ReleaseGpuResources 处理, 这里仅解绑
            (void)OH_NativeImage_DetachContext(nativeImage_);
        }
        OH_NativeImage_Destroy(&nativeImage_);
        nativeImage_ = nullptr;
    }
    nativeImageWindow_ = nullptr;
}

void AlphaVideoPostProcessor::UpdateOutputWindow(sptr<Surface> surface)
{
    outputSurface_ = surface;
    auto window = CreateNativeWindowFromSurface(&surface);
    outputWindow_ = window;
    FALSE_RETURN_MSG(outputWindow_ != nullptr, "CreateNativeWindowFromSurface failed");
    if (viewWidth_ > 0 && viewHeight_ > 0) {
        OH_NativeWindow_NativeWindowHandleOpt(outputWindow_, SET_BUFFER_GEOMETRY, viewWidth_, viewHeight_);
    }
    // 透明视频需要输出 surface 的 buffer 带 alpha 通道, 否则 compositor 会丢弃 alpha, 透明区显示为黑色
    OH_NativeWindow_NativeWindowHandleOpt(outputWindow_, SET_FORMAT, GRAPHIC_PIXEL_FMT_RGBA_8888);
    // 告知 compositor 本 surface 的 alpha 为预乘格式, 使 RenderService 选用 GRAPHIC_BLEND_SRCOVER 做 alpha 混合,
    // 透明区(alpha=0)透出应用背景而非黑底。compositor 是否混合还受 SurfaceBufferNode::isOpaque_ 控制,
    // 该标志由 app 侧 XComponent 通过 OH_ArkUI_XComponentSurfaceConfig_SetIsOpaque(false) 设置。
    if (outputSurface_ != nullptr) {
        outputSurface_->SetAlphaType(GraphicAlphaType::GRAPHIC_ALPHATYPE_PREMUL);
    }
}

bool AlphaVideoPostProcessor::InitializeEGLContext()
{
    renderContext_ = std::make_unique<EglRenderContext>();
    FALSE_RETURN_V_MSG(renderContext_ != nullptr, false, "create EglRenderContext failed");
    FALSE_RETURN_V_MSG(renderContext_->Init(), false, "EglRenderContext Init failed");
    FALSE_RETURN_V_MSG(outputWindow_ != nullptr, false, "outputWindow_ is nullptr");
    eglSurface_ = renderContext_->CreateEglSurface(static_cast<EGLNativeWindowType>(outputWindow_));
    FALSE_RETURN_V_MSG(eglSurface_ != EGL_NO_SURFACE, false, "CreateEglSurface failed");
    renderContext_->MakeCurrent(eglSurface_);
    return true;
}

bool AlphaVideoPostProcessor::CreateGLResources()
{
    shaderProgram_ = std::make_unique<ShaderProgram>(ALPHA_VERTEX_SHADER, ALPHA_FRAGMENT_SHADER);
    FALSE_RETURN_V_MSG(shaderProgram_ != nullptr && shaderProgram_->Valid(), false, "shader program invalid");

    glGenVertexArrays(1, &vertexArrayObject_);
    glGenBuffers(1, &vertexBufferObject_);
    glBindVertexArray(vertexArrayObject_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ALPHA_VERTICES), ALPHA_VERTICES, GL_STATIC_DRAW);
    glVertexAttribPointer(ALPHA_ATTR_POSITION, ALPHA_POSITION_SIZE, GL_FLOAT, GL_FALSE,
        ALPHA_VERTEX_STRIDE, nullptr);
    glEnableVertexAttribArray(ALPHA_ATTR_POSITION);
    glVertexAttribPointer(ALPHA_ATTR_TEXCOORD, ALPHA_TEXCOORD_SIZE, GL_FLOAT, GL_FALSE,
        ALPHA_VERTEX_STRIDE, reinterpret_cast<void *>(ALPHA_TEXCOORD_OFFSET));
    glEnableVertexAttribArray(ALPHA_ATTR_TEXCOORD);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); // 基线行为: DrawImage 每帧重绑 VAO(功耗回归后回退)
    shaderProgram_->Use();
    shaderProgram_->SetInt("texture", 0);
    // 一次性设置: matrix_ 为 FlipV 阵(仅翻 Y, 修正 OES 纹理上下颠倒, 对齐 demo kMatrixFlipV), 避免每帧重复上传
    shaderProgram_->SetMatrix4v("matTransform", matrix_, ALPHA_MAT4_ELEMENT_COUNT, false);
    // 初始合成模式(默认左彩右透明); 运行时切换由 SetTransparentVideoLayoutMode 置 modeDirty_, DrawImage 上 uniform
    shaderProgram_->SetInt("uCompositeMode", compositeMode_.load(std::memory_order_acquire));
    // 不透明直出着色器(SetPostProcessorOn(false) 时走此分支)改为 DrawImage 懒编译(起播时延优化):
    // 着色器编译+链接是 GL 初始化最贵单项, alpha 主路径用不到, 不在起播关键路径上白付这次编译
    return true;
}

bool AlphaVideoPostProcessor::CreateVideoTexture()
{
    glGenTextures(1, &nativeImageTexId_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 一次性绑定: 后续 OH_NativeImage_UpdateSurfaceImage 直接刷新该 OES 纹理(EGL image, 零拷贝)
    int32_t ret = OH_NativeImage_AttachContext(nativeImage_, nativeImageTexId_);
    FALSE_RETURN_V_MSG(ret == 0, false, "OH_NativeImage_AttachContext failed ret:%{public}d", ret);
    return true;
}
// 核心后处理 消费者监听(自动触发)
void AlphaVideoPostProcessor::OnNativeImageFrameAvailable(void *data)
{
    auto self = reinterpret_cast<AlphaVideoPostProcessor *>(data);
    FALSE_RETURN_MSG(self != nullptr, "frame available data is null");
    std::lock_guard<std::mutex> lock(self->wakeMutex_);
    self->availableFrameCnt_.fetch_add(1, std::memory_order_relaxed);
    self->wakeCond_.notify_one();
}

void AlphaVideoPostProcessor::AdjustOutputGeometryToFrame()
{
    geometryAdjusted_ = true;
    // 首帧自适应输出尺寸(掉帧优化): app 未调 setVideoWindowSize 时(该接口被超分门控, alpha 场景不可用),
    // filter 下发的是 hiplayer 默认 MAX_TARGET 1920x1080, 每帧渲染/搬运/合成 8.3MB RGBA, 远超实际所需;
    // 按解码帧"颜色半"尺寸收缩输出 buffer(只缩不放), RS 合成时再缩放到 XComponent 实际显示区。
    // 注: 运行时切换布局模式后理想输出尺寸会变, 此处不跟随(避免播放中途改 buffer 尺寸), 以首帧+初始模式为准。
    int32_t frameH = 0;
    int32_t frameW = 0;
    if (frameWidth_.load() > 0 && frameHeight_.load() > 0) {
        frameW = frameWidth_.load(); // 主来源: SetParameter 记录的解码帧宽高(configFormat_)
        frameH = frameHeight_.load();
    } else if (nativeImageWindow_ == nullptr ||
        OH_NativeWindow_NativeWindowHandleOpt(nativeImageWindow_, GET_BUFFER_GEOMETRY, &frameH, &frameW) != 0 ||
        frameW <= 0 || frameH <= 0) {
        MEDIA_LOG_W("query input frame geometry failed, keep view %{public}dx%{public}d", viewWidth_, viewHeight_);
        return; // 回退来源: 输入面窗口几何(解码器通常不设, 常失败)
    } else {
        MEDIA_LOG_I("AdjustOutputGeometryToFrame query ok");
    }
    int32_t mode = compositeMode_.load(std::memory_order_relaxed);
    // 打包方向按接口数值奇偶: 偶数(0左彩右透明/2左透明右彩)=左右打包, 宽减半;
    // 奇数(1上彩下透明/3上透明下彩)=上下打包, 高减半
    int32_t outW = (mode % 2 == 0) ? frameW / 2 : frameW;
    int32_t outH = (mode % 2 == 1) ? frameH / 2 : frameH;
    if (outW <= 0 || outH <= 0) {
        return;
    }
    if (viewWidth_ > 0 && viewHeight_ > 0 && outW >= viewWidth_ && outH >= viewHeight_) {
        return; // 已设置的尺寸不大于颜色半(低分辨率源或外部显式指定), 不放大
    }
    MEDIA_LOG_I("adjust output geometry to color-half %{public}dx%{public}d (frame %{public}dx%{public}d, "
        "was %{public}dx%{public}d)", outW, outH, frameW, frameH, viewWidth_, viewHeight_);
    viewWidth_ = outW;
    viewHeight_ = outH;
    if (outputWindow_ != nullptr) {
        OH_NativeWindow_NativeWindowHandleOpt(outputWindow_, SET_BUFFER_GEOMETRY, outW, outH);
    }
}

void AlphaVideoPostProcessor::ConsumeAndReport()
{
    FALSE_RETURN_MSG(renderContext_ != nullptr && eglSurface_ != EGL_NO_SURFACE, "egl not ready");
    uint32_t epoch = flushEpoch_.load(std::memory_order_acquire);
    // 功耗回归后回退: 保持与 700mA 基线完全一致的每帧调用序列(MakeCurrent 虽是冗余 CPU 调用,
    // 但 µs 级收益不值得任何驱动侧行为差异风险)
    renderContext_->MakeCurrent(eglSurface_);
    int32_t ret = OH_NativeImage_UpdateSurfaceImage(nativeImage_); // 取一帧并绑定到 OES 纹理(零拷贝)
    if (ret != 0) {
        MEDIA_LOG_E("UpdateSurfaceImage failed ret:%{public}d", ret);
        availableFrameCnt_.fetch_sub(1, std::memory_order_relaxed);
        frameState_ = static_cast<int>(FrameState::IDLE);
        return;
    }
    if (epoch != flushEpoch_.load(std::memory_order_acquire)) {
        // 竞态收尾: Flush(seek/倍速)恰好发生在本帧消费过程中——本帧按残留帧处理直接丢弃, 不上报。
        // 带旧 pts 的上报会毒化 filter 节奏锚点(SetFirstPts), 使 seek 后渲染被调度到遥远未来, 表现为长时间卡死
        availableFrameCnt_.fetch_sub(1, std::memory_order_relaxed);
        frameState_ = static_cast<int>(FrameState::IDLE);
        return;
    }
    if (!geometryAdjusted_) {
        AdjustOutputGeometryToFrame(); // 首帧时解码器已写入输入面, 几何信息可查
    }
    availableFrameCnt_.fetch_sub(1, std::memory_order_relaxed);
    int64_t pts = OH_NativeImage_GetTimestamp(nativeImage_);
    lastPts_ = pts;
    // 上报一帧(轻量 AVBuffer, 仅承载 pts; 像素仍在 GPU)交 filter 节奏调度
    if (filterCallback_ != nullptr) {
        auto buffer = AVBuffer::CreateAVBuffer();
        if (buffer != nullptr) {
            buffer->pts_ = pts;
            filterCallback_->OnOutputBufferAvailable(outIndex_.fetch_add(1, std::memory_order_relaxed), buffer);
        }
    }
    frameState_ = static_cast<int>(FrameState::HELD); // 纹理持有一帧, 等待节奏触发上屏
}

// seek 排空: 消费 NativeImage 中 Flush 前残留的帧。不上报(filter 此刻已清空缓冲表, 上报会被丢弃),
// 不置 HELD(残留帧直接丢弃, 纹理旧绑定随下次消费自然替换归还)。每轮排空一帧,
// 计数归零后由 RenderLoop 结束排空模式。排空期间新到的帧会被一并消费(seek 后新起播头 1~2 帧, 不可感知)。
void AlphaVideoPostProcessor::FlushConsume()
{
    FALSE_RETURN_MSG(renderContext_ != nullptr && eglSurface_ != EGL_NO_SURFACE, "egl not ready");
    renderContext_->MakeCurrent(eglSurface_);
    int32_t ret = OH_NativeImage_UpdateSurfaceImage(nativeImage_);
    if (ret != 0) {
        // 队列实际已空但计数>0(历史残留计数): 修正计数, 防空转
        MEDIA_LOG_W("FlushConsume UpdateSurfaceImage ret:%{public}d, fix cnt to 0", ret);
        availableFrameCnt_.store(0, std::memory_order_relaxed);
        return;
    }
    availableFrameCnt_.fetch_sub(1, std::memory_order_relaxed);
    frameState_.store(static_cast<int>(FrameState::IDLE));
}

void AlphaVideoPostProcessor::DrawImage()
{
    // 功耗回归后回退至 700mA 基线的每帧 GL 调用序列(视口/纹理/VAO 绑定每帧重设):
    // 微优化省的是 ~µs 级 CPU 调用, 却在 tile-based GPU 上引入不可控行为(实测 +200mA), 得不偿失。
    // 保留的安全项: glClear(见下)、不透明 program 懒编译(起播一次性)。
    glViewport(0, 0, viewWidth_, viewHeight_);
    // 模式切换 dirty: SetTransparentVideoLayoutMode 在 player 线程写入 + 置 dirty, 此处(render 线程)检测并上 uniform。
    if (modeDirty_.exchange(false, std::memory_order_acq_rel)) {
        int32_t m = compositeMode_.load(std::memory_order_acquire);
        shaderProgram_->Use();
        shaderProgram_->SetInt("uCompositeMode", m);
        if (opaqueShaderProgram_ != nullptr) { // 懒编译: 未创建时跳过, 创建时按当前模式初始化
            opaqueShaderProgram_->Use();
            opaqueShaderProgram_->SetInt("uCompositeMode", m);
        }
    }
    // 全屏 quad 覆写整个视口(position x/y 均 [-1,1]), 每像素均被直写;
    // glClear 在 tile-based GPU(移动端主流)上由 tiler 硬件完成、近零带宽, 且声明"整帧作废、
    // 无需回读旧内容"——省去它的收益(一次调用)远小于代价(驱动保守时每 tile 回读旧像素的 DDR 流量,
    // 实测 +200mA), 必须保留。
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // 运行时启停: isPostProcessorOn_ 选 alpha 合成(透明)或当前模式颜色半直出(不透明, alpha=1)。
    if (isPostProcessorOn_.load()) {
        shaderProgram_->Use();
    } else {
        // 起播时延优化: 不透明直出 program 懒编译——着色器编译+链接是 GL 初始化最贵单项,
        // 仅 SetAlphaVideo(false) 路径才用到, alpha 主路径不白付这次编译
        if (opaqueShaderProgram_ == nullptr) {
            opaqueShaderProgram_ = std::make_unique<ShaderProgram>(ALPHA_VERTEX_SHADER, ALPHA_OPAQUE_FRAGMENT_SHADER);
            FALSE_RETURN_MSG(opaqueShaderProgram_ != nullptr && opaqueShaderProgram_->Valid(),
                "opaque program invalid");
            opaqueShaderProgram_->Use();
            opaqueShaderProgram_->SetInt("texture", 0);
            opaqueShaderProgram_->SetMatrix4v("matTransform", matrix_, ALPHA_MAT4_ELEMENT_COUNT, false);
            opaqueShaderProgram_->SetInt("uCompositeMode", compositeMode_.load(std::memory_order_acquire));
        }
        opaqueShaderProgram_->Use();
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeImageTexId_);
    glBindVertexArray(vertexArrayObject_);
    glDrawElements(GL_TRIANGLES, ALPHA_INDEX_COUNT, GL_UNSIGNED_INT, ALPHA_INDICES);
    glBindVertexArray(0);
}

void AlphaVideoPostProcessor::DrawAndSwap()
{
    FALSE_RETURN_MSG(renderContext_ != nullptr && eglSurface_ != EGL_NO_SURFACE, "egl not ready on draw");
    // 不再重复 MakeCurrent: 渲染线程自 InitializeEGLContext 起 context 恒为 current, 无人解绑;
    // 本帧周期的 ConsumeAndReport 已确保 current。省去每帧一次 eglMakeCurrent(内部持驱动锁)
    DrawImage();
    int64_t rt = renderTimeNs_.load();
    if (rt > 0 && outputWindow_ != nullptr) {
        // 按节奏时间戳标注上屏时刻, 利于音视频同步(对标 WaterMarkFilter 的 SET_UI_TIMESTAMP)
        OH_NativeWindow_NativeWindowHandleOpt(outputWindow_, SET_UI_TIMESTAMP, rt);
    }
    renderContext_->SwapBuffers(eglSurface_);
    renderRequested_ = false;
    frameState_ = static_cast<int>(FrameState::IDLE);
}

void AlphaVideoPostProcessor::ReportEos()
{
    MEDIA_LOG_I("ReportEos, pts:%{public}" PRId64, eosPts_.load());
    if (filterCallback_ != nullptr) {
        auto buffer = AVBuffer::CreateAVBuffer();
        if (buffer != nullptr) {
            buffer->pts_ = eosPts_.load();
            buffer->flag_ |= static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
            filterCallback_->OnOutputBufferAvailable(outIndex_.fetch_add(1, std::memory_order_relaxed), buffer);
        }
    }
    std::lock_guard<std::mutex> lock(wakeMutex_);
    eosPending_ = false; // 清标志, 防重复回传
}

void AlphaVideoPostProcessor::RenderLoop()
{
    if (!InitializeEGLContext()) {
        MEDIA_LOG_E("InitializeEGLContext failed, render loop exit");
        return;
    }
    if (!CreateGLResources() || !CreateVideoTexture()) {
        MEDIA_LOG_E("create GL resources failed, render loop exit");
        ReleaseGpuResources();
        ReleaseEglResources();
        return;
    }
    eglReady_ = true;
    MEDIA_LOG_I("RenderLoop ready");
    while (running_) {
        bool doConsume = false;
        bool doDraw = false;
        bool doEos = false;
        bool doFlushConsume = false;
        {
            std::unique_lock<std::mutex> lock(wakeMutex_);
            wakeCond_.wait(lock, [this]() {
                return !running_ ||
                       (started_.load() && availableFrameCnt_.load() > 0 &&
                           frameState_.load() == static_cast<int>(FrameState::IDLE)) ||
                       (started_.load() && renderRequested_.load() &&
                           frameState_.load() == static_cast<int>(FrameState::HELD)) ||
                       (started_.load() && flushDraining_.load()) ||
                       (eosPending_.load() && availableFrameCnt_.load() == 0 &&
                           frameState_.load() == static_cast<int>(FrameState::IDLE));
            });
            if (!running_) {
                break;
            }
            if (!started_.load() && !eosPending_.load()) {
                continue; // 暂停/停止态: 仅保持线程存活, 不消费不绘制(但 EOS 收尾仍需处理)
            }
            DecideFrameAction(doConsume, doDraw, doEos, doFlushConsume);
        }
        DealFrame(doConsume, doDraw, doEos, doFlushConsume);
    }
}

void AlphaVideoPostProcessor::DecideFrameAction(bool &doConsume, bool &doDraw, bool &doEos, bool &doFlushConsume)
{
    if (flushDraining_.load()) {
        // seek 排空优先于正常消费: 残留帧直接丢弃(不上报不绘制), 腾出 NativeImage 队列空位
        if (availableFrameCnt_.load() > 0 &&
            frameState_.load() == static_cast<int>(FrameState::IDLE)) {
            doFlushConsume = true;
        } else {
            // 排空完成(无残留或已消费完); 顺带强制 IDLE, 防 Flush 与消费竞态遗留 HELD 卡住
            flushDraining_.store(false);
            frameState_.store(static_cast<int>(FrameState::IDLE));
        }
    } else if (availableFrameCnt_.load() > 0 &&
        frameState_.load() == static_cast<int>(FrameState::IDLE)) {
        doConsume = true;
    } else if (renderRequested_.load() &&
               frameState_.load() == static_cast<int>(FrameState::HELD)) {
        doDraw = true;
    } else if (eosPending_.load() && availableFrameCnt_.load() == 0 &&
               frameState_.load() == static_cast<int>(FrameState::IDLE)) {
        doEos = true; // 有效帧全消费+上屏完, 回传 EOS
    } else {
        MEDIA_LOG_I("DecideFrameAction OK");
    }
}

void AlphaVideoPostProcessor::DealFrame(bool doConsume, bool doDraw, bool doEos, bool doFlushConsume)
{
    if (doFlushConsume) {
        FlushConsume();
    } else if (doConsume) {
        ConsumeAndReport();
    } else if (doDraw) {
        DrawAndSwap();
    } else if (doEos) {
        ReportEos();
    } else {
        MEDIA_LOG_I("DealFrame OK");
    }
}

void AlphaVideoPostProcessor::ReleaseGpuResources()
{
    if (renderContext_ == nullptr || eglSurface_ == EGL_NO_SURFACE) {
        return;
    }
    renderContext_->MakeCurrent(eglSurface_);
    if (vertexArrayObject_ != 0) {
        glDeleteVertexArrays(1, &vertexArrayObject_);
        vertexArrayObject_ = 0;
    }
    if (vertexBufferObject_ != 0) {
        glDeleteBuffers(1, &vertexBufferObject_);
        vertexBufferObject_ = 0;
    }
    if (nativeImageTexId_ != 0) {
        glDeleteTextures(1, &nativeImageTexId_);
        nativeImageTexId_ = 0;
    }
    shaderProgram_.reset();
    opaqueShaderProgram_.reset();
}

void AlphaVideoPostProcessor::ReleaseEglResources()
{
    if (renderContext_ != nullptr && eglSurface_ != EGL_NO_SURFACE) {
        renderContext_->DestroyEglSurface(eglSurface_);
        eglSurface_ = EGL_NO_SURFACE;
    }
    renderContext_.reset();
    eglReady_ = false;
}

} // namespace Media
} // namespace OHOS