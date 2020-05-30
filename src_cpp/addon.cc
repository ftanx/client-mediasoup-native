#include "DigitalStageClient.hpp"
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
static DigitalStageClient client;

static bool isConnected = false;
static bool shallReceiveAudio = false;
static bool shallStopReceivingAudio = false;
static bool isReceivingAudio = false;
static bool shallStreamAudio = false;
static bool shallStopStreamingAudio = false;
static bool isStreamingAudio = false;
static bool shallStreamVideo = false;
static bool shallStopStreamingVideo = false;
static bool isStreamingVideo = false;

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
        auto callback = [](Napi::Env env, Function jsCallback, std::string* value) {
            // Transform native data into JS data, passing it to the provided
            // `jsCallback` -- the TSFN's JavaScript function.

            jsCallback.Call({ env.Null(), String::New(env, *value) });

            // We're finished with the data.
            delete value;
        };

        mediasoupclient::Initialize();
        client.Start(url);
        std::string* value = new std::string( "{\"event\":\"connected\",\"value\":true}" );
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
                 shallStreamAudio = false;
                 std::string producerId = client.ProduceAudio();
                 if( producerId.length() > 0 ) {
                    isStreamingAudio = true;
                     std::string* value = new std::string( "{\"event\":\"audioProducerAdded\",\"value\":{\"producerId\":\"" + producerId + "\"}}" );
                     napi_status status = tsfn.BlockingCall( value, callback );
                 }
             } else if( shallStopStreamingAudio ) {
                 std::cout << "Shall stop streaming audio" << std::endl;
                 shallStopStreamingAudio = false;
                 client.StopProducingAudio();
                isStreamingAudio = false;
             }

             if( shallStreamVideo ) {
                  std::cout << "Shall stream video" << std::endl;
                  shallStreamVideo = false;
                  std::string producerId = client.ProduceVideo(false);
                  if( producerId.length() > 0 ) {
                     isStreamingVideo = true;
                      std::string* value = new std::string( "{\"event\":\"videoProducerAdded\",\"value\":{\"producerId\":\"" + producerId + "\"}}" );
                      napi_status status = tsfn.BlockingCall( value, callback );
                  }
              } else if( shallStopStreamingVideo ) {
                  std::cout << "Shall stop streaming video" << std::endl;
                  shallStopStreamingVideo = false;
                  client.StopProducingVideo();
                 isStreamingVideo = false;
              }
        }
        client.Stop();

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

Value StartStreamingVideo(const CallbackInfo& info)
{
	Napi::Env env = info.Env();
    if( !isStreamingVideo ) {
        shallStreamVideo = true;
        return Boolean::New(env, true);
    }
	return Boolean::New(env, false);
}

Value StopStreamingVideo(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if( isStreamingVideo ) {
        shallStopStreamingVideo = true;
        return Boolean::New(env, true);
    }
	return Boolean::New(env, false);
}

Value StartReceivingAudio(const CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::Function cb = info[0].As<Napi::Function>();
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
	exports.Set("startStreamingVideo", Function::New(env, StartStreamingVideo));
	exports.Set("stopStreamingVideo", Function::New(env, StopStreamingVideo));
	exports.Set("startReceivingAudio", Function::New(env, StartReceivingAudio));
	exports.Set("stopReceivingAudio", Function::New(env, StopReceivingAudio));
    exports.Set("disconnect", Function::New(env, Disconnect));
	return exports;
}

NODE_API_MODULE(digitalstage, Init)
