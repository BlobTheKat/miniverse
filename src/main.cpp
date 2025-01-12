#include "sdl.cpp"
#include "scene.h"
#include "sim/sim.cpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

GLint whUni, whUniStar, xyUni, difftUni, noiseUni, tUni;
GLuint objShader, starShader;

GLuint buf, va;

physics::Simulation sim;

inline void init(){
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	objShader = makePipeline(asset(obj_vert), asset(obj_frag));
	whUni = glGetUniformLocation(objShader, "i_wh");
	xyUni = glGetUniformLocation(objShader, "xy_off");
	difftUni = glGetUniformLocation(objShader, "dt");
	noiseUni = glGetUniformLocation(objShader, "noise");
	tUni = glGetUniformLocation(objShader, "t");
	starShader = makePipeline(asset(fullscreen_vert), asset(star_frag));
	whUniStar = glGetUniformLocation(starShader, "wh");

	glUseProgram(objShader);
	glUniform1i(noiseUni, 0);
	GLuint texs[2];
	glGenTextures(2, texs);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texs[0]);
	vector<u8> data; int w, h, ch;
	stbi_uc* img = stbi_load_from_memory((stbi_uc*) asset_start(noise_png), asset_size(noise_png), &w, &h, &ch, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
	stbi_image_free(img);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texs[1]);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, 1, 1, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 0);
	glGenBuffers(1, &buf);
	glGenVertexArrays(1, &va);
	glBindVertexArray(va);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, sizeof(physics::Sprite), 0);
	glVertexAttribPointer(1, 1, GL_FLOAT, false, sizeof(physics::Sprite), (void*)offsetof(physics::Sprite, radius));
	glVertexAttribIPointer(2, 2, GL_UNSIGNED_INT, sizeof(physics::Sprite), (void*)offsetof(physics::Sprite, style));
	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glBindVertexArray(0);
	sim.add_attraction_rule({
		.prop = 0,
		.activator = 0,
		.dir = vec2(0, 2),
		.name = "Gravity"
	});
	srand(time(0));
	f32 m = 1; f64 x = 0;
	/*for(int i=0;i<40;i++){
		f32 r = m; m *= 3.;
		physics::Node& n = sim.add_node(x, r, r, r*r, 0, 0);
		x += r*8;
		using physics::style;
		int c = rand();
		int a = c&63, b = c>>6&63; if(a>b) swap(a,b);
		float f = 0.001 * float(5+(c>>12&15));
		n.style = style {
			//.col1 = {.4,.5,.9}, .col2 = style::BRIGHTEN_25,
			.col1 = {.4+float(a)*f,.4+float(b-a)*f,.4+float(63-b)*f}, .col2 = style::BRIGHTEN_25,
			.blend = style::BLEND_50, .glow = style::GLOW_STRONG,
			.speed = 2,
			.noise_freq = 2_f4, .noise_stretch = 2_f4,
			.atm_col = style::DARKEN_25, .atm_size = 2_f4,
			.gradient = style::SMALL_WHITE
		};
	}*/
	physics::Node& n = sim.add_node(0, 0, 10, 10, 0, 0);
	using physics::style;
	n.style = style {
		.col1 = {.4,.5,.9}, .col2 = style::BRIGHTEN_25,
		.blend = style::BLEND_50, .glow = style::GLOW_STRONG,
		.speed = 2,
		.noise_freq = 2_f4, .noise_stretch = 2_f4,
		.atm_col = style::DARKEN_25, .atm_size = 2_f4,
		.gradient = style::SMALL_WHITE
	};
	for(int i=200; i < 1'000'000; i++){
		f32 th = f32(rand()) * (PI2/RAND_MAX);
		f32 ax = sin(th), ay = cos(th);
		f32 dx = ay*20, dy = ax*-20;
		f32 r = physics::fast_sqrt(i*50);
		physics::Node& n = sim.add_node(ax*r, ay*r, 1, .1, dx, dy);
		n.style = style {
			.col1 = {.2,.3,.7}, .col2 = style::HUE_240DEG,
			.blend = style::BLEND_0, .glow = style::GLOW_0,
			.speed = .2,
			.noise_freq = 1_f4, .noise_stretch = 1_f4,
			.atm_col = style::BRIGHTEN_25, .atm_size = 1.5_f4,
			.gradient = style::MEDIUM_BLACK,
			.col1_v_size = style::NORMAL, .col1_v = style::MicroV::DARKEN_50,
			.col2_v_size = style::SHARP, .col2_v = style::MicroV::HUE_320
		};
	}
}

dvec2 cam;
f32 tzoom = 0., zoom = 0.;
vec2 stars_pos;
u8 paused = 0;
f32 diffta = 0;
f32 moved = 0;
inline void frame(){
	if(mouse.w) tzoom += mouse.w*-.3;
	f32 a = pow(.001, dt);
	zoom = tzoom*(1-a) + zoom*a;
	f32 renderZoom = pow(2, zoom);
	if(!(paused&1) || moved >= 0)
		sim.update(paused&1 ? 0 : dt, t, cam.x, cam.y, window.w*renderZoom*.125, window.h*renderZoom*.125);
	if(pointerLocked){
		if(mouse) moved = 1.5;
		moved -= dt;
		cam -= mouse.xy*(renderZoom*.03);
		Uint8 controls = keys[SDL_SCANCODE_W] | keys[SDL_SCANCODE_S]<<1 | keys[SDL_SCANCODE_A]<<2 | keys[SDL_SCANCODE_D]<<3 | keys[SDL_SCANCODE_EQUALS]<<4 | keys[SDL_SCANCODE_MINUS]<<5;
		if(keys[SDL_SCANCODE_SPACE]){
			if(!(paused&2)) paused ^= 3;
		}else paused &= -3;
		if(controls){
			f32 mv = renderZoom*dt;
			if(controls&1) cam.y += mv;
			if(controls&2) cam.y -= mv;
			if(controls&4) cam.x -= mv;
			if(controls&8) cam.x += mv;
			if(controls&16) tzoom -= dt*2;
			if(controls&32) tzoom += dt*2;
		}
	}
	a = 10./(renderZoom*window.w*window.h);
	f32 w = a*window.h, h = a*window.w;
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(objShader);
	glUniform2f(whUni, w, h);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBindVertexArray(va);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	physics::UpdateResultHandle draw = sim.result();
	if(draw){
		cout << draw->build_time << endl;
		if(!(paused&1)){
			f32 difft = t - draw->t;
			diffta += (difft-diffta)*dt*.2;
			difft -= diffta;
			glUniform1f(difftUni, difft);
		}else draw->t += dt, glUniform1f(difftUni, 0);
		glUniform1f(tUni, f32(i32(u32(t*65536)))*(1./65536.));
		glUniform2f(xyUni, cam.x - draw->cam_x, cam.y - draw->cam_y);
		for(int i = draw->draw_count; i;){
			vector<physics::Sprite>& buf = draw->draw_data[--i];
			usize sz = buf.size();
			glBufferData(GL_ARRAY_BUFFER, sz*sizeof(physics::Sprite), buf.data(), GL_STREAM_DRAW);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, sz);
		}
	}
	glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
	glBindVertexArray(0);
	glUseProgram(starShader);
	glUniform2f(whUniStar, window.w*.005, window.h*.005);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}