#define V8_COMPRESS_POINTERS
#define V8_ENABLE_SANDBOX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <v8/libplatform/libplatform.h>
#include <v8/v8-context.h>
#include <v8/v8-initialization.h>
#include <v8/v8-isolate.h>
#include <v8/v8-local-handle.h>
#include <v8/v8-primitive.h>
#include <v8/v8-script.h>
using namespace std;
using namespace v8;

void test(){
	std::unique_ptr<Platform> platform = platform::NewDefaultPlatform();
	V8::InitializePlatform(platform.get());
	V8::Initialize();
	Isolate::CreateParams cr;
	cr.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	Isolate* isolate = Isolate::New(cr);
{
	Isolate::Scope isolate_scope(isolate);
	// Create a stack-allocated handle scope.
	HandleScope handle_scope(isolate);
	// Create a new context.
	Local<Context> context = Context::New(isolate);
	// Enter the context for compiling and running the hello world script.
	Context::Scope context_scope(context);
	test1: {
		// Create a string containing the JavaScript source code.
		Local<String> source = String::NewFromUtf8Literal(isolate, "`Hello from ${['ASM', 'C++', 'Javascipt!'].pop()}`");
		// Compile the source code.
		Local<Script> script = Script::Compile(context, source).ToLocalChecked();
		// Run the script to get the result.
		Local<Value> result = script->Run(context).ToLocalChecked();
		// Convert the result to an UTF8 string and print it.
		String::Utf8Value utf8(isolate, result);
		printf("%s\n", *utf8);
	}
	test2: {
		// Use the JavaScript API to generate a WebAssembly module.
		//
		// |bytes| contains the binary format for the following module:
		//
		//     (func (export "add") (param i32 i32) (result i32)
		//       get_local 0
		//       get_local 1
		//       i32.add)
		//
		const char csource[] = R"(
			let bytes = new Uint8Array([
				0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
				0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
				0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
				0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b
			]);
			let module = new WebAssembly.Module(bytes);
			let instance = new WebAssembly.Instance(module);
			instance.exports.add(3, 4);
		)";
		// Create a string containing the JavaScript source code.
		v8::Local<v8::String> source =
				v8::String::NewFromUtf8Literal(isolate, csource);
		// Compile the source code.
		v8::Local<v8::Script> script =
				v8::Script::Compile(context, source).ToLocalChecked();
		// Run the script to get the result.
		v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
		// Convert the result to a uint32 and print it.
		uint32_t number = result->Uint32Value(context).ToChecked();
		printf("3 + 4 = %u\n", number);
	}
}

	isolate->Dispose();
	V8::Dispose();
	V8::DisposePlatform();
	delete cr.array_buffer_allocator;
}