#include "sdl.cpp"
#include "scene.h"

GLint mUni, oUni, cUni, dcUni, mUni2, tUni2;
GLuint def, sky;

inline void init(){
	def = makePipeline(asset(basic_vert), asset(basic_frag));
	sky = makePipeline(asset(sky_vert), asset(sky_frag));
	mUni2 = glGetUniformLocation(sky, "m");
	tUni2 = glGetUniformLocation(sky, "t");
	mUni = glGetUniformLocation(def, "m");
	oUni = glGetUniformLocation(def, "o");
	cUni = glGetUniformLocation(def, "col");
	GLuint buf[2];
	glGenBuffers(2, buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[1]);
	cube.upload();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	glPrimitiveRestartIndex(65535);
	glEnable(GL_PRIMITIVE_RESTART);
	glEnable(GL_DEPTH_CLAMP);
	//glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DITHER);
}

vec2 looking; f32 dist = 4;
f32 fov = 90;
vec3 pos{-9, .75, -9};
inline void frame(){
	//glClearColor(.2, .6, 1, 1);
	//glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mat4 m; mat4x3 o;
	if(playing){
		looking.y = fmax(M_PI*-.5, fmin(looking.y + mouse.y*.005, M_PI*.5));
		looking.x = fmod(looking.x + mouse.x*.005, M_PI*2);
		if(mouse.w) fov = min(130.f, max(10.f, fov * pow(.85f, mouse.w)));
		
		Uint8 controls = keys[SDL_SCANCODE_W] | keys[SDL_SCANCODE_S]<<1 | keys[SDL_SCANCODE_A]<<2 | keys[SDL_SCANCODE_D]<<3 | keys[SDL_SCANCODE_SPACE]<<4 | keys[SDL_SCANCODE_LSHIFT]<<5 | keys[SDL_SCANCODE_LCTRL]<<5;
		if(controls){
			const f32 fdt = dt * 8;
			const f32 dx = sin(looking.x) * fdt, dz = cos(looking.x) * fdt;
			if(controls&1) pos.x += dx, pos.z += dz;
			if(controls&2) pos.x -= dx, pos.z -= dz;
			if(controls&4) pos.x -= dz, pos.z += dx;
			if(controls&8) pos.x += dz, pos.z -= dx;
			if(controls&16) pos.y += fdt;
			if(controls&32) pos.y -= fdt;
		}
	}
	f32 ratio = f32(window.width)/f32(window.height);
	m.perspective<AREA_FOV>(fov, ratio, .01, 100.);
	m.rotateYZ(looking.y);
	m.rotateXZ(looking.x);
	glUseProgram(sky);
	glUniformMatrix4fv(mUni2, 1, false, m);
	glUniform1f(tUni2, t);
	glDepthFunc(GL_ALWAYS);
	cube.draw();
	glDepthFunc(GL_LESS);
	m.translate(-pos);
	o.identity();
	glUseProgram(def);
	glUniform4f(cUni, .9, .1, .2, 1);
	glUniformMatrix4fv(mUni, 1, false, m);
	glUniformMatrix4x3fv(oUni, 1, false, o);
	cube.draw();
}

inline void pausemenu(){

}