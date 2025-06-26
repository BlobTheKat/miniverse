#include "util/defs.cpp"

/*const f64 DEFAULT_SAMPLE_RATE = 44100.;

template<int C = 1>
struct Audio{
	static const usize BYTES_PER_SAMPLE = sizeof(f32) * C;
	Audio(Audio<C>&& b){
		this->data = b.data;
		this->len = b.len;
		this->sample_rate = b.sample_rate;
		b.data = 0; b.len = 0;
	}
	Audio(Audio&) = delete;
	Audio() = delete;
	Audio(f32* dat, usize len, f32 sample_rate = DEFAULT_SAMPLE_RATE) : len(len), sample_rate(sample_rate){
		usize sz = sizeof(f32) * len * C;
		data = (f32*) malloc(sz);
		memcpy(data, dat, sz);
	}
	Audio(usize len, f64 sample_rate = DEFAULT_SAMPLE_RATE) : len(len), sample_rate(sample_rate){ data = (f32*) calloc(len, sizeof(f32) * C); }
	Audio(f32 dur, f64 sample_rate = DEFAULT_SAMPLE_RATE) : len(ceil(dur*sample_rate)), sample_rate(sample_rate){ data = (f32*) calloc(len, sizeof(f32) * C); }
	void operator=(Audio a) = delete; void operator=(Audio& a) = delete; void operator=(Audio&& a) = delete;
	~Audio(){ if(this->data) free(this->data); }
	f32* data = 0;
	usize len = 0;
	f64 sample_rate = DEFAULT_SAMPLE_RATE;
	f64 duration(){ return f64(len)/sample_rate; }
	inline f32 at(int ch, f64 t){
		usize x0 = floor(t *= sample_rate);
		if(x0 >= len-1) return 0;
		t -= x0; x0 = x0*C + ch;
		return data[x0] * (1-t) + data[x0+C] * t;
	}
	inline f32 at_sample(int ch, f64 t){
		usize x0 = floor(t);
		if(x0 >= len-1) return 0;
		t -= x0; x0 = x0*C + ch;
		return data[x0] * (1-t) + data[x0+C] * t;
	}
	inline void get_sample(f64 t, f32* out){
		usize x0 = floor(t);
		if(x0 >= len-1) return;
		t -= x0; x0 = x0*C;
		for(usize x1 = x0 + C; x0 < x1; x0++){
			*out++ += data[x0] * (1-t) + data[x0+C] * t;
		}
	}
	Audio<C> copy(){
		return Audio<C>(data, len, sample_rate);
	}
	Audio<C>& gain(f32 mult){
		usize sz = len*C;
		for(usize i = 0; i < sz; i++)
			data[i] *= mult;
		return *this;
	}
	Audio<C>& speed(f32 by){ this->sample_rate *= by; }
	void expand(f32 by){ expand(ceil(by * sample_rate)); }
	void expand(usize by){
		usize len2 = len + by;
		data = realloc(data, len2 * C * sizeof(f32));
		memset(&data[len * C], 0, by * C * sizeof(f32));
		len = len2;
	}
	void convolute(Audio<1>& a) requires(C > 1) {
		f64 ratio = sample_rate / a.sample_rate;
		usize ex = ceil(f64(a.len) * ratio);
		this->expand(ex);
		for(usize i = len-1; i >= 0; i--){
			f32* d = &data[i*C];
			f32 v[C] = {0};
			usize i1 = i;
			for(usize j = 0, i1 = i; j < a.len; j++, i1 -= ratio){
				f32 x = a.data[j];
				for(int ch = 0; ch < C; ch++) v[ch] += this->at_sample(ch, i1) * x;
			}
			memcpy(d, v, sizeof(f32) * C);
		}
	}
	void convolute(Audio<C>& a){
		f64 ratio = sample_rate / a.sample_rate;
		usize ex = ceil(f64(a.len) * ratio);
		this->expand(ex);
		for(usize i = len-1; i >= 0; i--){
			f32* d = &data[i*C];
			f32 v[C] = {0};
			usize i1 = i;
			for(usize j = 0, i1 = i; j < a.len; j+=C, i1 -= ratio){
				for(int ch = 0; ch < C; ch++) v[ch] += this->at_sample(ch, i1) * a.data[j+ch];
			}
			memcpy(d, v, sizeof(f32) * C);
		}
	}
	Audio<C>& pan(f32 amt) requires(C == 2) {
		f32 right = 1.+min(amt, 0.f), left = right-amt;
		usize sz = len<<1;
		for(usize i = 0; i < sz; i+=2)
			data[i] *= left, data[i+1] *= right;
		return *this;
	}
	Audio<C>& matrix(mat<f32,C,C>& m){
		usize sz = len*C;
		for(usize i = 0; i < sz; i+=C)
			*(vec<f32,C>*)&data[i] = m * *(vec<f32,C>*)&data[i];
		return *this;
	}
	Audio<1> mono() requires (C > 1) {
		Audio<1> m(this->len, this->sample_rate);
		for(usize i = 0; i < this->len; i++){
			f32 tot = 0;
			for(f32* j = &data[i*C], *end = j+C; j < end; j++) tot += *j;
			m[i] = tot * (1.f/f32(C));
		}
		return m;
	}
	template<typename L> Audio& process(L a){
		usize sz = len*C; f64 t = 0, ts = 1./sample_rate;
		for(usize i = 0; i < sz; i+=C, t += ts)
			*(vec<f32,C>*)&data[i] = a(*(vec<f32,C>*)&data[i], t);
		return *this;
	}
	template<typename L> Audio& fill(L a){
		usize sz = len*C; f64 t = 0, ts = 1./sample_rate;
		for(usize i = 0; i < sz; i+=C, t += ts)
			*(vec<f32,C>*)&data[i] = a(t);
		return *this;
	}
	inline void set(int ch, f32* d){
		for(usize i = 0, j = ch; i < len; i++, j += C)
			data[j] = d[i];
	}
	template<int C1>
	inline void set(int ch, Audio<C1>& a, int ch0 = 0){
		usize sz = min(len, a.len) * C1;
		for(usize i = ch0, j = ch; i < sz; i += C1, j += C)
			data[j] = a.data[i];
	}
	inline Audio<1> get(int ch){
		Audio<1> a(len, sample_rate);
		for(usize i = 0, j = ch; i < len; i++, j += C)
			a.data[i] = data[j];
		return a;
	}
	inline operator f32*(){ return data; }
};

void test(){
	Audio<2> a(2.f);
	a.fill([](f64 t){
		return vec2(frandom()-.5,frandom()-.5);
	});
	//audio_device.push(t, a);
}

template<int C = 1>
struct AudioComposer{
	static const usize BYTES_PER_SAMPLE = sizeof(f32) * C;
	f64 t = 0;
	usize buffered = 0;
	f64 sample_rate;
	private:
	f64 i_sample_rate;
	usize cap = 0, left = 0;
	f32* data = 0;
	inline void grow(usize min){
		usize b = buffered;
		if(b >= min) return;
		buffered = min;
		min *= C * sizeof(f32);
		if(cap < min){
			char* c = malloc(cap = min<<1);
			memcpy(c, data, b);
			free(data - left);
			data = (f32*) c;
			left = 0;
			memset(c + min, 0, min);
		}
	}
	public:
	AudioComposer(f64 sample_rate = DEFAULT_SAMPLE_RATE) : sample_rate(sample_rate){
		i_sample_rate = 1./sample_rate;
	}
	f64 push(f64 when, Audio<C>& a){
		usize i0 = usize(round((when - t) * sample_rate)), i1;
		if(sample_rate == a.sample_rate){ // r1 == r
			// very fast path
			i1 = i0 + a.len;
			if(i1 < 0) goto end;
			f32* d = a.data;
			if(i0 < 0){
				// d += (-i0) * C;
				d -= i0 * C;
				i0 = 0;
			}
			grow(i1);
			f32* o = data + (i0*C), e = data + (i1*C);
			for(; o < e; o ++) *o++ += *d++;
			end:
			goto end;
		}
		f64 diff = a.sample_rate * i_sample_rate;
		f64 diff2;
		if(!modf(diff, &diff2)){ // r1 == k*r
			// fast path
			usize stride = usize(diff2) * C;
			i1 = i0 + (a.len+diff2-1)/diff2;
			f32* d = a.data;
			if(i0 < 0){
				// d += (-i0) * stride;
				d -= i0 * stride;
				i0 = 0;
			}
			if(i1 < 0) goto end;
			grow(i1);
			f32* o = data + (i0*C), e = data + (i1*C);
			for(; o < e; o += C){
				for(int ch = 0; ch < C; ch++) o[ch] += d[ch];
				d += stride;
			}
			goto end;
		}
		// slow path
		i1 = i0 + usize(ceil(a.len / diff2));
		f32* o = data + (i0*C), e = data + (i1*C);
		f64 s = 0;
		for(;o < e; o += C){
			a.get_sample(s, o);
			s += diff;
		}
		end:
		return f64(i1) * i_sample_rate + t;
	}
	inline void push(Audio<C>& a){ push(t, a); }
	Audio<C> pop(usize count){
		if(count >= buffered){
			Audio<C> ret = {data, buffered, sample_rate};
			if(left){
				usize sz = buffered * C * sizeof(f32);
				ret.data = malloc(sz);
				memcpy(ret.data, data, sz);
				free(data - left);
			}
			data = 0; buffered = cap = 0;
			return ret;
		}
		buffered;
	}
	usize pop(usize max, f32* dat){
		if(max >= buffered){
			usize b = buffered;
			memcpy(dat, data, buffered * C * sizeof(f32));
			free(data - left);
			data = 0; buffered = cap = 0;
			return b;
		}
	}
	Audio<C> pop(){ return pop(buffered); }
};*/