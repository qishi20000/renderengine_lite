#include "backend/opengl/GLDriver.h"

#include <GLES3/gl3.h>
#include <cassert>
#include <cstring>

// NOTE: this is an intentionally minimal, milestone-M1-scope implementation
// (see ARCHITECTURE.md section 11). It covers enough of DriverApi to stand
// up static geometry with a simple program. Camera external-texture import,
// UBO-based uniforms, and Adreno-specific extension usage (tiled_rendering,
// shader_framebuffer_fetch, timer queries) are added incrementally in later
// milestones — search for TODO(M3)/TODO(M6) markers.

namespace rel::backend::opengl {

namespace {

GLenum toGLTarget(BufferUsage) { return 0; } // buffers pick target by call site, not usage

GLenum bufferUsageToGLEnum(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static:    return GL_STATIC_DRAW;
        case BufferUsage::Dynamic:   return GL_DYNAMIC_DRAW;
        case BufferUsage::Streaming: return GL_STREAM_DRAW;
    }
    return GL_STATIC_DRAW;
}

GLenum primitiveTypeToGLEnum(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Points:        return GL_POINTS;
        case PrimitiveType::Lines:         return GL_LINES;
        case PrimitiveType::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveType::Triangles:     return GL_TRIANGLES;
        case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
    }
    return GL_TRIANGLES;
}

GLuint compileShader(GLenum stage, std::string_view src) {
    GLuint shader = glCreateShader(stage);
    const char* srcPtr = src.data();
    GLint len = static_cast<GLint>(src.size());
    glShaderSource(shader, 1, &srcPtr, &len);
    glCompileShader(shader);
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        // TODO: route through utils/Log instead of stderr once utils/ lands.
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[REL][GLDriver] shader compile failed: %s\n", log);
    }
    return shader;
}

} // namespace

GLDriver::GLDriver(Platform& platform) : mPlatform(platform) {
    // Default render target represents the window surface (FBO 0).
    mDefaultRenderTarget = mRenderTargets.allocate(GLRenderTarget{0, 0, 0});
}

GLDriver::~GLDriver() = default;

void GLDriver::makeCurrent() { mPlatform.makeCurrent(); }
void GLDriver::commit() { mPlatform.swapBuffers(); }

VertexBufferHandle GLDriver::createVertexBuffer(const void* data, uint32_t sizeBytes, BufferUsage usage) {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, sizeBytes, data, bufferUsageToGLEnum(usage));
    return mVertexBuffers.allocate(GLBuffer{id, GL_ARRAY_BUFFER, sizeBytes});
}

IndexBufferHandle GLDriver::createIndexBuffer(const void* data, uint32_t sizeBytes, BufferUsage usage) {
    GLuint id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeBytes, data, bufferUsageToGLEnum(usage));
    return mIndexBuffers.allocate(GLBuffer{id, GL_ELEMENT_ARRAY_BUFFER, sizeBytes});
}

TextureHandle GLDriver::createTexture(const TextureDescriptor& desc) {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // TODO(M1): map PixelFormat -> (internalFormat, format, type) properly.
    glTexStorage2D(GL_TEXTURE_2D, static_cast<GLsizei>(desc.levels),
                   GL_RGBA8, static_cast<GLsizei>(desc.width), static_cast<GLsizei>(desc.height));
    return mTextures.allocate(GLTexture{id, GL_TEXTURE_2D, desc});
}

TextureHandle GLDriver::importExternalTexture(void* nativeImage) {
    // TODO(M3): create a GL_TEXTURE_EXTERNAL_OES name, call
    // glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, nativeImage) via
    // the Platform-provided EGLImageKHR. See avm/CameraStreamManager and
    // ARCHITECTURE.md section 7 (zero-copy camera import).
    (void)nativeImage;
    assert(false && "importExternalTexture: not implemented until M3 (camera integration)");
    return {};
}

