#ifndef GL_FRAMEBUFFER_H
#define GL_FRAMEBUFFER_H
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "common.h"

class Framebuffer {
public:
	//static void blit(Framebuffer& source, Framebuffer&dest, )
	struct ColorAttachment{
		GLenum attachment;
		constexpr explicit ColorAttachment(uint32_t id) : attachment(GL_COLOR_ATTACHMENT0 + id) {}
		constexpr explicit operator GLenum() const { return attachment; }
	};

	enum class FramebufferTarget : GLenum {
		Read = GL_READ_FRAMEBUFFER,
		Write = GL_DRAW_FRAMEBUFFER
	};

	enum class Status: GLenum{
		Complete					= GL_FRAMEBUFFER_COMPLETE,
		IncompleteAttachment		= GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,
		IncompleteMissingAttachment = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,
		IncompleteDrawBuffer		= GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,
		IncompleteReadBuffer		= GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,
		Unsupported					= GL_FRAMEBUFFER_UNSUPPORTED,
		IncompleteMultisample		= GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
		IncompleteLayerTargets		= GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
	};

	explicit Framebuffer() noexcept { id_ = 0; }
	explicit Framebuffer(const Viewport& viewport);
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer(Framebuffer&& other) noexcept;

	~Framebuffer();

	Framebuffer& operator=(const Framebuffer&) = delete;
	Framebuffer& operator=(Framebuffer&& other) noexcept;

	GLuint id() const { return id_; }
	Status checkStatus(FramebufferTarget target);
	Framebuffer& clearColor()

private:
	GLuint id_;
	Viewport view_port_;
};

#endif
