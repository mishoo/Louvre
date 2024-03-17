#include <private/LTexturePrivate.h>
#include <private/LOutputPrivate.h>
#include <private/LCompositorPrivate.h>
#include <LRenderBuffer.h>
#include <LCompositor.h>
#include <GLES2/gl2.h>
#include <LLog.h>

using namespace Louvre;

LRenderBuffer::LRenderBuffer(const LSize &sizeB, bool alpha) noexcept
{
    m_type = Render;
    m_texture.m_sourceType = LTexture::Framebuffer;

    if (alpha)
        m_texture.m_format = DRM_FORMAT_BGRA8888;
    else
        m_texture.m_format = DRM_FORMAT_BGRX8888;

    m_texture.m_graphicBackendData = this;
    m_texture.m_sizeB.setW(1);
    m_texture.m_sizeB.setH(1);
    setSizeB(sizeB);
}

LRenderBuffer::~LRenderBuffer() noexcept
{
    for (auto &pair : m_threadsMap)
        compositor()->imp()->addRenderBufferToDestroy(pair.first, pair.second);
}

void LRenderBuffer::setSizeB(const LSize &sizeB) noexcept
{
    const LSize newSize {sizeB.w() <= 0 ? 1 : sizeB.w(), sizeB.h() <= 0 ? 1 : sizeB.h()};

    if (m_texture.sizeB() != newSize)
    {
        m_texture.m_sizeB = newSize;

        m_rect.setW(roundf(Float32(m_texture.m_sizeB.w()) * m_scale));
        m_rect.setH(roundf(Float32(m_texture.m_sizeB.h()) * m_scale));

        for (auto &pair : m_threadsMap)
            compositor()->imp()->addRenderBufferToDestroy(pair.first, pair.second);

        m_texture.reset();
        m_threadsMap.clear();
    }
}

const LTexture *LRenderBuffer::texture(Int32 index) const noexcept
{
    L_UNUSED(index);
    return &m_texture;
}

void LRenderBuffer::setFramebufferDamage(const LRegion *damage) noexcept
{
    L_UNUSED(damage);
}

LFramebuffer::Transform LRenderBuffer::transform() const noexcept
{
    return LFramebuffer::Normal;
}

Float32 LRenderBuffer::scale() const noexcept
{
    return m_scale;
}

const LSize &LRenderBuffer::sizeB() const noexcept
{
    return m_texture.sizeB();
}

const LRect &LRenderBuffer::rect() const noexcept
{
    return m_rect;
}

GLuint LRenderBuffer::id() const noexcept
{
    ThreadData &data { m_threadsMap[std::this_thread::get_id()] };

    if (!data.framebufferId)
    {
        glGenFramebuffers(1, &data.framebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, data.framebufferId);

        if (!m_texture.m_nativeId)
        {
            glGenTextures(1, &m_texture.m_nativeId);
            LTexture::LTexturePrivate::setTextureParams(m_texture.m_nativeId, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

            if (m_texture.format() == DRM_FORMAT_BGRA8888)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_texture.sizeB().w(), m_texture.sizeB().h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_texture.sizeB().w(), m_texture.sizeB().h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture.m_nativeId, 0);
    }

    return data.framebufferId;
}

Int32 LRenderBuffer::buffersCount() const noexcept
{
    return 1;
}

Int32 LRenderBuffer::currentBufferIndex() const noexcept
{
    return 0;
}