ProgramHandle GLDriver::createProgram(const ProgramDescriptor& desc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, desc.vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, desc.fragmentSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[REL][GLDriver] program link failed: %s\n", log);
    }
    return mPrograms.allocate(GLProgram{program});
}

RenderTargetHandle GLDriver::createRenderTarget(TextureHandle color, TextureHandle depth,
                                                 uint32_t width, uint32_t height) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    if (auto* tex = mTextures.get(color)) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->glId, 0);
    }
    if (auto* tex = mTextures.get(depth)) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->glId, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return mRenderTargets.allocate(GLRenderTarget{fbo, width, height});
}

RenderTargetHandle GLDriver::getDefaultRenderTarget() { return mDefaultRenderTarget; }

void GLDriver::updateVertexBuffer(VertexBufferHandle h, const void* data, uint32_t sizeBytes, uint32_t offset) {
    if (auto* buf = mVertexBuffers.get(h)) {
        glBindBuffer(GL_ARRAY_BUFFER, buf->glId);
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeBytes, data);
    }
}

void GLDriver::updateTexture(TextureHandle h, const void* data, uint32_t sizeBytes) {
    (void)sizeBytes;
    if (auto* tex = mTextures.get(h)) {
        glBindTexture(GL_TEXTURE_2D, tex->glId);
        // TODO(M1): honor mip level / sub-rect once needed; full-image upload for now.
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                         static_cast<GLsizei>(tex->desc.width), static_cast<GLsizei>(tex->desc.height),
                         GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
}

void GLDriver::destroyVertexBuffer(VertexBufferHandle h) {
    if (auto* buf = mVertexBuffers.get(h)) { glDeleteBuffers(1, &buf->glId); }
    mVertexBuffers.free(h);
}
void GLDriver::destroyIndexBuffer(IndexBufferHandle h) {
    if (auto* buf = mIndexBuffers.get(h)) { glDeleteBuffers(1, &buf->glId); }
    mIndexBuffers.free(h);
}
void GLDriver::destroyTexture(TextureHandle h) {
    if (auto* tex = mTextures.get(h)) { glDeleteTextures(1, &tex->glId); }
    mTextures.free(h);
}
void GLDriver::destroyProgram(ProgramHandle h) {
    if (auto* prog = mPrograms.get(h)) { glDeleteProgram(prog->glId); }
    mPrograms.free(h);
}
void GLDriver::destroyRenderTarget(RenderTargetHandle h) {
    if (auto* rt = mRenderTargets.get(h); rt && rt->fboId != 0) { glDeleteFramebuffers(1, &rt->fboId); }
    mRenderTargets.free(h);
}

void GLDriver::beginRenderPass(const RenderPassDescriptor& desc) {
    auto* rt = mRenderTargets.get(desc.target);
    glBindFramebuffer(GL_FRAMEBUFFER, rt ? rt->fboId : 0);
    if (rt) glViewport(0, 0, static_cast<GLsizei>(rt->width), static_cast<GLsizei>(rt->height));

    GLbitfield clearMask = 0;
    if (desc.clearColorBuffer) {
        glClearColor(desc.clearColor[0], desc.clearColor[1], desc.clearColor[2], desc.clearColor[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    if (desc.clearDepthBuffer) {
        glClearDepthf(desc.clearDepth);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    if (clearMask) glClear(clearMask);
}

void GLDriver::endRenderPass() {
    // TODO(M6): glInvalidateFramebuffer() for attachments not needed after
    // this pass (e.g. depth in the single-pass surround view pass) — see
    // ARCHITECTURE.md section 7, point 3. Needs RenderPassDescriptor wiring.
}

void GLDriver::bindProgram(ProgramHandle h) {
    mCurrentProgram = h;
    if (auto* prog = mPrograms.get(h)) glUseProgram(prog->glId);
}

void GLDriver::bindTexture(uint32_t unit, TextureHandle h, const SamplerParams& params) {
    auto* tex = mTextures.get(h);
    if (!tex) return;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(tex->target, tex->glId);
    auto toGL = [](SamplerFilter f) { return f == SamplerFilter::Linear ? GL_LINEAR : GL_NEAREST; };
    auto toGLWrap = [](SamplerWrap w) {
        switch (w) {
            case SamplerWrap::Repeat: return GL_REPEAT;
            case SamplerWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
            case SamplerWrap::ClampToEdge: default: return GL_CLAMP_TO_EDGE;
        }
    };
    glTexParameteri(tex->target, GL_TEXTURE_MIN_FILTER, toGL(params.minFilter));
    glTexParameteri(tex->target, GL_TEXTURE_MAG_FILTER, toGL(params.magFilter));
    glTexParameteri(tex->target, GL_TEXTURE_WRAP_S, toGLWrap(params.wrapS));
    glTexParameteri(tex->target, GL_TEXTURE_WRAP_T, toGLWrap(params.wrapT));
}

void GLDriver::setUniformMat4(std::string_view name, const float* mat4x4) {
    auto* prog = mPrograms.get(mCurrentProgram);
    if (!prog) return;
    // TODO(M1): cache uniform locations per-program instead of querying by
    // name every call; fine for the bring-up milestone.
    GLint loc = glGetUniformLocation(prog->glId, std::string(name).c_str());
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, mat4x4);
}

void GLDriver::setUniformFloat4(std::string_view name, const float* v4) {
    auto* prog = mPrograms.get(mCurrentProgram);
    if (!prog) return;
    GLint loc = glGetUniformLocation(prog->glId, std::string(name).c_str());
    if (loc >= 0) glUniform4fv(loc, 1, v4);
}

void GLDriver::setRasterState(const RasterState& state) {
    if (state.cullMode == CullMode::None) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(state.cullMode == CullMode::Front ? GL_FRONT : GL_BACK);
    }
    if (state.depthFunc == DepthFunc::Always && !state.depthWrite) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
        static const GLenum kFuncs[] = {GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_ALWAYS};
        glDepthFunc(kFuncs[static_cast<int>(state.depthFunc)]);
        glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);
    }
    if (state.blendEnable) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

void GLDriver::draw(PrimitiveType type, VertexBufferHandle vb, IndexBufferHandle ib,
                     uint32_t indexCount, const VertexAttribute* attributes, uint32_t attributeCount) {
    auto* vbuf = mVertexBuffers.get(vb);
    auto* ibuf = mIndexBuffers.get(ib);
    if (!vbuf) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbuf->glId);
    for (uint32_t i = 0; i < attributeCount; ++i) {
        const auto& attr = attributes[i];
        GLint components = 1;
        GLenum glType = GL_FLOAT;
        switch (attr.type) {
            case ElementType::Float:  components = 1; glType = GL_FLOAT; break;
            case ElementType::Float2: components = 2; glType = GL_FLOAT; break;
            case ElementType::Float3: components = 3; glType = GL_FLOAT; break;
            case ElementType::Float4: components = 4; glType = GL_FLOAT; break;
            case ElementType::UByte4: components = 4; glType = GL_UNSIGNED_BYTE; break;
            case ElementType::UShort: components = 1; glType = GL_UNSIGNED_SHORT; break;
            case ElementType::UInt:   components = 1; glType = GL_UNSIGNED_INT; break;
        }
        glEnableVertexAttribArray(attr.bufferSlot);
        glVertexAttribPointer(attr.bufferSlot, components, glType,
                               attr.normalized ? GL_TRUE : GL_FALSE,
                               static_cast<GLsizei>(attr.stride),
                               reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.offset)));
    }

    if (ibuf) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf->glId);
        glDrawElements(primitiveTypeToGLEnum(type), static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(primitiveTypeToGLEnum(type), 0, static_cast<GLsizei>(indexCount));
    }
}

} // namespace rel::backend::opengl
