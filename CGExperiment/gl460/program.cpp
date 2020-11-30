#include "program.h"
#include <algorithm>
#include <iostream>
#include <fstream>

namespace gl460{

Shader::Shader(const ShaderType type, const char* shader_file) {
	id_ = glCreateShader(static_cast<GLenum>(type));
	std::ifstream file{ shader_file, std::ifstream::binary };
	if (!file) {
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	} else {
		file.seekg(0, std::ios::end);
		size_t sz = file.tellg();
		file.seekg(0, std::ios::beg);
		//std::vector<char> source(sz);
		code_.resize(sz);
		file.read(code_.data(), sz);
	}
}

bool Shader::compile() {
	const char* shader_str = code_.c_str();
	glShaderSource(id_, 1, &shader_str, nullptr);
	glCompileShader(id_);

	GLint success, log_length;
	glGetShaderiv(id_, GL_COMPILE_STATUS, &success);
	glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &log_length);

	std::string message(log_length, '\0');
	if (!success) {
		glGetShaderInfoLog(id_, log_length, nullptr, &message[0]);
		std::cout << "Shader::compile error : " << message << std::endl;
	}
	return success;
}

Shader::~Shader() {
	if (!id_) return;
	glDeleteShader(id_);
}

Program::Program() {
	program_id_ = glCreateProgram();
}

Program::Program(Program&& other) noexcept : program_id_(other.program_id_) {
	other.program_id_ = 0;
}

Program & gl460::Program::operator=(Program && other) noexcept
{
	std::swap(program_id_, other.program_id_);
	return *this;
}

Program::~Program() {
	if (program_id_) {
		glDeleteProgram(program_id_);
	}
}

std::pair<bool, std::string> Program::validate() {
	glValidateProgram(program_id_);
	GLint success = true, log_length = 0;
	glGetProgramiv(program_id_, GL_VALIDATE_STATUS, &success);
	glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &log_length);

	std::string log(log_length, '\n');
	if (log_length > 1) {
		glGetProgramInfoLog(program_id_, log_length, nullptr, &log[0]);
	}
	log.resize(std::max(log_length, 1) - 1);
	return {success, std::move(log)};
}

bool Program::link() {
	glLinkProgram(program_id_);
	GLint success = true, log_length = 0;
	glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
	glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &log_length);
	std::string log(log_length, '\n');
	if (log_length > 1) {
		glGetProgramInfoLog(program_id_, log_length, nullptr, &log[0]);
	}
	log.resize(std::max(log_length, 1) - 1);
	if (!success) {
		std::cout << "Link Program Failed : " << log << std::endl;
	}

	return success;
}


void Program::attachShaders(gl460::ShaderType type, const std::string& path) {
	Shader s(type, path.c_str());
	s.compile();
	glAttachShader(program_id_, s.id());
}

void Program::attachShaders(std::map<gl460::ShaderType, std::string> shaders) {
	for (auto& iter : shaders) {
		attachShaders(iter.first, iter.second);
	}
}

void Program::use() {
	glUseProgram(program_id_);
}
} // namespace gl460

