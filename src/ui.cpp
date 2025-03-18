#include "util/short_mat2.cpp"
#include "sdl.cpp"

GLuint uiShader;
GLuint buf, va[3];

namespace ubuntu{
#include "ubuntu.h"
}

constexpr int SIZE_BITS = countl_zero(usize(0));
struct UIBuf{
	f32* buf = 0; usize ref = 1;
	usize len = 0, cap = 0;
	inline void add_v(vec2 xy, vec2 uv, vec4 col){
		f32* a = buf+(len++<<3);
		a[0] = xy.x; a[1] = xy.y; a[2] = uv.x; a[3] = uv.y;
		a[4] = col.x; a[5] = col.y; a[6] = col.z; a[7] = col.w;
	}
	inline void reserve(usize a = 1){
		if(len+a > cap) buf = (f32*) realloc(buf, (cap=max(cap,usize(1)<<(SIZE_BITS-countl_zero(a-1)))<<1)<<5);
	}
	~UIBuf(){ free(buf); }
};

struct UIMesh: mat3x2{
	UIBuf* buf;
	UIMesh(UIMesh& a) : mat3x2(a){ (buf = a.buf)->ref++; }
	UIMesh(UIMesh&& a) : mat3x2(a){ buf = a.buf; a.buf = 0; }
	UIMesh() : mat3x2{2,0,0,2,-1,-1}{ buf = new UIBuf(); }
	void setTo(UIMesh& a){ this->mat3x2::operator=(a); }
	void operator=(UIMesh&) = delete;
	void operator=(UIMesh&&) = delete;
	~UIMesh(){ if(buf && !--buf->ref) delete buf; }
	void draw(){
		glUseProgram(uiShader);
		glBindVertexArray(va[1]);
		glBufferData(GL_ARRAY_BUFFER, buf->len<<5, buf->buf, GL_STREAM_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, buf->len);
	}
	void add_rect(vec2 xy, vec2 wh, vec2 uv, vec2 uvwh, vec4 color, vec4 addX = {0}, vec4 addY = {0}){
		xy = this->transformPoint(xy); wh = this->transformMetric(wh);
		buf->reserve(6);
		buf->add_v(xy, uv, color);
		vec2 xy2 = {xy.x+wh.x,xy.y}, uv2 = {uv.x+uvwh.x,uv.y}; vec4 col2 = color + addX;
		buf->add_v(xy2, uv2, col2);
		xy2.y += wh.y; uv2.y += uvwh.y; col2 += addY;
		buf->add_v(xy2, uv2, col2); buf->add_v(xy2, uv2, col2);
		buf->add_v(xy, uv, color);
		buf->add_v({xy.x,xy2.y}, {uv.x,uv2.y}, color+addY);
	}
	void add_text(const string& s, vec4 col = 1, vec4 gr = 0){
		for(char c : s){
			ubuntu::Glyph& g = ubuntu::get_glyph(c);
			add_rect(g.pos.xy, g.pos.zw-g.pos.xy, g.uv.xy, g.uv.zw-g.uv.xy, col, 0, gr);
			this->translateX(g.advance);
		}
	}
};