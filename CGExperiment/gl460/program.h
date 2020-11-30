#ifndef GL_PROGRAM_H
#define GL_PROGRAM_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <utility>
#include <string>
#include <map>

namespace gl460 {
enum class ShaderType : GLenum {
	Vertex = GL_VERTEX_SHADER,
	TesselationControl = GL_TESS_CONTROL_SHADER,
	TesselationEvalute = GL_TESS_EVALUATION_SHADER,
	Geometry = GL_GEOMETRY_SHADER,
	Fragment = GL_FRAGMENT_SHADER,
	Compute = GL_COMPUTE_SHADER
};

class Shader {
public:
	explicit Shader(const ShaderType type, const char* shader_file);
	GLuint id() const { return id_; }
	bool compile();
	~Shader();
private:
	GLuint id_;
	ShaderType type_;
	std::string code_;
};

class Program {
public:
	explicit Program();
	Program(const Program&) = delete;
	Program(Program&& other) noexcept;
	Program& operator=(const Program&) = delete;
	Program& operator=(Program&& other) noexcept;

	~Program();

	/// shader source
	void attachShaders(gl460::ShaderType type, const std::string& path);
	void attachShaders(std::map<gl460::ShaderType, std::string> shaders);



	GLuint id() const { return program_id_; }
	std::pair<bool, std::string> validate();
	
	// void dispatchCompute(const Vector3ui& work_group_count);

	bool link();
	void use();

	GLuint uniformLocation(const std::string& name) {
		const GLint location = glGetUniformLocation(program_id_, name.c_str());
		return location;
	}

	

	void setMat4(const std::string &name, const glm::mat4 &mat) {
		//glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
		glProgramUniformMatrix4fv(program_id_, uniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
	}

	void setVec3(const std::string& name, const glm::vec3 v) {
		glProgramUniform3fv(program_id_, uniformLocation(name), 1, glm::value_ptr(v));
	}

private:
	GLuint program_id_ = 0;
};
}


#endif // !GL_PROGRAM_H
