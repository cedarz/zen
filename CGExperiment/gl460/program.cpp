#include "program.h"
#include <algorithm>
#include <iostream>

gl460::Program::Program() {
	id_ = glCreateProgram();
}

gl460::Program::Program(Program&& other) noexcept : id_(other.id_) {
	other.id_ = 0;
}

gl460::Program & gl460::Program::operator=(Program && other) noexcept
{
	std::swap(id_, other.id_);
	return *this;
}

gl460::Program::~Program() {
	if (id_) {
		glDeleteProgram(id_);
	}

}

std::pair<bool, std::string> gl460::Program::validate() {
	glValidateProgram(id_);
	GLint success = true, log_length = 0;
	glGetProgramiv(id_, GL_VALIDATE_STATUS, &success);
	glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &log_length);

	std::string log(log_length, '\n');
	if (log_length > 1) {
		glGetProgramInfoLog(id_, log_length, nullptr, &log[0]);
	}
	log.resize(std::max(log_length, 1) - 1);
	return {success, std::move(log)};
}

bool gl460::Program::link(std::initializer_list<std::reference_wrapper<Program>> shaders)
{
	bool all_success = true;
	for (Program& shader : shaders) {
		glLinkProgram(shader.id_);
		GLint success = true, log_length = 0;
		glGetProgramiv(shader.id_, GL_LINK_STATUS, &success);
		glGetProgramiv(shader.id_, GL_INFO_LOG_LENGTH, &log_length);
		std::string log(log_length, '\n');
		if (log_length > 1) {
			glGetProgramInfoLog(shader.id_, log_length, nullptr, &log[0]);
		}
		log.resize(std::max(log_length, 1) - 1);
		if (!success) {
			std::cout << "Link Program Failed : " << log << std::endl;
		}
		all_success = all_success && success;
	}
	return all_success;
}

void gl460::Program::attachShader(Shader & shader) {
	glAttachShader(id_, shader.id());
}

void gl460::Program::attachShaders(std::initializer_list<std::reference_wrapper<Shader>> shaders) {
	for (Shader& s : shaders) {
		attachShader(s);
	}
}

void gl460::Program::use() {
	glUseProgram(id_);
}


