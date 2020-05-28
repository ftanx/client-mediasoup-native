#include "Broadcaster.hpp"
#include "mediasoupclient.hpp"
#include <cpr/cpr.h>
#include <iostream>
#include <chrono>
#include <napi.h>
#include <string>
#include <thread>

std::thread nativeThread;
bool running = false;
static Broadcaster broadcaster;

class StartAsyncWorker : public Napi::AsyncWorker
{
public:
	StartAsyncWorker(const Napi::Function& callback, const std::string& arg0)
	  : Napi::AsyncWorker(callback), arg0(arg0), result("")
	{
	}

protected:
	void Execute() override
	{
		if (nativeThread != null)
		{
			nativeThread = std::thread([count] {
        running      = true;
				mediasoupclient::Initialize();
				broadcaster.Start(arg0, true, false);

				while (running)
				{
          std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
				}
				broadcaster.Stop();
			});
		}
		result = "Hello" + arg0;
	}

	void OnOK() override
	{
		Napi::Env env = Env();

		Callback().MakeCallback(Receiver().Value(), { env.Null(), Napi::String::New(env, result) });
	}

	void OnError(const Napi::Error& e) override
	{
		Napi::Env env = Env();

		Callback().MakeCallback(Receiver().Value(), { e.Value(), env.Undefined() });
	}

private:
	std::string arg0;
	std::string result;
};

class StopAsyncWorker : public Napi::AsyncWorker
{
public:
  StopAsyncWorker(const Napi::Function& callback)
    : Napi::AsyncWorker(callback), result("")
  {
  }

protected:
  void Execute() override
  {
    running = false;
    result = "Hello";
  }

  void OnOK() override
  {
    Napi::Env env = Env();

    Callback().MakeCallback(Receiver().Value(), { env.Null(), Napi::String::New(env, result) });
  }

  void OnError(const Napi::Error& e) override
  {
    Napi::Env env = Env();

    Callback().MakeCallback(Receiver().Value(), { e.Value(), env.Undefined() });
  }

private:
  std::string result;
};

void StartAsyncCallback(const Napi::CallbackInfo& info)
{
	Napi::Env env = info.Env();
	if (info.Length() < 2)
	{
		Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
		return;
	}

	if (!info[1].IsFunction())
	{
		Napi::TypeError::New(env, "Invalid argument types").ThrowAsJavaScriptException();
		return;
	}

	Napi::Function cb = info[1].As<Napi::Function>();
	std::string arg0  = info[0].As<Napi::String>().Utf8Value();

	(new StartAsyncWorker(cb, arg0))->Queue();
}

void StopAsyncCallback(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
    return;
  }

  if (!info[0].IsFunction())
  {
    Napi::TypeError::New(env, "Invalid argument types").ThrowAsJavaScriptException();
    return;
  }

  Napi::Function cb = info[0].As<Napi::Function>();

  (new StopAsyncWorker(cb))->Queue();
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	exports.Set(Napi::String::New(env, "start"), Napi::Function::New(env, StartAsyncCallback),
              Napi::String::New(env, "stop"), Napi::Function::New(env, StopAsyncCallback));
	return exports;
}

NODE_API_MODULE(digitalstage, Init)
