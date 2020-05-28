#include "Broadcaster.hpp"
#include "mediasoupclient.hpp"
#include <chrono>
#include <cpr/cpr.h>
#include <iostream>
#include <napi.h>
#include <string>
#include <thread>

using namespace Napi;

std::thread nativeThread;
ThreadSafeFunction tsfn;
static Broadcaster broadcaster;
static bool running = false;

Value Start(const CallbackInfo& info)
{
	Napi::Env env = info.Env();
	if (info.Length() < 2)
	{
		throw TypeError::New(env, "Expected two arguments");
	}
	else if (!info[1].IsFunction())
	{
		throw TypeError::New(env, "Expected first arg to be function");
	}
	else if (!info[0].IsString())
	{
		throw TypeError::New(env, "Expected second arg to be string");
	}

	std::string url = info[0].As<String>().Utf8Value();

	// Create a ThreadSafeFunction
	tsfn = ThreadSafeFunction::New(
	  env,
	  info[1].As<Function>(), // JavaScript function called asynchronously
	  "Resource Name",        // Name
	  0,                      // Unlimited queue
	  1,                      // Only one thread will use this initially
	  [](Napi::Env) {         // Finalizer used to clean threads up
		  nativeThread.join();
	  });

	nativeThread = std::thread([url] {
		auto callback = [](Napi::Env env, Function jsCallback, int* value) {
			// Transform native data into JS data, passing it to the provided
			// `jsCallback` -- the TSFN's JavaScript function.
			jsCallback.Call({ Number::New(env, *value) });

			// We're finished with the data.
			delete value;
		};
    int* value = new int( clock() );

		mediasoupclient::Initialize();
		broadcaster.Start(url, value, false);
    napi_status status = tsfn.BlockingCall( value, callback );
    if ( status != napi_ok ) {
      return;
		}


		// Perform a blocking call
		// napi_status status = tsfn.BlockingCall( value, callback );
		// if ( status != napi_ok )
		//{
		// Handle error
		//  break;
		//}
		running = true;
		while (running)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

    broadcaster.Stop();

		// Release the thread-safe function
		tsfn.Release();
	});

	return Boolean::New(env, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	exports.Set("start", Function::New(env, Start));
	return exports;
}

NODE_API_MODULE(digitalstage, Init)
