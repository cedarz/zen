#ifndef GL_SHADER_H
#define GL_SHADER_H

#include <string>
#include <vector>
#include <glad/glad.h>

namespace gl460 {
struct ShaderState {
	explicit ShaderState(std::vector<std::string>& extensions) {}
	enum : std::size_t {
		StageCount = 4
	};
};

class Shader {
public:
	enum class Type : GLenum {
		Vertex = GL_VERTEX_SHADER,
		TesselationControl = GL_TESS_CONTROL_SHADER,
		TesselationEvalute = GL_TESS_EVALUATION_SHADER,
		Geometry = GL_GEOMETRY_SHADER,
		Fragment = GL_FRAGMENT_SHADER,
		Compute = GL_COMPUTE_SHADER
	};

	static bool compile(std::initializer_list<std::reference_wrapper<Shader>> shaders);

	explicit Shader(const Type type);
	Shader(const Shader&) = delete;
	Shader(Shader&& other) noexcept;
	~Shader();

	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&& other) noexcept;

	GLuint id() const { return id_; }
	std::vector<std::string> sources() const { return sources_; }
	Shader& addSource(std::string source); 
	Shader& addFile(const std::string& filename);
	bool compile() { return compile({*this}); }
private:
	GLuint id_;
	Type type_;
	std::vector<std::string> sources_;
};

}
#endif // !GL_SHADER_H
