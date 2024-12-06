#include "tree.cpp"

namespace physics{

thread_local char *st_base = 0, *_st_base = 0;
thread_local usize left = 0, right = 0, _left = 0, _right = 0;
inline void* st_alloc(usize n){
	n = adv_aligned<alignof(f32), alignof(void*)>(n);
	if(right < n){
		right = left + ((right + n) << 1);
		st_base = (char*) realloc(st_base, left + right);
	}
	right -= n;
	void* b = st_base+left; left += n;
	return b;
}
inline void _trim_alloc(){
	if(right > sizeof(void*)<<8)
		st_base = (char*) realloc(st_base, right>>=1);
	else if(right < sizeof(void*)<<6){
		free(st_base); st_base = (char*) malloc(right = sizeof(void*)<<6);
	}
}
inline void switch_allocs(){
	swap(st_base, _st_base);
	swap(left, _left); swap(right, _right);
}

inline void st_pop(usize n){ right += n; left -= n; }

struct QNodeAggregate{
	f32 dx = 0, dy = 0;
	f32 tidal = 0;
};


// For an object (Δx, Δy) from the attractor, the applied force is
// 1/|(Δx, Δy)| * complex(normalize((Δx, Δy))) * complex(dir)
// (We ignore masses for now, they are trivial to include later)
// Since normalize((Δx, Δy)) = (Δx, Δy) / |(Δx, Δy)| and
// |(Δx, Δy)| = sqrt(Δx^2 + Δy^2), the equation simplifies to
// 1/sqrt(Δx^2 + Δy^2) * complex((Δx, Δy)/sqrt(Δx^2 + Δy^2)) * complex(dir)
// = 1/(Δx^2 + Δy^2) * complex((Δx, Δy)) * complex(dir)
// Note that the two sqrt's cancel out
// Now let's calculate tidal forces (derivative of acceleration)
// δF = (1/sqrt(Δx^2 + Δy^2))'
// And since (1/x)' = 1/x^2
// δF = (1/sqrt(Δx^2 + Δy^2)^2)
// = 1/(Δx^2 + Δy^2) (same as the first term from earlier)
// Congratulations, you now understand the ILA optimization (Inverse-linear attraction)
// tidal_force = 1 / (dx*dx+dy*dy)
// velocity += (dx,dy) * tidal_force
// In this specific case we also support lateral and inversed attraction,
// for which we just multiply by a complex number a+bi, which is the same as the matrix
//   v  v
// [ a -b ] >
// [ b  a ] >
// This number can be thought of as the vector (a,b) of acceleration per second
// at distance (-1, 0) from the attractor (and activators of 1)

template<typename T>
inline void update(QNodeAggregate& agg, Node* self, T* other){
	f32 xd = other->x - self->x, yd = other->y - self->y;
	f32 inv_sq_dist = fast_inverse(other->rad_cap(xd*xd+yd*yd));
	_AttractionRule* r = cur_a_rules;
	while(++r < cur_a_rules_end){
		f32 p = inv_sq_dist * dt;
		if(r->prop) p *= other->props[r->prop];
		if(r->activator) p *= self->props[r->activator];
		agg.tidal += p;
		f32 up = p * r->dir.y, right = p * r->dir.x;
		agg.dx += up*xd+right*yd;
		agg.dy += up*yd-right*xd;
	}
}

inline void finish(Node* self, QNodeAggregate& agg){
	Node* n = self->copy();
	n->x += n->dx += agg.dx;
	n->y += n->dy += agg.dy;
	drawBuf.emplace_back(vec2(n->x - cam_x, n->y - cam_y), n->radius);
	tree->add(n);
}

template<typename T>
inline void update(QNodeAggregate* agg, QNode* self, T* other){
	f32 xd = other->x - self->x, yd = other->y - self->y;
	f32 inv_sq_dist = fast_inverse(other->rad_cap(xd*xd+yd*yd));
	_AttractionRule* r = cur_a_rules;
	while(++r < cur_a_rules_end){
		f32 p = inv_sq_dist * dt;
		QNodeAggregate* a = agg+r->agg_id;
		a->tidal += p;
		if(r->prop) p *= other->props[r->prop];
		f32 up = p * r->dir.y, right = p * r->dir.x;
		a->dx += up*xd+right*yd;
		a->dy += up*yd-right*xd;
	}
}

inline void compile(QNodeAggregate& agg, Node* n, QNodeAggregate* agg1){
	u32* r = cur_aggregates;
	while(++r < cur_aggregates_end){
		u32 id = *r;
		f32 f = id ? n->props[id] : 1;
		agg.dx += agg1->dx*f;
		agg.dy += agg1->dy*f;
		agg.tidal += agg1->tidal*f;
		agg1++;
	}
}

void finish(QNode* self, QNodeAggregate* agg){
	if(!signbit(self->err_radius)){
		Node* n = self->chain;
		while(n){
			QNodeAggregate agg2;
			compile(agg2, n, agg);
			finish(n, agg2);
			n = n->next;
		}
	}else{
		char* a = (char*) self->chain;
		finish((QNode*) a, agg);
		finish((QNode*)(a+=qnode_size), agg);
		finish((QNode*)(a+=qnode_size), agg);
		finish((QNode*)(a+=qnode_size), agg);
	}
}

inline void updateq(QNodeAggregate* agg, QNode* self, Node* other){
	f32 r0 = abs(self->err_radius);
	f32 xd = other->x - self->x, yd = other->y - self->y;
	if(r0*r0 > xd*xd+yd*yd){
		*(QNode**)st_alloc(sizeof(QNode*)) = bit_cast<QNode*>(bit_cast<uptr>(other)|1);
	}else update(agg, self, other);
}

void updateq(QNodeAggregate* agg, QNode* self, QNode* other){
	f32 r0 = abs(self->err_radius), r1 = abs(other->err_radius), r2 = r0+r1;
	f32 xd = other->x - self->x, yd = other->y - self->y;
	if(r2*r2 > xd*xd+yd*yd){
		if(r1 >= r0){ if(signbit(other->err_radius)){
			char* a = (char*) other->chain;
			update(agg, self, (QNode*) a);
			update(agg, self, (QNode*)(a += prev_qnode_size));
			update(agg, self, (QNode*)(a += prev_qnode_size));
			update(agg, self, (QNode*)(a += prev_qnode_size));
		}else{
			Node* n = other->chain;
			while(n){ update(agg, self, n); n = n->next; }
		} }else *(QNode**)st_alloc(sizeof(QNode*)) = other;
	}else update(agg, self, other);
}

void updatev(QNodeAggregate* agg, QNode* self, QNode** other, QNode** end){
	usize list_base = left;
	for(; other < end; other++){
		QNode* n = *other; uptr a = bit_cast<uptr>(n);
		if(a&1) update(agg, self, bit_cast<Node*>(a&-2));
		else update(agg, self, n);
	}
	usize to_pop = list_base - left;
	if(!signbit(self->err_radius)){
		other = (QNode**)(st_base + list_base); end = (QNode**)(st_base + left);
		st_pop(to_pop); // data is still available after pop
		switch_allocs();
		Node* n = self->chain;
		while(n){
			QNode** o = other;
			QNodeAggregate agg2;
			for(; o < end; o++){
				QNode* n2 = *o; uptr a = bit_cast<uptr>(n);
				if(a&1) update(agg2, n, bit_cast<Node*>(a&-2));
				else update(agg2, n, n2);
			}
			compile(agg2, n, agg);
			finish(n, agg2);
			n = n->next;
		}
		return;
	}
	char* a = (char*) self->chain;
	if(other != end){
		usize l2 = left;
		QNodeAggregate* agg2 = (QNodeAggregate*)st_alloc(attr_count*sizeof(QNodeAggregate));
		other = (QNode**)(st_base + list_base); end = (QNode**)(st_base + l2);
		st_pop(to_pop + attr_count*sizeof(QNodeAggregate)); // data is still available after pop
		switch_allocs();
		for(int i = 0; i < attr_count; i++) agg2[i] = agg[i];
		updatev(agg2, (QNode*) a, other, end);
		for(int i = 0; i < attr_count; i++) agg2[i] = agg[i];
		updatev(agg2, (QNode*)(a+=qnode_size), other, end);
		for(int i = 0; i < attr_count; i++) agg2[i] = agg[i];
		updatev(agg2, (QNode*)(a+=qnode_size), other, end);
		updatev(agg, (QNode*)(a+=qnode_size), other, end);
	}else{
		finish((QNode*) a, agg);
		finish((QNode*)(a+=qnode_size), agg);
		finish((QNode*)(a+=qnode_size), agg);
		finish((QNode*)(a+=qnode_size), agg);
	}
}

}