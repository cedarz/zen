#ifndef GL_PROGRAM_H
#define GL_PROGRAM_H

#include <glad/glad.h>
#include "shader.h"
#include <utility>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gl460 {
class Program {
public:
	explicit Program();
	Program(const Program&) = delete;
	Program(Program&& other) noexcept;
	Program& operator=(const Program&) = delete;
	Program& operator=(Program&& other) noexcept;

	~Program();

	GLuint id() const { return id_; }
	std::pair<bool, std::string> validate();
	
	// void dispatchCompute(const Vector3ui& work_group_count);

	static bool link(std::initializer_list<std::reference_wrapper<Program>> shaders);

	void attachShader(Shader& shader);
	void attachShaders(std::initializer_list<std::reference_wrapper<Shader>> shaders);
	bool link() { return link({*this}); }
	GLuint uniformLocation(const std::string& name) {
		const GLint location = glGetUniformLocation(id_, name.c_str());
		return location;
	}

	void use();

	void setMat4(const std::string &name, const glm::mat4 &mat) {
		//glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
		glProgramUniformMatrix4fv(id_, uniformLocation(name), 1, GL_FALSE, &mat[0][0]);
	}

	void setVec3(const std::string& name, const glm::vec3 v) {
		glProgramUniform3fv(id_, uniformLocation(name), 1, glm::value_ptr(v));
	}

private:
	GLuint id_ = 0;
};
}


#endif // !GL_PROGRAM_H
