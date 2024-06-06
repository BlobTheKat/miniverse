#include "sdl.cpp"
#include "scene.h"

const buffer vert = asset(basic_vert);
const buffer frag = asset(basic_frag);

GLint mUni, oUni, cUni;
inline void init(){
	GLuint p = makePipeline(vert, frag);
	glUseProgram(p);
	GLuint v;
	glGenVertexArrays(1, &v);
	glBindVertexArray(v);
	GLuint buf[2];
	glGenBuffers(2, buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[1]);
	mainScene.upload();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	mUni = glGetUniformLocation(p, "m");
	oUni = glGetUniformLocation(p, "o");
	cUni = glGetUniformLocation(p, "col");
	glPrimitiveRestartIndex(65535);
	glEnable(GL_PRIMITIVE_RESTART);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST);
	glPointSize(2);
}
vec2 looking; float dist = 4;
inline void frame(){
	Mat m, o;
	if(playing){
		looking.y = fmax(M_PI*-.5, fmin(looking.y + mouse.y*.005, M_PI*.5));
		looking.x = fmod(looking.x + mouse.x*.005, M_PI*2);
		dist *= powf(.9, mouse.z);
	}
	cameraMatrix(m, 100, float(window.width)/float(window.height), .01, 100);
	m.translate(-vec3(0., 0., dist));
	m.pitch(-looking.y);
	m.yaw(-looking.x);
	idMatrix(o);
	o.pitch(t); o.yaw(t*.5);
	o.translate(vec3(0, -1, 0));
	
	glUniform4f(cUni, .8, 0, .2, 1);
	glUniformMatrix4fv(mUni, 1, false, m);
	glUniformMatrix3x4fv(oUni, 1, false, o);
	mainScene.draw();
}

inline void pausemenu(){
	
}