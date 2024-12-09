#include "tree.cpp"

namespace physics{

struct alignas(QNode*) QNodeAggregate{
	f32 dx = 0, dy = 0;
	f32 tidal = 0;
};

thread_local char *st_base = 0;
thread_local usize left = 0, right = 0;
inline usize st_alloc(usize n){
	if(right < n){
		right = left + ((right + n) << 1);
		st_base = (char*) realloc(st_base, left + right);
	}
	right -= n;
	usize b = left; left += n;
	return b;
}
inline void st_pop(usize n){ right += n; left -= n; }


// For an object (Δx, Δy) from the attractor, the applied force is
// 1/|(Δx, Δy)| * complex(normalize((Δx, Δy)))
// (We ignore masses for now, they are trivial to include later)
// Since normalize((Δx, Δy)) = (Δx, Δy) / |(Δx, Δy)| and
// |(Δx, Δy)| = sqrt(Δx^2 + Δy^2), the equation simplifies to
// 1/sqrt(Δx^2 + Δy^2) * complex((Δx, Δy)/sqrt(Δx^2 + Δy^2))
// = 1/(Δx^2 + Δy^2) * complex((Δx, Δy))
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
// This number can be thought of as the vector (b,a) of acceleration per second
// at distance (0, -1) from the attractor (and activators of 1)

template<typename T>
inline void update(QNodeAggregate& agg, Node* self, T* other){
	if constexpr(is_same<T, Node>::value){ if(self == other) return; }
	f32 xd = other->x - self->x, yd = other->y - self->y, dst = xd*xd+yd*yd;
	if(!isfinite(dst)) return;
	f32 inv_sq_dist = 1/other->rad_cap(dst);
	_AttractionRule* r = cur_a_rules;
	while(r < cur_a_rules_end){
		f32 p = inv_sq_dist * dt * other->props[r->prop];
		if(r->activator) p *= self->props[r->activator];
		agg.tidal += p;
		f32 up = p * r->dir.y, right = p * r->dir.x;
		agg.dx += up*xd+right*yd;
		agg.dy += up*yd-right*xd;
		r++;
	}
}

inline void finish(Node* self, QNodeAggregate& agg){
	Node* n = self->copy();
	f32 dx = n->dx, dy = n->dy;
	// Leapfrog!
	n->x += (dx = (dx + (n->dx = dx + agg.dx)) * dt) * .5;
	n->y += (dy = (dy + (n->dy = dy + agg.dy)) * dt) * .5;
	tree->add(n, dx, dy);
}

template<typename T>
inline void update(usize agg, QNode* self, T* other){
	f32 xd = other->x - self->x, yd = other->y - self->y, dst = xd*xd+yd*yd;
	if(!isfinite(dst)) return;
	f32 inv_sq_dist = 1/other->rad_cap(dst);
	_AttractionRule* r = cur_a_rules;
	QNodeAggregate* aggf = (QNodeAggregate*)(st_base+agg);
	while(r < cur_a_rules_end){
		f32 p = inv_sq_dist * dt * other->props[r->prop];
		QNodeAggregate& a = aggf[r->agg_id];
		a.tidal += p;
		f32 up = p * r->dir.y, right = p * r->dir.x;
		a.dx += up*xd+right*yd;
		a.dy += up*yd-right*xd;
		r++;
	}
}

inline void compile(QNodeAggregate& agg, Node* n, QNodeAggregate* agg1){
	u32* r = cur_aggregates;
	while(r < cur_aggregates_end){
		u32 id = *r;
		f32 f = id ? n->props[id] : 1;
		agg.dx += agg1->dx*f;
		agg.dy += agg1->dy*f;
		agg.tidal += agg1->tidal*f;
		agg1++; r++;
	}
}

void finish(QNode* self, QNodeAggregate* agg){
	if(!signbit(self->err_radius)){
		Node** n = self->lchain;
		if(!n) return;
		Node** end = (Node**)(*n++);
		while(n < end){
			Node* n1 = *n++;
			QNodeAggregate agg2;
			compile(agg2, n1, agg);
			finish(n1, agg2);
		}
	}else{
		char* a = self->qchain;
		finish((QNode*) a, agg);
		finish((QNode*)(a+=prev_qnode_size), agg);
		finish((QNode*)(a+=prev_qnode_size), agg);
		finish((QNode*)(a+=prev_qnode_size), agg);
	}
}

inline void updateq(usize agg, QNode* self, Node* other){
	f32 r0 = abs(self->err_radius);
	f32 xd = other->x - self->x, yd = other->y - self->y;
	if(r0*r0 >= xd*xd+yd*yd){
		usize i = st_alloc(sizeof(QNode*));
		*(QNode**)(st_base+i) = bit_cast<QNode*>(bit_cast<uptr>(other)|1);
	}else update(agg, self, other);
}

void updateq(QNodeAggregate& agg, Node* self, QNode* other){
	f32 r0 = abs(other->err_radius);
	f32 xd = other->x - self->x, yd = other->y - self->y;
	if(r0*r0 >= xd*xd+yd*yd){
		if(signbit(other->err_radius)){
			char* a = other->qchain;
			if(((QNode*)a)->chain) updateq(agg, self, (QNode*) a);
			if(((QNode*)(a+=prev_qnode_size))->chain) updateq(agg, self, (QNode*) a);
			if(((QNode*)(a+=prev_qnode_size))->chain) updateq(agg, self, (QNode*) a);
			if(((QNode*)(a+=prev_qnode_size))->chain) updateq(agg, self, (QNode*) a);
		}else{
			Node** n = other->lchain;
			Node** end = (Node**)*n++;
			while(n < end){ Node* n1 = *n++; update(agg, self, n1); }
		}
	}else update(agg, self, other);
}

void updateq(usize agg, QNode* self, QNode* other){
	f32 r0 = abs(self->err_radius), r1 = abs(other->err_radius), r2 = r0+r1;
	f32 xd = other->x - self->x, yd = other->y - self->y;
	if(r2*r2 >= xd*xd+yd*yd){
		if(r1 >= r0){ if(signbit(other->err_radius)){
			char* a = other->qchain;
			if(((QNode*)a)->chain) updateq(agg, self, (QNode*) a);
			if(((QNode*)(a+=prev_qnode_size))->chain) updateq(agg, self, (QNode*) a);
			if(((QNode*)(a+=prev_qnode_size))->chain) updateq(agg, self, (QNode*) a);
			if(((QNode*)(a+=prev_qnode_size))->chain) updateq(agg, self, (QNode*) a);
		}else{
			Node** n = other->lchain;
			if(n){
				Node** end = (Node**)*n++;
				while(n < end){ Node* n1 = *n++; updateq(agg, self, n1); }
			}
		} }else{
			usize i = st_alloc(sizeof(QNode*));
			*(QNode**)(st_base+i) = other;
		}
	}else update(agg, self, other);
}

void updatev(usize agg, QNode* self, usize other, usize end){
	usize list_base = left;
	do{
		QNode* n = *(QNode**)(st_base + other); uptr a = bit_cast<uptr>(n);
		other += sizeof(QNode*);
		if(a&1) updateq(agg, self, bit_cast<Node*>(a&-2));
		else updateq(agg, self, n);
	}while(other < end);
	if(!signbit(self->err_radius)){
		QNode **other = (QNode**)(st_base + list_base), **end = (QNode**)(st_base + left);
		Node** n = self->lchain;
		Node** nend = (Node**)*n++;
		QNodeAggregate* aggf = (QNodeAggregate*)(st_base+agg);
		while(n < nend){
			Node* n1 = *n++;
			QNode** o = other;
			QNodeAggregate agg2;
			while(o < end){
				QNode* n2 = *o++; uptr a = bit_cast<uptr>(n2);
				if(a&1) update(agg2, n1, bit_cast<Node*>(a&-2));
				else if(n2->chain) updateq(agg2, n1, n2);
			}
			compile(agg2, n1, aggf);
			finish(n1, agg2);
		}
		st_pop(left - list_base);
		return;
	}
	char* a = self->qchain;
	if(left != list_base){
		usize list_end = left;
		usize agg2 = st_alloc(attr_block_size);
		QNodeAggregate *aggf = (QNodeAggregate*)(st_base+agg), *agg2f = (QNodeAggregate*)(st_base+agg2);
		if(((QNode*)a)->chain){
			for(int i = 0; i < attr_count; i++) agg2f[i] = aggf[i];
			updatev(agg2, (QNode*) a, list_base, list_end);
			aggf = (QNodeAggregate*)(st_base+agg); agg2f = (QNodeAggregate*)(st_base+agg2);
		}
		if(((QNode*)(a+=prev_qnode_size))->chain){
			for(int i = 0; i < attr_count; i++) agg2f[i] = aggf[i];
			updatev(agg2, (QNode*) a, list_base, list_end);
			aggf = (QNodeAggregate*)(st_base+agg); agg2f = (QNodeAggregate*)(st_base+agg2);
		}
		if(((QNode*)(a+=prev_qnode_size))->chain){
			for(int i = 0; i < attr_count; i++) agg2f[i] = aggf[i];
			updatev(agg2, (QNode*) a, list_base, list_end);
		}
		st_pop(attr_block_size);
		if(((QNode*)(a+=prev_qnode_size))->chain)
			updatev(agg, (QNode*) a, list_base, list_end);
	}else{
		QNodeAggregate* aggf = (QNodeAggregate*)(st_base+agg);
		finish((QNode*) a, aggf);
		finish((QNode*)(a+=prev_qnode_size), aggf);
		finish((QNode*)(a+=prev_qnode_size), aggf);
		finish((QNode*)(a+=prev_qnode_size), aggf);
	}
	st_pop(left - list_base);
}

}