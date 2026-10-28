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

#ifndef FILTERS_WATER_MARK_FILTER_H
#define FILTERS_WATER_MARK_FILTER_H

#include <vector>
#include "opengl_video_filter_base.h"
#include "surface.h"

namespace OHOS {
namespace Media {
namespace Pipeline {

struct WaterMarkConfig {
   int32_t top;             // 水印左上角在视频行数
   int32_t left;            // 水印左上角在视频列数
   int32_t watermarkWidth;  // 水印缩放后的宽度
   int32_t watermarkHeight; // 水印缩放后的高度
   int32_t rawHeight;       // 水印原始的高度
   int32_t rawWidth;        // 水印原始的宽度
   int64_t size;                          // 水印原始的buffer的size
   std::unique_ptr<uint8_t[]> buffer;     // 水印原始的buffer
   int32_t rotation = 0;    // 水印旋转角度（0/90/180/270）
};

// WaterMarkFilter 仅负责 SDR 视频水印叠加（含可选的 SDR 降分辨率）。
// 通用 GL 视频渲染基础设施（EGL context、NativeImage、渲染线程、视频 OES 纹理渲染、VAO/VBO、
// Surface 管理、生命周期）由 OpenGLVideoFilterBase 提供。HDR/10bit 转码管线由兄弟类
// VideoResizeOpenGLFilter 通过同样的 virtual hook 注入，互不耦合。
class WaterMarkFilter : public OpenGLVideoFilterBase {
public:
    explicit WaterMarkFilter(std::string name, FilterType type);
    ~WaterMarkFilter() override;

    Status SetWatermark(std::shared_ptr<AVBuffer> &waterMarkBuffer, int32_t width, int32_t height);
    Status SetVideoResize(int32_t width, int32_t height) override;  // 设置视频缩放的目标宽高
    void SetAVRecorderMode();
    Status PreprocessWatermarks();
    Status ApplyRotation(int32_t rotation);
    std::shared_ptr<AVBuffer> GetMergedWatermarkBuffer(int32_t rotation);
    void SetHardWatermarkMode(bool isHardMode);
    bool HasHardWatermarkPositionViolation() const;

protected:
    // ---- OpenGLVideoFilterBase virtual hook overrides ----
    Status DoPrepare() override;
    void RenderFrame(int32_t viewWidth, int32_t viewHeight) override;
    bool OnGLResourcesCreated() override;
    void OnSetupOverlayUniforms() override;
    void OnReleaseGpuResources() override;
    void OnReleaseCpuResources() override;
    void OnReleaseShaders() override;
    // -------------------------------------------------------

    // ---- 水印专属成员 ----
    std::vector<WaterMarkConfig> watermarkList_;
    std::vector<GLuint> watermarkTexIds_;  // 各水印的 GL 纹理 ID
    GLuint textTexId_ = DEFINED_VALUE;
    GLuint mergedWatermarkTexId_ = 0;         // 合并后的水印纹理ID
    GLuint watermarkFBO_ = 0;                 // 用于合成的帧缓冲对象
    GLuint watermarkVAO_ = 0;                 // 水印合成用的VAO
    GLuint watermarkVBO_ = 0;
    std::unique_ptr<ShaderProgram> watermarkMergeShaderProgram_;  // 水印合成shader
    std::unique_ptr<ShaderProgram> watermarkOverlayProgram_;     // 水印叠加着色器
    std::atomic<bool> isHardWatermarkMode_ = false; //是否硬编水印模式
    int32_t wmBBoxLeft_ = 0;      // 水印包围盒左边界（像素，屏幕坐标系）
    int32_t wmBBoxTop_ = 0;       // 水印包围盒上边界
    int32_t wmBBoxWidth_ = 0;     // 水印包围盒宽度
    int32_t wmBBoxHeight_ = 0;    // 水印包围盒高度

    // ---- 水印专属方法 ----
    bool CreateWatermarkTextures();           // 为每个水印创建纹理
    bool MergeWaterMarksWithOpenGL();         // 使用OpenGL合成多个水印
    Status MergeAndSaveBBox();                // 合成水印并保存包围盒（PreprocessWatermarks/ApplyRotation共用）
    void ComputeWatermarkBoundingBox();       // 计算所有水印的旋转后包围盒
    bool CreateWatermarkFBO(int32_t width, int32_t height);  // 创建水印合并用的FBO
    bool CreateWatermarkMergeShader();        // 创建水印合并着色器
    bool RenderWatermarksToFBO(int32_t width, int32_t height);  // 渲染水印到FBO
    void CalculateRotatedWatermarkVertices(const WaterMarkConfig& config, int32_t width, int32_t height,
                                          float wmVertices[16], size_t wmVerticesSize);  // 根据旋转角度计算水印顶点坐标
    bool RenderSingleWatermarkToFBO(const WaterMarkConfig& config, GLuint texId, int32_t width,
                                    int32_t height);  // 渲染单个水印到FBO
    WaterMarkConfig ApplyRotationToConfig(const WaterMarkConfig& config, uint32_t rotation) const;
    bool CreateWatermarkVaoVbo();
    sptr<SurfaceBuffer> AllocateWatermarkSurfaceBuffer(int32_t wmWidth, int32_t wmHeight);
    bool CopyPixelsToSurfaceBuffer(const std::unique_ptr<uint8_t[]>& srcBuf,
        int32_t rowStride, int32_t wmHeight, const sptr<SurfaceBuffer>& surfaceBuffer);
    void ReleaseMergeResources();
    std::shared_ptr<AVBuffer> ReadMergedWatermarkPixels();
    void RenderWatermarkOverlay(int32_t viewWidth, int32_t viewHeight);
};
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
#endif // FILTERS_WATER_MARK_FILTER_H
