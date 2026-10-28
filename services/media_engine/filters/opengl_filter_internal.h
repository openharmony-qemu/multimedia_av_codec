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

// 内部实现头文件，仅供 opengl_video_filter_base.cpp / water_mark_filter.cpp /
// video_resize_opengl_filter.cpp 三个实现文件共用，不属于 InnerAPI，不对外暴露。
// 收敛此前在三个 .cpp 匿名命名空间中重复定义的 GL 工具（ScopedFboBind、SetTextureParams2D、
// CHECK_GL_ERROR、QUAD_INDICES、SHARED_VERTEX_SHADER），消除 ODR 无冲突但语义重复的代码。

#ifndef OPENGL_FILTER_INTERNAL_H
#define OPENGL_FILTER_INTERNAL_H

#include <GLES3/gl3.h>
#include <string>
#include "common/log.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
namespace GlInternal {

// RAII 绑定 FBO，析构时恢复先前绑定的 FBO，避免污染调用方 GL 状态。
class ScopedFboBind {
public:
    explicit ScopedFboBind(GLuint fbo) : prevFbo_(0)
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&prevFbo_));
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }
    ~ScopedFboBind() { glBindFramebuffer(GL_FRAMEBUFFER, prevFbo_); }
private:
    GLuint prevFbo_;
};

// 为 2D 纹理目标设置默认的 CLAMP_TO_EDGE + LINEAR 过滤参数。
inline void SetTextureParams2D(GLenum target)
{
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// 检查 GL 错误并返回 false + 打印日志。每个使用此宏的 .cpp 文件需在文件末尾 #undef。
#define CHECK_GL_ERROR(msg) \
    do { GLenum e_ = glGetError(); \
         FALSE_RETURN_V_MSG(e_ == GL_NO_ERROR, false, msg ": %{public}d", e_); \
    } while (0)

// 四边形索引数据：两个三角形组成一个 quad，供 glDrawElements 使用。
static constexpr GLuint QUAD_INDICES[] = {
    0, 1, 2,  // 第一个三角形
    0, 2, 3   // 第二个三角形
};

// 视频直通与水印叠加共享的顶点着色器：输出 NDC 位置 + 视频/水印两套纹理坐标。
// WaterMark 的 overlay shader 与基类的视频直通 shader 均使用此 vertex shader。
static const std::string SHARED_VERTEX_SHADER = R"delimiter(
attribute vec3 position;
attribute vec2 texCoord;

varying vec2 vVideoTexCoord;
varying vec2 vWmTexCoord;

void main()
{
    gl_Position = vec4(position, 1.0);
    vVideoTexCoord = vec2(texCoord.x, texCoord.y);
    vWmTexCoord = vec2(texCoord.x, texCoord.y);
}
)delimiter";

} // namespace GlInternal
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // OPENGL_FILTER_INTERNAL_H
