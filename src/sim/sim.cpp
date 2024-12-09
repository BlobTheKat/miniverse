#include "update.cpp"
#include <iostream>

namespace physics{

void work(struct SimData&, int);
struct UpdateResultHandle{
	UpdateResult* ref;
	operator bool(){return ref!=0;}
	bool operator!(){return ref==0;}
	UpdateResultHandle() = delete;
	UpdateResultHandle(UpdateResultHandle&) = delete;
	UpdateResultHandle(UpdateResultHandle&&) = delete;
	UpdateResultHandle(UpdateResult* a) : ref(a){}
	UpdateResultHandle& operator=(UpdateResultHandle) = delete;
	UpdateResultHandle& operator=(UpdateResultHandle&) = delete;
	UpdateResultHandle& operator=(UpdateResultHandle&&) = delete;
	UpdateResult& operator*(){ return *ref; }
	UpdateResult* operator->(){ return ref; }
	~UpdateResultHandle(){ if(ref && !--ref->ref_count){ ref->~UpdateResult(); free(ref); } }
};
struct Simulation{
	vector<string> prop_names;
	vector<AttractionRule> a_rules;
	vector<u32> a_ids;
	char* constant_block = 0;
	int attr_count = -1; f32 _dt = 0;
	thread* threads = 0;
	usize tick_num = 0;
	Node* toAdd = 0;
	f32 theta = .25;
	SimData dat;
	int thread_count(){return dat.thr_count;}
	Simulation(int c = thread::hardware_concurrency()){
		if(c <= 0) c = 4;
		dat.thr_count = c;
		dat.running.test_and_set();
		threads = (thread*) malloc(sizeof(thread)*c);
		while(c--) new (threads+c) thread(work, ref(dat), c<<1);
	}
	void update_rules(UpdateParams* p){
		p->changed = true;
		u32* a_r_ids = (u32*) calloc(prop_count = prop_names.size()+1, sizeof(u32));
		attr_count = 0; vector<u32> a_ids;
		for(auto i = a_rules.begin(); i != a_rules.end(); i++){
			u32 ac = i->activator;
			if(a_r_ids[ac] == 0){
				a_r_ids[ac] = ++attr_count;
				a_ids.push_back(ac);
			}
		}
		usize size1 = adv_aligned<alignof(_AttractionRule)>(attr_count * sizeof(u32)), sz2 = a_rules.size();
		usize size2 = adv_aligned<alignof(Node)>(size1 + sz2 * sizeof(_AttractionRule));
		p->constant_block = constant_block = (char*) malloc(size2);
		u32* attr = (u32*) (constant_block);
		for(usize i = 0; i < attr_count; i++) attr[i] = a_ids[i];
		_AttractionRule* _a_rules = (_AttractionRule*) (constant_block + size1);
		for(usize i = 0; i < sz2; i++) _a_rules[i] = a_rules[i].convert(a_r_ids[i]-1);
		free(a_r_ids);
		p->prop_count = prop_count; p->attr_count = attr_count;
		p->arule_count = a_rules.size();
		p->node_size = adv_aligned<alignof(Node)>(offsetof(Node, props) + sizeof(f32) * prop_count);
		p->qnode_size = adv_aligned<alignof(QNode)>(offsetof(QNode, props) + sizeof(f32) * prop_count);
		p->i_theta = 1/theta;
	}
	template<typename... T>
	inline void add_node(T... a){
		Node* n = new Node(a...);
		n->next = toAdd; toAdd = n;
	}
	void add_prop(sstring&& name){
		prop_names.emplace_back(name);
		attr_count = -1;
	}
	void add_attraction_rule(AttractionRule&& a){
		a_rules.emplace_back(a);
		attr_count = -1;
	}
	void update(f32 dt, f64 t, f64 cam_x, f64 cam_y, f64 cam_hw, f64 cam_hh){
		UpdateParams* p = dat.params+(tick_num&1);
		if(!p->avail.is_locked()){ _dt += dt; return; }
		tick_num++;
		if(p->constant_block && p->constant_block != constant_block) free(p->constant_block);
		if(attr_count < 0) update_rules(p);
		else p->changed = false;
		p->toAdd = toAdd; toAdd = 0;
		p->dt = dt + _dt; _dt = 0;
		p->res = new (malloc(sizeof(UpdateResult) + dat.thr_count*sizeof(vector<Sprite>))) UpdateResult(dat.thr_count);
		p->res->cam_x = cam_x; p->res->cam_y = cam_y; p->res->t = t;
		p->cam_x = cam_x; p->cam_y = cam_y;
		p->cam_hw = cam_hw; p->cam_hh = cam_hh;
		p->avail.unlock();
	}
	UpdateResultHandle result(){
		dat.resLock.lock();
		UpdateResult* r = dat.latest;
		if(r) r->ref_count++;
		dat.resLock.unlock();
		return r;
	}
	~Simulation(){
		dat.running.clear();
		UpdateParams* p = dat.params+(tick_num&1);
		p->avail.unlock();
		int c = dat.thr_count;
		while(c--){
			threads[c].join();
			threads[c].~thread();
		}
		free(threads);
		free(dat.params[0].constant_block);
		if(dat.params[1].constant_block != dat.params[0].constant_block)
			free(dat.params[1].constant_block);
	}
};
void add(QNode* qn, usize size_limit){
	char* a = qn->qchain;
	if(qn->node_count.load(relaxed) > size_limit){
		f64 sz = qn->size*.5;
		qn->qchain = bit_cast<char*>(bit_cast<uptr>(a)|1);
		((QNode*)a)->size = sz; add((QNode*)a, size_limit); 
		((QNode*)(a+=qnode_size))->size = sz; add((QNode*)a, size_limit);
		((QNode*)(a+=qnode_size))->size = sz; add((QNode*)a, size_limit);
		((QNode*)(a+=qnode_size))->size = sz; add((QNode*)a, size_limit);
	}else if(a) dat->tasks.push_back(qn);
	else qn->qchain = bit_cast<char*>(uptr(1));
}
constexpr usize SMALLEST_MAX_TASK_SIZE = 100000;

void print_tree(QNode* a, int ind){
	if(ind > 2) return;
	string i = string(ind<<1, ' ');
	if(!a) return void(cout << i << "null\n");
	cout << i << "\x1b[32mQNode(" << a->cmass_x << ", " << a->cmass_y << ") ~ " << abs(a->err_radius) << ", mass=" << a->mass << "\x1b[m\n";
	if(signbit(a->err_radius)){
		char* x = a->qchain;
		print_tree((QNode*)x, ++ind);
		print_tree((QNode*)(x+=prev_qnode_size), ind);
		print_tree((QNode*)(x+=prev_qnode_size), ind);
		print_tree((QNode*)(x+=prev_qnode_size), ind);
	}else{
		i += "  ";
		Node** n = a->lchain;
		if(n){
			Node** end = (Node**)(*n++);
			while(n < end){
				Node* n1 = *n++;
				cout << i << "- (" << n1->x << ", " << n1->y << "), mass=" << n1->mass << "\n";
			}
		}
	}
}

void work(SimData& dat, int a){
	physics::dat = &dat;
	QNStack st; tree = &st;
	UpdateParams* params = dat.params; a |= 1;
	if(!(a>>1)) params->avail.wait();
	else dat.stage1.wait();
	while(1){
		if(!dat.running.test()){ if(!(a>>1)) dat.stage1.unlock_all(); break; }
		prev_node_size = node_size; prev_qnode_size = qnode_size;
		prev_prop_count = prop_count;
		if(params->changed){
			i_theta = params->i_theta;
			cur_aggregates = (u32*) (params->constant_block);
			cur_aggregates_end = cur_aggregates + params->attr_count;
			cur_a_rules = (_AttractionRule*) cur_aggregates_end;
			cur_a_rules_end = cur_a_rules + params->arule_count;
			node_size = params->node_size; qnode_size = params->qnode_size;
			prop_count = params->prop_count; attr_count = params->attr_count;
			attr_block_size = adv_aligned<alignof(QNode*)>(attr_count * sizeof(QNodeAggregate));
		}
		st.cam_x = params->cam_x; st.cam_y = params->cam_y;
		st.cam_hw = params->cam_hw; st.cam_hh = params->cam_hh;
		if(!(a>>1)){
			dat.start = chrono::high_resolution_clock::now();
			dat.waiting = dat.thr_count;
			dat.task_number = 0;
			dat.old_roots = dat.roots.load(relaxed);
#ifdef PRINT_TREE
			char* x = dat.old_roots;
			print_tree((QNode*)x, 0);
			print_tree((QNode*)(x+=prev_qnode_size*5), 0);
			print_tree((QNode*)(x+=prev_qnode_size*5), 0);
			print_tree((QNode*)(x+=prev_qnode_size*5), 0);
			cout << "\n\x1b[31m================\n\n\x1b[m";
#endif
			char* a = (char*) salloc(qnode_size<<4);
			f64 sz = max(1., dat.prev_size*.5);
			for(char* b = a+(qnode_size<<4); b > a;) (new (b -= qnode_size) QNode())->size = sz;
			dat.roots.store(a, relaxed);
			dat.stage2.lock();
			dat.stage1.unlock_all();
		}
		char* r = dat.old_roots; usize q5 = prev_qnode_size*5; f64 sz = dat.prev_size;
		dt = params->dt;
		if(right > sizeof(void*)<<8)
			st_base = (char*) realloc(st_base, right>>=1);
		else if(right < sizeof(void*)<<6)
			st_base = (char*) realloc(st_base, right = sizeof(void*)<<6);
		for(;;){
			usize task = dat.task_number++;
			if(task >= dat.tasks.size()) break;
			QNode* qn = dat.tasks[task];
			usize lst = st_alloc(4*sizeof(QNode*)), agg = st_alloc(attr_block_size);
			QNodeAggregate* aggf = (QNodeAggregate*)(st_base+agg);
			for(int i=0;i<attr_count;i++) construct_at(aggf+i);
			QNode **list = (QNode**)(st_base+lst), **l1 = list;
			char* r2 = r;
			if(((QNode*)r2)->chain) *l1++ = (QNode*) r2;
			if(((QNode*)(r2+=q5))->chain) *l1++ = (QNode*) r2;
			if(((QNode*)(r2+=q5))->chain) *l1++ = (QNode*) r2;
			if(((QNode*)(r2+=q5))->chain) *l1++ = (QNode*) r2;
			updatev(agg, qn, lst, lst + (l1-list)*sizeof(QNode*));
			st_pop(attr_block_size+4*sizeof(QNode*));
		}

		if(--dat.waiting){ swap(params->res->draw_data[a>>1], st.drawBuf); st.finish(); dat.stage2.wait(); }
		else{
			dat.stage1.lock();
			Node* n = params->toAdd;
			while(n){
				st.add(new (salloc(node_size)) Node(*n));
				Node* n2 = n->next; delete n; n = n2;
			}
			st.finish();
			q5 = qnode_size*5;
			UpdateResult* res = params->res;
			swap(res->draw_data[a>>1], st.drawBuf);
			dat.waiting = dat.thr_count;
			dat.task_number = 0;
			usize node_count = dat.node_count.load(relaxed);
			res->node_count = node_count;
			res->build_time = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - dat.start).count()*.001;
			dat.resLock.lock();
			UpdateResult* old = dat.latest;
			dat.latest = res;
			if(old && !--old->ref_count){ old->~UpdateResult(); free(old); }
			dat.resLock.unlock();
			dat.node_count.store(0, relaxed);
			usize size_limit = QNode::DEEP_BIAS + max<usize>(SMALLEST_MAX_TASK_SIZE, node_count / (dat.thr_count * 8));
			char* a = dat.roots.load(relaxed);
			dat.tasks.clear();
			dat.prev_size = ((QNode*)a)->size;
			add((QNode*)a, size_limit);
			add((QNode*)(a+=q5), size_limit);
			add((QNode*)(a+=q5), size_limit);
			add((QNode*)(a+=q5), size_limit);
			dat.stage2.unlock_all();
		}
		for(;;){
			usize task = dat.task_number++;
			if(task >= dat.tasks.size()) break;
			QNode* qn = dat.tasks[task];
			qn->gather<false>();
		}
		free_swap();
		if(!(a>>1)){
			dat.stage3.set();
			// If we are not the last ones, wait
			if(--dat.waiting) dat.stage3.wait();
			else dat.stage3.clear();
			q5 = qnode_size*5;
			char* qn = dat.roots.load(relaxed);
			((QNode*)qn)->gather<true>();
			((QNode*)(qn+=q5))->gather<true>();
			((QNode*)(qn+=q5))->gather<true>();
			((QNode*)(qn+=q5))->gather<true>();
			params->avail.lock();
			params = dat.params+(a&1); a ^= 1;
			params->avail.wait();
		}else{
			// If we are the last ones then i=0 definitely already called lock()
			if(!--dat.waiting) dat.stage3.unlock();
			params = dat.params+(a&1); a ^= 1;
			dat.stage1.wait();
		}
	}
	free(st_base);
	free_all();
}

};