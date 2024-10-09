struct Scene{
	usize vertexCount, indexCount;
	const vec3* vertices;
	const u16* indices;
	template<usize L1, usize L2> Scene(const vec3 (&a)[L1], const u16 (&b)[L2]){
		this->vertexCount = L1; this->vertices = a;
		this->indexCount = L2; this->indices = b;
	}
	void upload() const {
		glBufferData(GL_ARRAY_BUFFER, this->vertexCount*12, this->vertices, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indexCount<<1, this->indices, GL_STATIC_DRAW);
	}
	inline void draw(GLenum mode = GL_TRIANGLE_STRIP) const {
		glDrawElements(mode, this->indexCount, GL_UNSIGNED_SHORT, 0);
	}
};

const vec3 vData[] = {vec3(-3,0,-3),vec3(-3,0,3),vec3(3,0,-3),vec3(3,0,3),vec3(-3,2,-3),vec3(-3,2,3),vec3(3,2,-3),vec3(3,2,3)};
const u16 iData[] = {0, 1, 2, 3, 6, 7, 4, 5, -1, 2, 6, 0, 4, 1, 5, 3, 7};
const Scene mainScene(vData, iData);