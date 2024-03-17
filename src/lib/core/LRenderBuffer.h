#ifndef LRENDERBUFFER_H
#define LRENDERBUFFER_H

#include <LTexture.h>
#include <LFramebuffer.h>

/**
 * @brief Custom render destination framebuffer
 *
 * The LRenderBuffer can be utilized to render to a framebuffer instead of an LOutput and then use that rendered content as a texture.\n
 * Essentially, it acts as a wrapper for an OpenGL framebuffer with additional functionality. You can obtain the OpenGL framebuffer ID using the id() method.
 * It can also be utilized with an LPainter by using LPainter::bindFramebuffer().
 *
 * @note A framebuffer possess properties such as position, size, and buffer scale. LPainter uses these properties to properly position and scale the rendered content.
 */
class Louvre::LRenderBuffer final : public LFramebuffer
{
public:   
    /**
     * @brief Constructor for LRenderBuffer with specified size.
     *
     * This constructor creates an LRenderBuffer with the specified size in buffer coordinates.
     *
     * @param sizeB The size of the framebuffer in buffer coordinates.
     * @param alpha Employ a format with an alpha component.
     */
    LRenderBuffer(const LSize &sizeB, bool alpha = true) noexcept;

    /// @cond OMIT
    LRenderBuffer(const LRenderBuffer&) = delete;
    LRenderBuffer& operator= (const LRenderBuffer&) = delete;
    /// @endcond

    /**
     * @brief Destructor for LRenderBuffer.
     */
    ~LRenderBuffer() noexcept;

    /**
     * @brief Retrieve the OpenGL ID of the framebuffer.
     *
     * This method returns the OpenGL ID of the framebuffer associated with the LRenderBuffer.
     *
     * @return The OpenGL ID of the framebuffer.
     */
    GLuint id() const noexcept override;

    /**
     * @brief Retrieve the texture associated with the framebuffer.
     *
     * LRenderBuffers always have a single framebuffer, so 0 must always be passed as an argument.
     * The index is part of the LFramebuffer parent class, which may be used by special framebuffers like those of LOutput.
     *
     * @param index The index of the texture (default is 0).
     * @return A pointer to the texture associated with the framebuffer.
     */
    const LTexture *texture(Int32 index = 0) const noexcept override;

    /**
     * @brief Set the size of the framebuffer in buffer coordinates.
     *
     * @warning The existing framebuffer is destroyed and replaced by a new one each time this method is called, so it should be used with moderation.
     *
     * @param sizeB The new size of the framebuffer in buffer coordinates.
     */
    void setSizeB(const LSize &sizeB) noexcept;

    /**
     * @brief Retrieve the size of the framebuffer in buffer coordinates.
     *
     * This method returns the size of the framebuffer in buffer coordinates.
     *
     * @return The size of the framebuffer.
     */
    const LSize &sizeB() const noexcept override;

    /**
     * @brief Retrieve the size of the framebuffer.
     *
     * This method returns the size of the framebuffer in surface coordinates. This is sizeB() / scale().
     *
     * @return The size of the framebuffer.
     */
    inline const LSize &size() const noexcept
    {
        return m_rect.size();
    }

    /**
     * @brief Set the position of the framebuffer.
     *
     * This method sets the position of the framebuffer in surface coordinates.
     *
     * @param pos The new position of the framebuffer.
     */
    inline void setPos(const LPoint &pos) noexcept
    {
        m_rect.setPos(pos);
    }

    /**
     * @brief Retrieve the position of the framebuffer.
     *
     * This method returns the position of the framebuffer in surface coordinates.
     *
     * @return The position of the framebuffer.
     */
    inline const LPoint &pos() const noexcept
    {
        return m_rect.pos();
    }

    /**
     * @brief Retrieve the position and size of the framebuffer in surface coordinates.
     *
     * The size provided by this rect is equal to size().
     *
     * @return The position and size of the framebuffer in surface coordinates.
     */
    const LRect &rect() const noexcept override;

    /**
     * @brief Set the buffer scale to properly scale the rendered content.
     *
     * For example, framebuffers used in HiDPI displays should have a scale of 2 or greater.
     *
     * @param scale The buffer scale factor.
     */
    inline void setScale(Float32 scale) noexcept
    {
        if (scale < 0.25f)
            scale = 0.25;

        if (m_scale != scale)
        {
            m_rect.setW(roundf(Float32(m_texture.sizeB().w())/scale));
            m_rect.setH(roundf(Float32(m_texture.sizeB().h())/scale));
            m_scale = scale;
        }
    }

    /**
     * @brief Retrieve the buffer scale of the framebuffer.
     *
     * This method returns the buffer scale factor of the framebuffer.
     *
     * @return The buffer scale factor.
     */
    Float32 scale() const noexcept override;

    Int32 buffersCount() const noexcept override;
    Int32 currentBufferIndex() const noexcept override;
    void setFramebufferDamage(const LRegion *damage) noexcept override;
    Transform transform() const noexcept override;

private:
    friend class LCompositor;
    struct ThreadData
    {
        GLuint framebufferId = 0;
    };
    mutable LTexture m_texture;
    LRect m_rect;
    Float32 m_scale { 1.f };
    mutable std::unordered_map<std::thread::id, ThreadData> m_threadsMap;
};

#endif // LRENDERBUFFER_H
