#ifndef DIGITALSTAGECLIENT_H
#define DIGITALSTAGECLIENT_H

#include "mediasoupclient.hpp"
#include "json.hpp"
#include <future>
#include <string>


class Broadcaster : public mediasoupclient::SendTransport::Listener,
                    mediasoupclient::Producer::Listener
{

}
