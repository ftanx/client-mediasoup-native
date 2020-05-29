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

static bool isConnected = false;
static bool shallReceiveAudio = false;
static bool shallStopReceivingAudio = false;
static bool isReceivingAudio = false;
static bool shallStreamAudio = false;
static bool shallStopStreamingAudio = false;
static bool isStreamingAudio = false;

Value Connect(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if( isConnected ) {
        Error::New(env, "Already connected").ThrowAsJavaScriptException();
        return Boolean::New(env, false);
    }

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
	const std::string url = info[0].As<String>().Utf8Value();

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
        auto callback = [](Napi::Env env, Function jsCallback, bool* value) {
            // Transform native data into JS data, passing it to the provided
            // `jsCallback` -- the TSFN's JavaScript function.
            jsCallback.Call({ env.Null(), Boolean::New(env, *value) });

            // We're finished with the data.
            delete value;
        };
        //TODO: Get the result of the connection of the client class
        bool* value = new bool( false );

        mediasoupclient::Initialize();
        broadcaster.Start(url, value, false);
        napi_status status = tsfn.BlockingCall( value, callback );
        if ( status != napi_ok ) {
            return;
        }
        if(value) {
            isConnected = true;
        }
        while (isConnected)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if( shallReceiveAudio ) {
                std::cout << "Shall receive audio" << std::endl;
                isReceivingAudio = true;
                 shallReceiveAudio = false;
            } else if( shallStopReceivingAudio ) {
                std::cout << "Shall stop receiving audio" << std::endl;
                isReceivingAudio = false;
                 shallStopReceivingAudio = false;
            }

             if( shallStreamAudio ) {
                 std::cout << "Shall stream audio" << std::endl;
                 isStreamingAudio = true;
                 shallStreamAudio = false;
             } else if( shallStopStreamingAudio ) {
                 std::cout << "Shall stop streaming audio" << std::endl;
                 isStreamingAudio = false;
                 shallStopStreamingAudio = false;
             }
        }
        broadcaster.Stop();

        // Release the thread-safe function
        tsfn.Release();
    });
	return Boolean::New(env, true);
}

Value Disconnect(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if( !isConnected ) {
        Error::New(env, "Not connected").ThrowAsJavaScriptException();
        return Boolean::New(env, false);
    }
    isConnected = false;
	return Boolean::New(env, true);
}

Value StartStreamingAudio(const CallbackInfo& info)
{
	Napi::Env env = info.Env();
    if( !isStreamingAudio ) {
        shallStreamAudio = true;
        return Boolean::New(env, true);
    }
	return Boolean::New(env, false);
}

Value StopStreamingAudio(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if( isStreamingAudio ) {
        shallStopStreamingAudio = true;
        return Boolean::New(env, true);
    }
	return Boolean::New(env, false);
}

Value StartReceivingAudio(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if( !isReceivingAudio ) {
        shallReceiveAudio = true;
        return Boolean::New(env, true);
    }
	return Boolean::New(env, false);
}

Value StopReceivingAudio(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if( isReceivingAudio ) {
        shallStopReceivingAudio = true;
        return Boolean::New(env, true);
    }
	return Boolean::New(env, false);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("connect", Function::New(env, Connect));
	exports.Set("startStreamingAudio", Function::New(env, StartStreamingAudio));
	exports.Set("stopStreamingAudio", Function::New(env, StopStreamingAudio));
	exports.Set("startReceivingAudio", Function::New(env, StartReceivingAudio));
	exports.Set("stopReceivingAudio", Function::New(env, StopReceivingAudio));
    exports.Set("disconnect", Function::New(env, Disconnect));
	return exports;
}

NODE_API_MODULE(digitalstage, Init)
