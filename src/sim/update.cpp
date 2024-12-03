#include "tree.cpp"

namespace physics{

thread_local char *st_base = 0, *_st_base = 0;
thread_local usize left = 0, right = 0, _left = 0, _right = 0;
inline usize st_alloc(usize n){
	n = adv_aligned<alignof(f32), alignof(void*)>(n);
	if(right < n){
		right = left + ((right + n) << 1);
		st_base = (char*) realloc(st_base, left + right);
	}
	right -= n;
	usize b = left; left += n;
	return b;
}
inline void _trim_alloc(){
	if(right > sizeof(usize)<<8)
		st_base = (char*) realloc(st_base, right>>=1);
	else if(right < sizeof(usize)<<6){
		free(st_base); st_base = (char*) realloc(st_base, right = sizeof(usize)<<6);
	}
}
inline void switch_allocs(){
	swap(st_base, _st_base);
	swap(left, _left); swap(right, _right);
}

inline void st_pop(usize n){ right += n; left -= n; }


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

inline void update(Node* self, Node* other){
	f32 xd = other->x - self->x, yd = other->y - self->y, rad = other->radius;
	f32 inv_sq_dist = fast_inverse(max(rad*rad, xd*xd+yd*yd));
	_AttractionRule* r = cur_a_rules;
	f32* props = other->props;
	while(++r < cur_a_rules_end){
		f32 p = *props * inv_sq_dist * dt;
		self->tidal_force = f32(self->tidal_force) + p;
		if(r->prop) p *= props[r->prop];
		f32 up = p * r->dir.y, right = p * r->dir.x;
		self->dx += up*xd+right*yd;
		self->dy += up*yd-right*xd;
	}
}

void update(Node* self, QNode* other){
	// TODO
}

inline void apply_agg(f32* agg, Node* self){
	// TODO
}

void apply_agg(f32* agg, QNode* self){
	if(bit_cast<i32>(self->err_radius)>=0){
		Node* n = self->chain.load(relaxed);
		while(n){ apply_agg(agg, n); n = n->next; }
	}else{
		char* a = (char*) self->chain.load(relaxed);
		apply_agg(agg, (QNode*) a);
		apply_agg(agg, (QNode*)(a+=qnode_size));
		apply_agg(agg, (QNode*)(a+=qnode_size));
		apply_agg(agg, (QNode*)(a+=qnode_size));
	}
}

inline void update(f32* agg, QNode* self, Node* other){
	f32 r0 = abs(self->err_radius);
	f32 xd = other->x - self->cmass_x, yd = other->y - self->cmass_y;
	if(r0*r0 > xd*xd+yd*yd){
		*(QNode**)(st_base + st_alloc(sizeof(QNode*))) = bit_cast<QNode*>(bit_cast<uptr>(other)|1);
		return;
	}
	// TODO
}

void updatev(f32* agg, QNode* self, QNode** other, QNode** end){
	usize list_base = left;
	for(; other < end; other++){
		QNode* n = *other; uptr a = bit_cast<uptr>(n);
		if(a&1) update(agg, self, bit_cast<Node*>(a&-2));
		else update(agg, self, n);
	}
	usize to_pop = list_base - left;
	if(bit_cast<i32>(self->err_radius)>=0){
		other = (QNode**)(st_base + list_base); end = (QNode**)(st_base + left);
		st_pop(to_pop); // data is still available after pop
		switch_allocs();
		Node* n = self->chain.load(relaxed);
		while(n){
			apply_agg(agg, n);
			QNode** o = other;
			for(; o < end; o++){
				QNode* n2 = *o; uptr a = bit_cast<uptr>(n);
				if(a&1) update(n, bit_cast<Node*>(a&-2));
				else update(n, n2);
			}
			n = n->next;
		}
		return;
	}
	char* a = (char*) self->chain.load(relaxed);
	if(other != end){
		usize l2 = left;
		f32* agg2 = (f32*)(st_base + st_alloc(attr_floats*sizeof(f32)));
		other = (QNode**)(st_base + list_base); end = (QNode**)(st_base + l2);
		st_pop(to_pop + attr_floats*sizeof(f32)); // data is still available after pop
		switch_allocs();
		for(int i = 0; i < attr_floats; i += 3) agg2[i] = agg[i], agg2[i+1] = agg[i+1], agg2[i+2] = agg[i+2];
		updatev(agg2, (QNode*) a, other, end);
		for(int i = 0; i < attr_floats; i += 3) agg2[i] = agg[i], agg2[i+1] = agg[i+1], agg2[i+2] = agg[i+2];
		updatev(agg2, (QNode*)(a+=qnode_size), other, end);
		for(int i = 0; i < attr_floats; i += 3) agg2[i] = agg[i], agg2[i+1] = agg[i+1], agg2[i+2] = agg[i+2];
		updatev(agg2, (QNode*)(a+=qnode_size), other, end);
		updatev(agg, (QNode*)(a+=qnode_size), other, end);
	}else{
		apply_agg(agg, (QNode*) a);
		apply_agg(agg, (QNode*)(a+=qnode_size));
		apply_agg(agg, (QNode*)(a+=qnode_size));
		apply_agg(agg, (QNode*)(a+=qnode_size));
	}
}

void update(f32* agg, QNode* self, QNode* other){
	f32 r0 = abs(self->err_radius), r1 = abs(other->err_radius), r2 = r0+r1;
	f32 xd = other->cmass_x - self->cmass_x, yd = other->cmass_y - self->cmass_y;
	if(r2*r2 > xd*xd+yd*yd){
		if(r1 >= r0){ if(bit_cast<i32>(other->err_radius)<0){
			char* a = (char*) other->chain.load(relaxed);
			update(agg, self, (QNode*) a);
			update(agg, self, (QNode*)(a += prev_qnode_size));
			update(agg, self, (QNode*)(a += prev_qnode_size));
			update(agg, self, (QNode*)(a += prev_qnode_size));
		}else{
			Node* n = other->chain.load(relaxed);
			while(n){ update(agg, self, n); n = n->next; }
		} }else *(QNode**)(st_base + st_alloc(sizeof(QNode*))) = other;
		return;
	}
	// TODO
}

void update_self(f32* agg, QNode* self){

}

void update_self(f32* agg, Node* self){

}

}