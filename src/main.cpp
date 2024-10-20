#include "sdl.cpp"
#include "scene.h"
#include "runtime.cpp"

GLint mUni, volUni, camUni, dcUni, mUni2, tUni2;
GLuint cb, sky;

GLuint cbtex;

inline void init(){
	cb = makePipeline(asset(cube_vert), asset(cube_frag));
	sky = makePipeline(asset(sky_vert), asset(sky_frag));
	mUni2 = glGetUniformLocation(sky, "m");
	tUni2 = glGetUniformLocation(sky, "t");
	mUni = glGetUniformLocation(cb, "m");
	volUni = glGetUniformLocation(cb, "vol");
	camUni = glGetUniformLocation(cb, "cam");
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
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_3D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glGenTextures(1, &cbtex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, cbtex);
	
	bvec4* data = (bvec4*) SDL_malloc(16777216*sizeof(bvec4));
	for(int x = 0; x < 256; x++) for(int y = 0; y < 256; y++) for(int z = 0; z < 256; z++){
		f32 h = hypot(f32(x)-127.5, f32(y)-127.5, f32(z)-127.5);
		data[z<<16|y<<8|x] = h < 128 ? bvec4{230,26,77,255} : bvec4{0};
	}
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	SDL_free(data);
	glGenerateMipmap(GL_TEXTURE_3D);
	test();
}

vec2 looking; f32 dist = 4;
f32 fov = 90;
vec3 pos{0, .75, -120};
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
	m.rotateZY(-looking.y);
	m.rotateZX(-looking.x);
	glUseProgram(sky);
	glUniformMatrix4fv(mUni2, 1, false, m);
	glUniform1f(tUni2, t);
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_CULL_FACE);
	if(keys[SDL_SCANCODE_TAB]){
		f32 w = fov*.1;
		m.orthographic(-w*ratio, w*ratio, -w, w, -w, 4*w);
		m.rotateZY(-looking.y);
		m.rotateZX(-looking.x);
	}
	cube.draw();
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	m.translate(-pos);
	o.identity();
	o.scale(100);
	m *= o;
	glUseProgram(cb);
	glUniformMatrix4fv(mUni, 1, false, m);
	glUniform3fv(camUni, 1, o.invert(pos));
	glUniform1i(volUni, 0);
	cube.draw();
}

inline void pausemenu(){

}