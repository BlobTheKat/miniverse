#include "utils.cpp"

const buffer vert = asset(basic_vert);
const buffer frag = asset(basic_frag);

inline void init(){
	GLuint p = makePipeline(vert, frag);
	glUseProgram(p);
}
inline void frame(){
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}