#ifndef DIGITALSTAGECLIENT_H
#define DIGITALSTAGECLIENT_H

#include "mediasoupclient.hpp"
#include "json.hpp"
#include <future>
#include <string>


class DigitalStageClient : public mediasoupclient::SendTransport::Listener,
                    mediasoupclient::Producer::Listener
{
public:
	std::future<void> OnConnect(
	  mediasoupclient::Transport* transport, const nlohmann::json& transportLocalParameters);
	void OnConnectionStateChange(mediasoupclient::Transport* transport, const std::string& connectionState);
	std::future<std::string> OnProduce(
	  mediasoupclient::SendTransport* /*transport*/,
	  const std::string& kind,
	  nlohmann::json rtpParameters,
	  const nlohmann::json& appData);

	/* Virtual methods inherited from Producer::Listener. */
public:
	void OnTransportClose(mediasoupclient::Producer* producer);

public:
	void Start(const std::string& baseUrl);
	void Stop();
	std::string ProduceAudio();
	std::string ProduceVideo(bool useSimulcast);
    void StopProducingAudio();
    void StopProducingVideo();

private:
	mediasoupclient::Device device;
	mediasoupclient::SendTransport* sendTransport;
	mediasoupclient::RecvTransport* receiveTransport;
    mediasoupclient::Producer* audioProducer;
    mediasoupclient::Producer* videoProducer;

	std::string baseUrl;
	std::string transportId;
	//std::vector<std::string> audioConsumerIds;
};
#endif // STOKER_HPP
