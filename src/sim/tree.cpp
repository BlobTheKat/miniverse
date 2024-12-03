#include "node.cpp"

namespace physics{

// 1. Shallow-split into a tree on 1 thread (for 4 threads, splitting into 16 nodes is pretty sufficient)
// 2. Assign each node to a thread, which will then split fully into a formal QuadTree
// 3. Once everyone's done, simulate our thread's node against itself and against others' nodes
// 4. During 3, emit new nodes into a linked-list allocated on a block-chain allocator (Not the crypto one!)

constexpr int BLOCK_SIZE = 1, BLOCK_BYTES = (BLOCK_SIZE<<16) - sizeof(char*);

thread_local char *p_root = 0, *p_old_root = 0;
thread_local char *block = (char*) &p_root, **block_end = &p_root;
void* salloc(int sz){
	char* a = block;
	if((block = a + sz) <= (char*)block_end) return a;
	a = *block_end;
	if(!a) a = *block_end = (char*) page_alloc(BLOCK_SIZE);
	block = a + sz; block_end = (char**) (a + BLOCK_BYTES);
	return a;
}
void free_swap(){
	char* a = *block_end; *block_end = 0;
	while(a){
		char* b = *(char**)(a+BLOCK_BYTES); page_free(a); a = b;
	}
	a = p_old_root; p_old_root = p_root; p_root = a;
	block = (char*) (block_end = &p_root);
}
void free_all(){
	char* a = p_root; p_root = 0;
	while(a){
		char* b = *(char**)(a+BLOCK_BYTES); page_free(a); a = b;
	}
	a = p_old_root; p_old_root = 0;
	while(a){
		char* b = *(char**)(a+BLOCK_BYTES); page_free(a); a = b;
	}
	block = (char*) &p_root; block_end = &p_root;
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
}
#include <emmintrin.h>
namespace physics{
#define _signextractd(x, y) _mm_movemask_pd(_mm_set_pd(y, x))
#define _signextractf(x, y) _mm_movemask_ps(_mm_set_ps(0, 0, y, x))
#else
#define _signextractd(x, y) (int)(bit_cast<u64>(x)>>63)|(int)(bit_cast<u64>(y)>>62)&2
#define _signextractf(x, y) (int)(bit_cast<u32>(x)>>31)|(int)(bit_cast<u32>(y)>>30)&2
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
	f32 dt, i_theta; 
	bool changed = false; a_lock avail = 1;
};

struct UpdateResult{
	usize node_count;
};

struct alignas(hardware_destructive_interference_size) SimData{
	atomic<usize> taskNumber = 0;
	vector<QNode*> tasks;
	int thr_count = 0;
	atomic_flag running;
	a_lock rootLock, stage1 = 1, stage2 = 0, stage3 = 0;
	atomic<char*> roots;
	char* old_roots;
	atomic<int> waiting;
	UpdateParams params[2];
	atomic<usize> node_count;
	atomic<UpdateResult> res;
	f64 prev_size = 128;
};

thread_local SimData* dat;
thread_local UpdateParams* params;

struct QNode{
	constexpr static int LEAF_CAPACITY = 8;
	atomic<Node*> chain = 0;
	union{
		f64 size;
		struct{ f64 cmass_x, cmass_y; };
	};
	union{
		atomic<usize> node_count = 0;
		struct{
			f32 err_radius;
			union{
				f32 mass;
				f32 props[];
			};
		};
	};
	template<bool shallow = false>
	void gather(){
		if constexpr(shallow){
			uptr x = bit_cast<uptr>(chain.load(relaxed));
			if(!(x&1)) return;
			char *a = bit_cast<char*>(x&-2), *a1 = a;
			chain.store((Node*) a, relaxed);
		}else if(isize(node_count.load(relaxed)) >= 0){
			node_count.~atomic();
			Node* ch = chain.load(relaxed);
			if(!ch) return;
			mass = ch->mass;
			Node* n = ch->next;
			f64 cx = 0, cy = 0;
			while(n){ f32 m = n->mass; mass += m; cx += n->x*m; cy += n->y*m; n = n->next; }
			f32 i_mass = 1/mass, f = ch->mass*i_mass, e = 0;
			cmass_x = cx*i_mass; cmass_y = cy*i_mass;
			for(int i = 1; i < prop_count; i++) props[i] = ch->props[i]*f;
			n = ch->next;
			while(n){
				f32 f = n->mass*i_mass, f2 = 1+f*i_theta;
				f32 xd = n->x-cmass_x, yd = n->y-cmass_y;
				e += (xd*xd+yd*yd)*(f2*f2);
				for(int i = 1; i < prop_count; i++)
					props[i] += n->props[i]*f;
				n = n->next;
			}
			err_radius = fast_sqrt(e);
			return;
		}
		node_count.~atomic();
		char *a = (char*) chain.load(relaxed), *a1 = a;
		((QNode*)a1)->gather<shallow>();
		f32 fa = mass = ((QNode*)a1)->mass;
		((QNode*)(a1+=qnode_size))->gather<shallow>();
		f32 fb = ((QNode*)a1)->mass; mass += fb;
		((QNode*)(a1+=qnode_size))->gather<shallow>();
		f32 fc = ((QNode*)a1)->mass; mass += fc;
		((QNode*)(a1+=qnode_size))->gather<shallow>();
		f32 fd = ((QNode*)a1)->mass; mass += fd;
		f32 i_mass = 1/mass; fa *= i_mass; fb *= i_mass; fc *= i_mass; fd *= i_mass;
		a1 = a; f64 cx = (((QNode*)a1)->cmass_x)*fa, cy = (((QNode*)a1)->cmass_y)*fa;
		cx += (((QNode*)(a1+=qnode_size))->cmass_x)*fb; cy += (((QNode*)(a1+=qnode_size))->cmass_y)*fb;
		cx += (((QNode*)(a1+=qnode_size))->cmass_x)*fc; cy += (((QNode*)(a1+=qnode_size))->cmass_y)*fc;
		cx += (((QNode*)(a1+=qnode_size))->cmass_x)*fd; cy += (((QNode*)(a1+=qnode_size))->cmass_y)*fd;
		cmass_x = cx; cmass_y = cy;
		a1 = a; f64 dcx = ((QNode*)a1)->cmass_x-cx, dcy = ((QNode*)a1)->cmass_y-cy;
		f32 e = (dcx*dcx+dcy*dcy)*(1+(2+fa)*fa);
		dcx = ((QNode*)(a1+=qnode_size))->cmass_x-cx; dcy = ((QNode*)a1)->cmass_y-cy;
		e += (dcx*dcx+dcy*dcy)*(1+(2+fb)*fb);
		dcx = ((QNode*)(a1+=qnode_size))->cmass_x-cx; dcy = ((QNode*)a1)->cmass_y-cy;
		e += (dcx*dcx+dcy*dcy)*(1+(2+fc)*fc);
		dcx = ((QNode*)(a1+=qnode_size))->cmass_x-cx; dcy = ((QNode*)a1)->cmass_y-cy;
		e += (dcx*dcx+dcy*dcy)*(1+(2+fd)*fd);
		for(int i = 1; i < prop_count; i++){
			f32 f = ((QNode*)(a1=a))->props[i]*fa;
			f += ((QNode*)(a1+=qnode_size))->props[i]*fb;
			f += ((QNode*)(a1+=qnode_size))->props[i]*fc;
			f += ((QNode*)(a1+=qnode_size))->props[i]*fd;
			props[i] = f;
		}
		err_radius = -fast_sqrt(e);
	}
};
static_assert(alignof(QNode) > 1 && alignof(Node) > 1);
struct QNStack{
	struct QNStackEntry{
		f64 xm, ym;
		u64 count = 0;
		QNode* qn;
	};
	QNStackEntry* d; f64 size = -1;
	usize count = 0;
	size_t left = 0, right = 3;
	QNStack(){
		d = new (malloc(sizeof(QNStackEntry)<<2)) QNStackEntry();
		d->qn = 0; d->xm = d->ym = 0;
	}
	void add(Node* n){
		count++;
		f64 x = n->x, y = n->y;
		find:
		f64 xm = d->xm, ym = d->ym;
		f64 xd = x-xm, yd = y-ym;
		if(max(abs(xd), abs(yd)) <= size){
			found:
			QNode* qn = d->qn;
			// We found our node (maybe?)
			usize a = qn->node_count.fetch_add(1, relaxed);
			if(a < QNode::LEAF_CAPACITY){
				n->next = qn->chain.exchange(n, relaxed);
				return;
			}else if(isize(a) > 0){
				if(a == QNode::LEAF_CAPACITY){
					n->next = qn->chain.load(relaxed);
					// Split
					char* s = (char*) salloc(qnode_size<<2);
					qn->chain.store((Node*) s, relaxed);
					Node *chains[4] = {0,0,0,0}; usize count = SIZE_MAX^SIZE_MAX>>1;
					while(n){
						int i = (n->x>=xm)|(n->y>=ym)<<1; count++;
						n->next = chains[i]; chains[i] = n;
					}
					(new (s) QNode())->chain = chains[0];
					(new (s+=qnode_size) QNode())->chain = chains[1];
					(new (s+=qnode_size) QNode())->chain = chains[2];
					(new (s+=qnode_size) QNode())->chain = chains[3];
					qn->node_count.store(count, release);
					return;
				}
				while(isize(qn->node_count.load(relaxed)) > QNode::LEAF_CAPACITY);
			}
			// Go deeper
			xd = n->x-xm; yd = n->y-ym;
			int i = _signextractd(xd, yd);
			d->count = count-1; size *= .5;
			if(!right){
				right = left; left++;
				d = (QNStackEntry*)realloc(d - right, (left<<1)*sizeof(QNStackEntry)) + left;
			}else right--, d++;
			d->qn = (QNode*)((char*)qn->chain.load(relaxed)+qnode_size*i);
			d->xm = xm += copysign(size, xd);
			d->ym = ym += copysign(size, yd);
			goto found;
		}
		up:
		if(!left){
			int q5 = qnode_size*5, i = _signextractd(x, y)*q5;
			// Check and expand
			char* a = dat->roots.load(relaxed);
			f64 xyd = max(abs(x), abs(y)); size = ((QNode*)a)->size;
			if(xyd <= size*2){ new_root:
				QNode* qnt = d->qn;
				d->qn = (QNode*)(a+i); d->count = count;
				if(qnt){
					QNode *qn = (QNode*)(a+_signextractd(xm, ym)*q5);
					while(qn != qnt){
						qn->node_count.fetch_add(count, relaxed);
						qn = (QNode*)((char*)qn->chain.load(relaxed) + i);
					}
				}
				xm = d->xm = copysign(size, x); ym = d->ym = copysign(size, y);
				goto found;
			}
			dat->rootLock.lock();
			a = dat->roots.load(relaxed);
			size = ((QNode*)a)->size*2;
			for(;;){
				if(xyd <= size){ dat->rootLock.unlock(); size *= .5; goto new_root; }
				char *old = a, *a1 = a = (char*) salloc(qnode_size<<4);
				for(char* b = a+(qnode_size<<4); b > a;) (new (b -= qnode_size) QNode())->size = size;
				((QNode*)a1)->chain = (Node*)old;
				((QNode*)(a1+=q5))->chain = (Node*)(old+=qnode_size<<2);
				((QNode*)(a1+=q5))->chain = (Node*)(old+=qnode_size<<2);
				((QNode*)(a1+=q5))->chain = (Node*)(old+=qnode_size<<2);
				dat->roots.store(a, release);
				size *= 2;
			}
		}
		left--; right++; d--; size *= 2;
		size_t a = count - d->count;
		if(a) d->qn->node_count.fetch_add(a, relaxed);
		goto find;
	}
	void finish(){
		while(left){
			left--; d--; right++;
			size_t a = count - d->count;
			if(a) d->qn->node_count.fetch_add(a, relaxed);			
		}
		if(d->qn){
			int q5 = qnode_size*5, j = _signextractd(d->xm, d->ym)*q5;
			QNode *qn = (QNode*)(dat->roots.load(relaxed) + j);
			while(qn != d->qn){
				qn->node_count.fetch_add(count, relaxed);
				qn = (QNode*)((char*)qn->chain.load(relaxed) + j);
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
	~QNStack(){free(d);}
};

}