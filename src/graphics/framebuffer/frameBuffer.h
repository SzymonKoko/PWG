#ifndef SRC_GRAPHICS_BUFFER_FRAME_BUFFER_H_
#define SRC_GRAPHICS_BUFFER_FRAME_BUFFER_H_

#include "graphics/texture/texture.h"
#include <vector>
#include <memory>

namespace pwg
{
	/**
	* @brief Class responsible for managing an OpenGL Frame Buffer Object (FBO).
	* Provides functionality for off-screen rendering, resizing, and cleanup.
	*/
	class FrameBuffer
	{
	public:

		/**
		* @brief Constructs a FrameBuffer with specified dimensions.
		* Initializes color attachment texture and optionally a Render Buffer Object (RBO) for depth and stencil.
		* @param width Width of the framebuffer in pixels.
		* @param height Height of the framebuffer in pixels.
		* @param useRBO If true, creates a Render Buffer Object for depth/stencil attachments.
		*/
		FrameBuffer(int width, int height, bool useRBO);

		/**
		* @brief Copy constructor. Creates a new framebuffer with the same configuration as another.
		* @param otherFBO Reference to another FrameBuffer instance.
		*/
		FrameBuffer(const pwg::FrameBuffer& otherFBO);

		/**
		* @brief Copy assignment operator.
		* @param otherFBO Reference to another FrameBuffer instance.
		* @return Reference to this FrameBuffer.
		*/
		FrameBuffer& operator=(const FrameBuffer& otherFBO);

		/**
		* @brief Destructor. Deletes framebuffer and associated OpenGL resources.
		*/
		~FrameBuffer();

		/**
		* @brief Binds the framebuffer as the current render target.
		*/
		void Bind();

		/**
		* @brief Unbinds the framebuffer and restores the default render target.
		*/
		void Unbind();

		/**
		* @brief Deletes all OpenGL objects related to this framebuffer.
		* Should be called before recreating or resizing manually.
		*/
		void Delete();

		/**
		* @brief Resizes the framebuffer to a new resolution.
		* Reinitializes the texture and (if used) render buffer attachments.
		* @param width New framebuffer width in pixels.
		* @param height New framebuffer height in pixels.
		*/
		void Resize(int width, int height);

		/**
		* @brief Getter for the OpenGL framebuffer ID.
		* @return Framebuffer object ID.
		*/
		unsigned int GetID() { return m_frameBufferObjectID; }

		/**
		* @brief Getter for the texture attached to this framebuffer.
		* @return OpenGL texture ID used as the framebuffer color attachment.
		*/
		unsigned int GetTextureID() { return m_fboTexture->GetTextureID(); }

		/**
		* @brief Getter for the framebuffer width.
		* @return Width in pixels.
		*/
		int GetWidth() { return m_width; }

		/**
		* @brief Getter for the framebuffer height.
		* @return Height in pixels.
		*/
		int GetHeight() { return m_height; }

	private:
		unsigned int m_frameBufferObjectID{ 0 };			/**< OpenGL Framebuffer Object ID. */
		unsigned int m_renderBufferObjectID{ 0 };			/**< Optional Render Buffer Object ID (for depth/stencil). */
		std::shared_ptr<Texture> m_fboTexture{ nullptr };	/**< Texture attached as color output. */
		int m_width{ 800 };									/**< Framebuffer width in pixels. */
		int m_height{ 800 };								/**< Framebuffer height in pixels. */
		bool m_useRBO{ false };								/**< Indicates whether a Render Buffer Object is used. */

	private:

		/**
		* @brief Initializes framebuffer attachments and OpenGL objects.
		* Creates a color texture and optionally a render buffer for depth/stencil.
		*/
		void InitializeFrameBuffer();

		/**
		* @brief Cleans up existing OpenGL framebuffer resources.
		* Used internally before resizing or destroying.
		*/
		void CleanUp();

	};
} // namespace pwg

#endif // !SRC_GRAPHICS_BUFFER_FRAME_BUFFER_H_
