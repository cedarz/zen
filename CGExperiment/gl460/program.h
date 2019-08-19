#ifndef GL_PROGRAM_H
#define GL_PROGRAM_H

#include <glad/glad.h>
#include <utility>
#include <string>

namespace gl460 {
class Program {
public:
	explicit Program();
	Program(const Program&) = delete;
	Program(Program&& other) noexcept;
	Program& operator=(const Program&) = delete;
	Program& operator=(Program&& other) noexcept;

	GLuint id() const { return id_; }
	std::pair<bool, std::string> validate();
	
	// void dispatchCompute(const Vector3ui& work_group_count);

	//static bool link(std::initializer_list<)

private:
	GLuint id_;
};
}


#endif // !GL_PROGRAM_H
