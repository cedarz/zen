#include "shader.h"
#include <algorithm>
#include <fstream>
#include <iostream>
namespace gl460{
Shader::Shader(const Type type) : type_(type), id_(0) {
	id_ = glCreateShader(GLenum(type_));
}
Shader::~Shader() {
	if(!id_) return;
	glDeleteShader(id_);
}

Shader::Shader(Shader&& other) noexcept: 
	type_(other.type_), id_(other.id_), sources_(std::move(other.sources_)) {
	other.id_ = 0;	
}

Shader& Shader::operator=(Shader&& other) noexcept {
	std::swap(type_, other.type_);
	std::swap(id_, other.id_);
	std::swap(sources_, other.sources_);
	return *this;
}

Shader& Shader::addSource(std::string source) {
	//std::cout << source << std::endl;
	sources_.push_back(std::move(source));
	return *this;
}

Shader& Shader::addFile(const std::string& filename) {
	std::ifstream file{filename, std::ifstream::binary};
	if (!file) {
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	} else {
		file.seekg(0, std::ios::end);
		size_t sz = file.tellg();
		file.seekg(0, std::ios::beg);
		//std::string source;
		std::vector<char> source(sz);
		file.read(source.data(), sz);
		addSource(std::string{ source.begin(), source.end() });
	}
	return *this;
}

bool Shader::compile(std::initializer_list<std::reference_wrapper<Shader>> shaders) {
	bool all_success = true;
	for (Shader& shader : shaders) {
		std::vector<const GLchar*> pointers(shader.sources_.size(), nullptr);
		std::vector<GLint> lengths(shader.sources_.size(), 0);
		for (std::size_t i = 0; i < shader.sources_.size(); ++i) {
			pointers[i] = shader.sources_[i].data();
			lengths[i] = shader.sources_[i].size();
		}
		glShaderSource(shader.id_, shader.sources_.size(), pointers.data(), lengths.data());
		glCompileShader(shader.id_);

		GLint success, log_length;
		glGetShaderiv(shader.id_, GL_COMPILE_STATUS, &success);
		glGetShaderiv(shader.id_, GL_INFO_LOG_LENGTH, &log_length);

		std::string message(log_length, '\0');
		if (!success) {
			glGetShaderInfoLog(shader.id_, log_length, nullptr, &message[0]);

			std::cout << "Shader::compile error : " << message << std::endl;
		}
		all_success = all_success && success;
	}
	return all_success;
}
}
