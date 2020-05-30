#include "DigitalStageClient.hpp"
#include "MediaStreamTrackFactory.hpp"
#include "mediasoupclient.hpp"
#include "json.hpp"
#include <cpr/cpr.h>
#include <cstdlib>
#include <iostream>
#include <string>

using json = nlohmann::json;

void DigitalStageClient::OnTransportClose(mediasoupclient::Producer* /*producer*/)
{
  std::cout << "[INFO] DigitalStageClient::OnTransportClose()" << std::endl;
}


std::future<void> DigitalStageClient::OnConnect(
  mediasoupclient::Transport* transport, const json& dtlsParameters)
{
    std::cout << "[INFO] DigitalStageClient::OnConnect()" << std::endl;

  std::promise<void> promise;

  /* clang-format off */
  json body =
    {
      { "transportId", transport->GetId() },
      { "dtlsParameters", dtlsParameters }
    };
  /* clang-format on */


  std::cout << body.dump() << std::endl;

  auto r = cpr::PostAsync(
    cpr::Url{ this->baseUrl + "/transport/webrtc/connect" },
    cpr::Body{ body.dump() },
    cpr::Header{ { "Content-Type", "application/json" } },
    cpr::VerifySsl{ false })
    .get();

  if (r.status_code == 200)
  {
    promise.set_value();
  }
  else
  {
    std::cerr << "[ERROR] unable to connect transport"
              << " [status code:" << r.status_code << ", body:\"" << r.text << "\"]" << std::endl;

    promise.set_exception(std::make_exception_ptr(r.text));
  }

  return promise.get_future();
}

void DigitalStageClient::OnConnectionStateChange(
  mediasoupclient::Transport* transport, const std::string& connectionState)
{
  std::cout << "[INFO] DigitalStageClient::OnConnectionStateChange() [connectionState:" << connectionState
            << "]" << std::endl;

  if (connectionState == "failed")
  {
    Stop();
    std::exit(0);
  }
}

std::future<std::string> DigitalStageClient::OnProduce(
  mediasoupclient::SendTransport* /*transport*/,
  const std::string& kind,
  json rtpParameters,
  const json& appData)
{
  std::cout << "[INFO] DigitalStageClient::OnProduce()" << std::endl;

  std::promise<std::string> promise;

  /* clang-format off */
  json body =
    {
      { "transportId",   this->sendTransport->GetId()},
      { "kind",          kind             },
      { "appData",       appData          },
      { "rtpParameters", rtpParameters    }
    };
  /* clang-format on */

  auto r = cpr::PostAsync(
    cpr::Url{ this->baseUrl + "/producer/create/" },
    cpr::Body{ body.dump() },
    cpr::Header{ { "Content-Type", "application/json" } },
    cpr::VerifySsl{ false })
    .get();

  if (r.status_code == 200)
  {
    auto response = json::parse(r.text);

    auto it = response.find("id");
    if (it == response.end() || !it->is_string())
      promise.set_exception(std::make_exception_ptr("'id' missing in response"));

    promise.set_value((*it).get<std::string>());
  }
  else
  {
    std::cerr << "[ERROR] unable to create producer"
              << " [status code:" << r.status_code << ", body:\"" << r.text << "\"]" << std::endl;

    promise.set_exception(std::make_exception_ptr(r.text));
  }

  return promise.get_future();
}

std::string DigitalStageClient::ProduceAudio()
{
  std::cout << "[INFO] ProduceAudio" << std::endl;
  if (!this->audioProducer)
  {
    if( !this->device.CanProduce("audio") ) {
      std::cerr << "[WARN] cannot produce audio" << std::endl;
      return "";
    }
    auto audioTrack = createAudioTrack(std::to_string(rtc::CreateRandomId()));

    /* clang-format off */
    json codecOptions = {
      { "opusStereo", true },
      { "opusDtx",		true }
    };
    /* clang-format on */

    this->audioProducer = this->sendTransport->Produce(this, audioTrack, nullptr, &codecOptions);
  }
  return this->audioProducer->GetId();
}

