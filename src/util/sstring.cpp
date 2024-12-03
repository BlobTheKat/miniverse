#pragma once
#include <cstdint>
#include <cstring>
#include <string>

class sstring{
	char* _data;
	public:
	inline sstring() : _data(0){}
	inline sstring(const char* a, size_t len) : _data((char*)malloc(len+sizeof(size_t))+sizeof(size_t)){
		((size_t*)_data)[-1] = len;
		memcpy(_data, a, len);
	}
	inline sstring(const sstring& other){
		if(!other._data){ _data = 0; return; }
		size_t len = ((size_t*)other._data)[-1];
		char* d = (char*) malloc(len+sizeof(size_t));
		memcpy(_data=d+sizeof(size_t), other._data, *(size_t*)d = len);
	}
	inline sstring(sstring&& other) : _data(other._data){ other._data = 0; }
	inline sstring(const std::string& other) : sstring(other.data(), other.size()){}
	inline operator std::string() const{return {_data,_data?((size_t*)_data)[-1]:0};}
	inline operator char*() const{return _data;}
	inline operator bool() const{return _data!=0;}
	inline bool operator!() const{return _data==0;}
	inline char* data() const{return _data;}
	inline size_t size() const{return _data?((size_t*)_data)[-1]:0;}
	inline ~sstring(){ if(_data) free(_data-sizeof(size_t)); }
	inline sstring& operator=(const sstring& other){
		if(_data) free(_data-sizeof(size_t));
		new (this) sstring(other);
		return *this;
	}
	inline sstring& operator=(sstring&& other){
		if(_data) free(_data-sizeof(size_t));
		_data = other._data; other._data = 0;
		return *this;
	}
};