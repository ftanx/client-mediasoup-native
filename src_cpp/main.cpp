﻿#include "Broadcaster.hpp"
#include "mediasoupclient.hpp"
#include <cpr/cpr.h>
#include <csignal> // sigsuspend()
#include <cstdlib>
#include <iostream>
#include <string>

using json = nlohmann::json;

static Broadcaster broadcaster;

void signalHandler(int signum)
{
	std::cout << "[INFO] interrupt signal (" << signum << ") received" << std::endl;

	// Remove broadcaster from the server.
	broadcaster.Stop();

	std::cout << "[INFO] leaving!" << std::endl;

	std::exit(signum);
}

int main(int argc, char* argv[])
{
	// Register signal SIGINT and signal handler.
	signal(SIGINT, signalHandler);

	// Retrieve configuration from environment variables.
	const char* envServerUrl    = std::getenv("SERVER_URL");
	const char* envRoomId       = std::getenv("STAGE_ID");
	const char* envEnableAudio  = std::getenv("ENABLE_AUDIO");
	const char* envUseSimulcast = std::getenv("USE_SIMULCAST");
	const char* envWebrtcDebug  = std::getenv("WEBRTC_DEBUG");

	if (envServerUrl == nullptr)
	{
		std::cerr << "[ERROR] missing 'SERVER_URL' environment variable" << std::endl;

		return 1;
	}

	if (envRoomId == nullptr)
	{
		std::cerr << "[ERROR] missing 'ROOM_ID' environment variable" << std::endl;

		return 1;
	}

	std::string baseUrl = envServerUrl;

	bool enableAudio = true;

	if (envEnableAudio && std::string(envEnableAudio) == "false")
		enableAudio = false;

	bool useSimulcast = true;

	if (envUseSimulcast && std::string(envUseSimulcast) == "false")
		useSimulcast = false;

	// Set RTC logging severity.
	if (envWebrtcDebug && std::string(envWebrtcDebug) == "info")
		rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_INFO);
	else if (envWebrtcDebug && std::string(envWebrtcDebug) == "warn")
		rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_WARNING);
	else if (envWebrtcDebug && std::string(envWebrtcDebug) == "error")
		rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_ERROR);

	auto logLevel = mediasoupclient::Logger::LogLevel::LOG_DEBUG;
	mediasoupclient::Logger::SetLogLevel(logLevel);
	mediasoupclient::Logger::SetDefaultHandler();

	// Initilize mediasoupclient.
	mediasoupclient::Initialize();

	std::cout << "[INFO] welcome to digitalstage native client!\n" << std::endl;

	broadcaster.Start(baseUrl, enableAudio, useSimulcast);

	std::cout << "[INFO] press Ctrl+C or Cmd+C to leave..."<< std::endl;

	(void)sigsuspend(nullptr);

	return 0;
}