void DigitalStageClient::StopProducingAudio()
{
  std::cout << "[INFO] StopProducingAudio" << std::endl;
  if (this->audioProducer && !this->audioProducer->IsClosed())
  {
    this->audioProducer->Close();
    delete this->audioProducer;
    this->audioProducer = NULL;
  }
}

std::string DigitalStageClient::ProduceVideo(bool useSimulcast)
{
  std::cout << "[INFO] ProduceVideo" << std::endl;
  if (!this->videoProducer)
  {
    if( !this->device.CanProduce("video") ) {
        std::cerr << "[WARN] cannot produce video" << std::endl;
        return "";
      }
    auto videoTrack = createSquaresVideoTrack(std::to_string(rtc::CreateRandomId()));

    if (useSimulcast)
    {
      std::vector<webrtc::RtpEncodingParameters> encodings;
      encodings.emplace_back(webrtc::RtpEncodingParameters());
      encodings.emplace_back(webrtc::RtpEncodingParameters());
      encodings.emplace_back(webrtc::RtpEncodingParameters());

       this->videoProducer = this->sendTransport->Produce(this, videoTrack, &encodings, nullptr);
    }
    else
    {
      this->videoProducer = this->sendTransport->Produce(this, videoTrack, nullptr, nullptr);
    }
  }
  return  this->videoProducer->GetId();
}

void DigitalStageClient::StopProducingVideo()
{
  if (this->videoProducer && !this->videoProducer->IsClosed())
  {
    this->videoProducer->Close();
    delete this->videoProducer;
    this->videoProducer = NULL;
  }
}

void DigitalStageClient::Start(
  const std::string& baseUrl)
{
  std::cout << "[INFO] DigitalStageClient::Start()" << std::endl;

  this->baseUrl = baseUrl;

  std::cout << "[INFO] Get rtp capabilities from " << this->baseUrl + "/rtp-capabilities" << "..." << std::endl;

  auto r = cpr::GetAsync(
    cpr::Url{ this->baseUrl + "/rtp-capabilities" },
    cpr::Header{ { "Content-Type", "application/json" } },
    cpr::VerifySsl{ false })
    .get();

  if (r.status_code != 200)
  {
    std::cerr << "[ERROR] unable to get rtp capabilities"
              << " [status code:" << r.status_code << ", body:\"" << r.text << "\"]" << std::endl;

    return;
  }

  nlohmann::json routerRtpCapabilities = nlohmann::json::parse(r.text);

  // Load the device.
  this->device.Load(routerRtpCapabilities);

  std::cout << "[INFO] creating mediasoup WebRtcTransport..." << std::endl;

  r = cpr::GetAsync(
    cpr::Url{ this->baseUrl + "/transport/webrtc/create" },
    cpr::Header{ { "Content-Type", "application/json" } },
    cpr::VerifySsl{ false })
    .get();

  if (r.status_code != 200)
  {
    std::cerr << "[ERROR] unable to create mediasoup WebRtcTransport"
              << " [status code:" << r.status_code << ", body:\"" << r.text << "\"]" << std::endl;

    return;
  }

  auto response = json::parse(r.text);

  if (response.find("id") == response.end())
  {
    std::cerr << "[ERROR] 'id' missing in response" << std::endl;

    return;
  }
  else if (response.find("iceParameters") == response.end())
  {
    std::cerr << "[ERROR] 'iceParametersd' missing in response" << std::endl;

    return;
  }
  else if (response.find("iceCandidates") == response.end())
  {
    std::cerr << "[ERROR] 'iceCandidates' missing in response" << std::endl;

    return;
  }
  else if (response.find("dtlsParameters") == response.end())
  {
    std::cerr << "[ERROR] 'dtlsParameters' missing in response" << std::endl;

    return;
  }

  std::cout << "[INFO] creating SendTransport..." << std::endl;

  auto transportId = response["id"].get<std::string>();

  this->sendTransport = this->device.CreateSendTransport(
    this,
    transportId,
    response["iceParameters"],
    response["iceCandidates"],
    response["dtlsParameters"]);
}

void DigitalStageClient::Stop()
{
  std::cout << "[INFO] DigitalStageClient::Stop()" << std::endl;
}
