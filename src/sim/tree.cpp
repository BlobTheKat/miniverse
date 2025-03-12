#include "draw.cpp"

namespace physics{

// 1. Shallow-split into a tree on 1 thread (for 4 threads, splitting into 16 nodes is pretty sufficient)
// 2. Assign each node to a thread, which will then split fully into a formal QuadTree
// 3. Once everyone's done, simulate our thread's node against itself and against others' nodes
// 4. During 3, emit new nodes into a linked-list allocated on a block-chain allocator (Not the crypto one!)

constexpr int BLOCK_SIZE = 4, BLOCK_BYTES = (BLOCK_SIZE<<16) - sizeof(char*);

thread_local usize mem = 0;
thread_local char *p_root = 0, *p_old_root = 0;
thread_local char *block = (char*) &p_root, **block_end = &p_root;
void* salloc(int sz){
	char* a = block;
	if((block = a + sz) <= (char*)block_end) return a;
	a = *block_end;
	if(!a) a = *block_end = (char*) page_alloc(BLOCK_SIZE), mem += BLOCK_SIZE<<16;
	block = a + sz; block_end = (char**) (a + BLOCK_BYTES);
	return a;
}
void free_swap(){
	char* a = *block_end; *block_end = 0;
	while(a){
		char* b = *(char**)(a+BLOCK_BYTES); page_free(a), mem -= BLOCK_SIZE<<16; a = b;
	}
	a = p_old_root; p_old_root = p_root; p_root = a;
	block = (char*) (block_end = &p_root);
}
void free_all(){
	return;
	char* a = p_root; p_root = 0;
	while(a){
		char* b = *(char**)(a+BLOCK_BYTES); page_free(a); a = b;
	}
	a = p_old_root; p_old_root = 0;
	while(a){
		char* b = *(char**)(a+BLOCK_BYTES); page_free(a); a = b;
	}
	block = (char*) &p_root; block_end = &p_root;
	mem = 0;
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
}
#include <emmintrin.h>
namespace physics{
#define _signextractd(x, y) _mm_movemask_pd(_mm_set_pd(y, x))
#define _signextractf(x, y) _mm_movemask_ps(_mm_set_ps(0, 0, y, x))
#else
#define _signextractd(x, y) ((int)(bit_cast<u64>(x)>>63)|(int)(bit_cast<u64>(y)>>62)&2)
#define _signextractf(x, y) ((int)(bit_cast<u32>(x)>>31)|(int)(bit_cast<u32>(y)>>30)&2)
#endif

#if UINTPTR_MAX == UINT64_MAX
#define POINTER_PACK_BITS 48
#define POINTER_PACK_MASK 0xffffffffffff
#elif UINTPTR_MAX != UINT32_MAX
#error "Can only build for 32 or 64 bit targets"
#endif

struct UpdateParams{
	Node* toAdd;
	char* constant_block = 0;
	int attr_count, arule_count;
	int node_size, qnode_size, prop_count;
	f64 cam_x, cam_y, cam_hw, cam_hh;
	f32 dt, i_theta;
	bool changed = false; a_lock avail = 1;
	struct UpdateResult* res;
};

struct UpdateResult{
	atomic_int ref_count = 1; int draw_count;
	usize node_count;
	f64 cam_x, cam_y, t, build_time;
	usize mem;
	vector<Sprite> draw_data[0];
	UpdateResult(int dc) : draw_count(dc){
		while(dc) new (&draw_data[--dc]) vector<Sprite>();
	}
	~UpdateResult(){
		while(draw_count) draw_data[--draw_count].~vector();
	}
};

struct alignas(hardware_destructive_interference_size) SimData{
	atomic<usize> task_number = 0;
	vector<struct QNode*> tasks;
	int thr_count = 0;
	atomic_flag running; a_lock resLock;
	a_lock rootLock, stage1 = 1, stage2 = 0, stage3 = 0;
	atomic<char*> roots;
	char* old_roots;
	atomic<int> waiting;
	UpdateParams params[2];
	atomic<usize> node_count;
	atomic<usize> mem;
	UpdateResult* latest = 0;
	f64 prev_size = 128;
	chrono::time_point<chrono::high_resolution_clock> start;
};

thread_local SimData* dat;
struct QNode{
	constexpr static int LEAF_CAPACITY = 16;
	constexpr static int DEEP_BIAS = LEAF_CAPACITY+1;
	union{
		char* qchain = 0;
		Node* chain;
		Node** lchain;
	};
	union{
		f64 size;
		struct{ f64 x, y; };
		struct{ f64 cmass_x, cmass_y; };
	};
	union{
		atomic<isize> node_count = 0;
		struct{
			f32 err_radius;
			union{
				f32 mass;
				struct{ f32 props[]; };
			};
		};
	};
	inline f32 rad_cap(f32 sq_dst){return sq_dst;}
	template<bool shallow = false>
	void gather(){
		if constexpr(shallow){
			uptr x = bit_cast<uptr>(chain);
			if(!(x&1)) return;
			if(!(chain = bit_cast<Node*>(x&-2))){
				cmass_x = 0; cmass_y = 0; err_radius = 0;
				for(int i = 0; i < prop_count; i++) props[i] = 0;
				return;
			}
		}else if(node_count.load(relaxed) <= QNode::LEAF_CAPACITY){
			node_count.~atomic();
			if(!chain){ no_chain:
				cmass_x = 0; cmass_y = 0; err_radius = 0;
				for(int i = 0; i < prop_count; i++) props[i] = 0;
				return;
			}
			// linked lists are evil (for branch prediction)
			mass = chain->mass;
			f64 cx = chain->x*mass, cy = chain->y*mass;
			usize count = 2;
			Node* n = chain->next;
			while(n){ f32 m = n->mass; mass += m; cx += n->x*m; cy += n->y*m; n = n->next; count++; }
			f32 i_mass = mass ? 1/mass : 1, e = 0;
			cmass_x = cx *= i_mass; cmass_y = cy *= i_mass;
			for(int i = 1; i < prop_count; i++) props[i] = 0;
			n = chain;
			Node **l = lchain = (Node**) salloc(count*sizeof(Node*));
			*(Node***)(l++) = l+count;
			do{
				*l++ = n;
				f32 f = n->mass*i_mass;
				if(f){
					f32 f2 = 1+f*i_theta;
					f32 xd = n->x-cx, yd = n->y-cy;
					e += (xd*xd+yd*yd)*(f2*f2);
					for(int i = 1; i < prop_count; i++)
						props[i] += n->props[i]*f;
				}
				n = n->next;
			}while(n);
			err_radius = fast_sqrt(e);
			return;
		}
		node_count.~atomic();
		char *a = (char*) chain, *a1 = a;
		((QNode*)a1)->gather<shallow>();
		f32 fa = mass = ((QNode*)a1)->mass;
		((QNode*)(a1+=qnode_size))->gather<shallow>();
		f32 fb = ((QNode*)a1)->mass; mass += fb;
		((QNode*)(a1+=qnode_size))->gather<shallow>();
		f32 fc = ((QNode*)a1)->mass; mass += fc;
		((QNode*)(a1+=qnode_size))->gather<shallow>();
		f32 fd = ((QNode*)a1)->mass; mass += fd;
		f32 i_mass = mass ? 1/mass : 1; fa *= i_mass; fb *= i_mass; fc *= i_mass; fd *= i_mass;
		a1 = a; f64 cx = (((QNode*)a1)->cmass_x)*fa, cy = (((QNode*)a1)->cmass_y)*fa;
		cx += (((QNode*)(a1+=qnode_size))->cmass_x)*fb; cy += (((QNode*)a1)->cmass_y)*fb;
		cx += (((QNode*)(a1+=qnode_size))->cmass_x)*fc; cy += (((QNode*)a1)->cmass_y)*fc;
		cx += (((QNode*)(a1+=qnode_size))->cmass_x)*fd; cy += (((QNode*)a1)->cmass_y)*fd;
		cmass_x = cx; cmass_y = cy;
		for(int i = 1; i < prop_count; i++){
			f32 f = ((QNode*)(a1=a))->props[i]*fa;
			f += ((QNode*)(a1+=qnode_size))->props[i]*fb;
			f += ((QNode*)(a1+=qnode_size))->props[i]*fc;
			f += ((QNode*)(a1+=qnode_size))->props[i]*fd;
			props[i] = f;
		}
		a1 = a; f64 dcx = ((QNode*)a1)->cmass_x-cx, dcy = ((QNode*)a1)->cmass_y-cy;
		f32 e = ((QNode*)a1)->mass ? fast_sqrt(dcx*dcx+dcy*dcy)*i_theta+abs(((QNode*)a1)->err_radius) : 0;
		dcx = ((QNode*)(a1+=qnode_size))->cmass_x-cx; dcy = ((QNode*)a1)->cmass_y-cy;
		if(((QNode*)a1)->mass) e = max(e, fast_sqrt(dcx*dcx+dcy*dcy)*i_theta+abs(((QNode*)a1)->err_radius));
		dcx = ((QNode*)(a1+=qnode_size))->cmass_x-cx; dcy = ((QNode*)a1)->cmass_y-cy;
		if(((QNode*)a1)->mass) e = max(e, fast_sqrt(dcx*dcx+dcy*dcy)*i_theta+abs(((QNode*)a1)->err_radius));
		dcx = ((QNode*)(a1+=qnode_size))->cmass_x-cx; dcy = ((QNode*)a1)->cmass_y-cy;
		if(((QNode*)a1)->mass) e = max(e, fast_sqrt(dcx*dcx+dcy*dcy)*i_theta+abs(((QNode*)a1)->err_radius));
		err_radius = -e;
	}
};
static_assert(alignof(QNode) > 1 && alignof(Node) > 1);
struct QNStack{
	constexpr static isize LOCK = SIZE_MAX^SIZE_MAX>>1;
	struct QNStackEntry{
		f64 xm, ym;
		union{ u64 count; u8 unverified; };
		QNode* qn;
	};
	f64 cam_x, cam_y, cam_hw, cam_hh;
	vector<Sprite> drawBuf;
	QNStackEntry* d; f64 size = -1;
	usize count = 0;
	size_t left = 0, right = 3;
	QNStack(){
		d = (QNStackEntry*) malloc(sizeof(QNStackEntry)<<2);
		d->qn = 0; d->xm = d->ym = 0; d->unverified = 1;
	}
	inline void add(Node* n){ add(n, n->dx, n->dy); }
	void add(Node* n, f32 dx, f32 dy){
		count++;
		f64 x = n->x, y = n->y;
		if(!isfinite(x)) x = n->x = n->dx = dx = 0;
		if(!isfinite(y)) y = n->y = n->dy = dy = 0;
		u32 s = n->style.y&15;
		f32 r = n->radius*f32((4|s&3)<<(s>>2))*.25, xd1 = n->x - cam_x, yd1 = n->y - cam_y;
		if(abs(xd1)-abs(dx)-r < cam_hw && abs(yd1)-abs(dy)-r < cam_hh) drawBuf.emplace_back(vec2(xd1, yd1), vec2(n->dx, n->dy), r, n->style);
		find:
		f64 xm = d->xm, ym = d->ym;
		f64 xd = x-xm, yd = y-ym;
		if(max(abs(xd), abs(yd)) <= size){
			found:
			QNode* qn = d->qn;
			usize a;
			// We found our node (maybe?)
			if(d->unverified){
				found2:
				if(qn->node_count.load(acquire) > QNode::LEAF_CAPACITY) goto deeper;
				else d->unverified = 0;
			}
			a = qn->node_count.exchange(LOCK, acquire);
			if(a == LOCK) while((a = qn->node_count.exchange(LOCK, acquire)) == LOCK);
			if(a < QNode::LEAF_CAPACITY){
				n->next = qn->chain; qn->chain = n;
				qn->node_count.store(a+1, release);
				return;
			}else if(a == QNode::LEAF_CAPACITY){
				n->next = qn->chain;
				// Split
				char* s = (char*) salloc(qnode_size<<2);
				qn->qchain = s;
				Node *chains[4] = {0,0,0,0}; isize count = QNode::DEEP_BIAS;
				while(n){
					int i = (n->x>=xm)|(n->y>=ym)<<1; count++;
					Node* n2 = n->next;
					n->next = chains[i]; chains[i] = n;
					n = n2;
				}
				(new (s) QNode())->chain = chains[0];
				(new (s+=qnode_size) QNode())->chain = chains[1];
				(new (s+=qnode_size) QNode())->chain = chains[2];
				(new (s+=qnode_size) QNode())->chain = chains[3];
				qn->node_count.store(count, release);
				return;
			}
			qn->node_count.fetch_add(a-LOCK, release);
			deeper:
			int i = 3-_signextractd(xd, yd);
			d->count = count-1; size *= .5;
			if(!right){
				right = left; left++;
				d = (QNStackEntry*) realloc(d - right, (left<<1)*sizeof(QNStackEntry)) + left;
			}else right--, d++, left++;
			qn = d->qn = (QNode*)(qn->qchain+i*qnode_size);
			d->xm = xm += copysign(size, xd);
			d->ym = ym += copysign(size, yd);
			xd = x-xm; yd = y-ym;
			goto found2;
		}
		up:
		if(!left){
			int q5 = qnode_size*5, i = _signextractd(x, y)*q5;
			// Check and expand
			char* a = dat->roots.load(acquire);
			f64 xyd = max(abs(x), abs(y)); size = ((QNode*)a)->size;
			if(xyd <= size*2){ new_root:
				QNode* qnt = d->qn;
				d->qn = (QNode*)(a+i); d->unverified = 1;
				if(qnt){
					i = _signextractd(xm, ym)*qnode_size;
					QNode *qn = (QNode*)(a+i*5);
					while(qn != qnt){
						qn->node_count.fetch_add(count, relaxed);
						qn = (QNode*)(qn->qchain + i);
					}
				}
				xm = d->xm = copysign(size, x); ym = d->ym = copysign(size, y);
				xd = x-xm; yd = y-ym;
				goto found;
			}
			dat->rootLock.lock();
			a = dat->roots.load(acquire);
			size = ((QNode*)a)->size*2;
			for(;;){
				if(xyd <= size){ dat->rootLock.unlock(); size *= .5; goto new_root; }
				char *old = a, *a1 = a = (char*) salloc(qnode_size<<4);
				for(char* b = a+(qnode_size<<4); b > a;) (new (b -= qnode_size) QNode())->size = size;
				((QNode*)a1)->qchain = old;
				((QNode*)a1)->node_count.store(QNode::DEEP_BIAS, relaxed);
				((QNode*)(a1+=q5))->qchain = old+=qnode_size<<2;
				((QNode*)a1)->node_count.store(QNode::DEEP_BIAS, relaxed);
				((QNode*)(a1+=q5))->qchain = old+=qnode_size<<2;
				((QNode*)a1)->node_count.store(QNode::DEEP_BIAS, relaxed);
				((QNode*)(a1+=q5))->qchain = old+=qnode_size<<2;
				((QNode*)a1)->node_count.store(QNode::DEEP_BIAS, relaxed);
				dat->roots.store(a, release);
				size *= 2;
			}
		}
		left--; right++; d--; size *= 2;
		size_t a = count - d->count;
		if(a) d->qn->node_count.fetch_add(a, relaxed);
		d->unverified = 1;
		goto find;
	}
	void finish(){
		while(left){
			left--; d--; right++;
			size_t a = count - d->count;
			if(a) d->qn->node_count.fetch_add(a, relaxed);
		}
		if(d->qn){
			int i = _signextractd(d->xm, d->ym)*qnode_size;
			QNode *qn = (QNode*)(dat->roots.load(acquire) + i*5);
			while(qn != d->qn){
				qn->node_count.fetch_add(count, relaxed);
				qn = (QNode*)(qn->qchain + i);
			}
			d->qn = 0;
		}
		dat->node_count.fetch_add(count, relaxed);
		size = -1; count = 0;
		if(right > 16){
			right >>= 1;
			d = (QNStackEntry*) realloc(d, (right+1)*sizeof(QNStackEntry));
		}
	}
	~QNStack(){free(d-left);}
};

thread_local QNStack* tree;

inline Node* Node::copy(){
	Node* n = new (salloc(node_size)) Node(*this);
	for(int i=1;i<prop_count;i++) n->props[i] = this->props[i];
	return n;
}
inline Node* Node::create(){ return new (salloc(node_size)) Node(); }

}