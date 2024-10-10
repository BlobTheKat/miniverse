#include "sdl.cpp"
#include "scene.h"

const buffer vert = asset(basic_vert);
const buffer frag = asset(basic_frag);

GLint mUni, oUni, cUni, dcUni;
inline void init(SDL_Window* win){
	GLuint p = makePipeline(vert, frag);
	mUni = glGetUniformLocation(p, "m");
	oUni = glGetUniformLocation(p, "o");
	cUni = glGetUniformLocation(p, "col");
	dcUni = glGetUniformLocation(p, "depthCoeff");
	glUseProgram(p);
	GLuint buf[2];
	glGenBuffers(2, buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[1]);
	mainScene.upload();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	glPrimitiveRestartIndex(65535);
	glEnable(GL_PRIMITIVE_RESTART);
	glEnable(GL_DEPTH_CLAMP);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

vec2 looking; f32 dist = 4;
f32 fov = 90;
vec3 pos{9, .75, 9};
inline void frame(){
	mat4 m; mat4x3 o;
	if(playing){
		looking.y = fmax(M_PI*-.5, fmin(looking.y + mouse.y*.005, M_PI*.5));
		looking.x = fmod(looking.x + mouse.x*.005, M_PI*2);
		if(mouse.w) fov = min(130.f, max(10.f, fov * pow(.85f, mouse.w)));
	}
	f32 ratio = f32(window.width)/f32(window.height);
	m.perspective<AREA_FOV>(fov, ratio, .01, 100.);
	//m.orthographic(-16, 16, -9, 9, -10, 10);
	m.rotateYZ(-looking.y);
	m.rotateXZ(-looking.x);
	m.translate(-pos);
	o.identity();
	glUniform4f(cUni, .9, .1, .2, 1);
	glUniformMatrix4fv(mUni, 1, false, m);
	glUniformMatrix4x3fv(oUni, 1, false, o);
	mainScene.draw();
}

inline void pausemenu(){
	
}