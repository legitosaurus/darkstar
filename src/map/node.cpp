/*
===========================================================================

Copyright (c) 2010-2015 Darkstar Dev Teams

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/

This file is part of DarkStar-server source code.

===========================================================================
*/

#include <mutex>
#include <queue>

#include "node.h"

#include "party.h"
#include "alliance.h"

#include "entities/charentity.h"

#include "packets/message_standard.h"
#include "packets/party_invite.h"
#include "packets/server_ip.h"

#include "utils/charutils.h"
#include "utils/zoneutils.h"
#include "utils/jailutils.h"

#include "../common/rapidjson/writer.h"
#include "../common/rapidjson/stringbuffer.h"
#include <iostream>

namespace node
{
	zmq::context_t zContext;
	zmq::socket_t* zSocket = nullptr;
	std::mutex node_mutex;
	std::queue<node_message_t> node_queue;

	void queue_message(uint64 ipp, MSGSERVTYPE type, zmq::message_t* extra, zmq::message_t* packet)
	{
		std::lock_guard<std::mutex>lk(node_mutex);
		node_message_t msg;
		msg.dest = ipp;

		msg.type = type;

		msg.data = new zmq::message_t(extra->size());
		memcpy(msg.data->data(), extra->data(), extra->size());

		msg.packet = new zmq::message_t(packet->size());
		memcpy(msg.packet->data(), packet->data(), packet->size());

		node_queue.push(msg);
	}

	void node_send(uint64 ipp, MSGSERVTYPE type, zmq::message_t* extra, zmq::message_t* packet)
	{
		try
		{
			zmq::message_t to(sizeof(uint64));
			memcpy(to.data(), &ipp, sizeof(uint64));
			zSocket->send(to, ZMQ_SNDMORE);

			zmq::message_t newType(sizeof(MSGSERVTYPE));
			WBUFB(newType.data(), 0) = type;
			zSocket->send(newType, ZMQ_SNDMORE);

			zmq::message_t newExtra(extra->size());
			memcpy(newExtra.data(), extra->data(), extra->size());
			zSocket->send(newExtra, ZMQ_SNDMORE);

			zmq::message_t newPacket(packet->size());
			memcpy(newPacket.data(), packet->data(), packet->size());
			zSocket->send(newPacket);
		}
		catch (zmq::error_t e)
		{
			ShowError("Message: %s", e.what());
		}
	}
	void node_listen()
	{
		while (true)
		{
			zmq::message_t from;
			zmq::message_t type;
			zmq::message_t extra;
			zmq::message_t packet;

			try
			{
				if (!zSocket->recv(&from))
				{
					if (!node_queue.empty())
					{
						std::lock_guard<std::mutex>lk(node_mutex);
						while (!node_queue.empty())
						{
							node_message_t msg = node_queue.front();
							node_queue.pop();
							node_send(msg.dest, msg.type, msg.data, msg.packet);
						}
					}
					continue;
				}
				int more;
				size_t size = sizeof(more);
				zSocket->getsockopt(ZMQ_RCVMORE, &more, &size);
				if (more)
				{
					zSocket->recv(&type);
					zSocket->getsockopt(ZMQ_RCVMORE, &more, &size);
					if (more)
					{
						zSocket->recv(&extra);
						zSocket->getsockopt(ZMQ_RCVMORE, &more, &size);
						if (more)
						{
							zSocket->recv(&packet);
						}
					}
				}
			}
			catch (zmq::error_t e)
			{
				if (!zSocket)
				{
					return;
				}
				ShowError("Message: %s\n", e.what());
				continue;
			}
			std::string extraString(static_cast<char*>(extra.data()), extra.size());
			std::string packetString(static_cast<char*>(packet.data()), packet.size());
			std::string fromString(static_cast<char*>(from.data()), from.size());
			ShowWarning("web::listen extra: %d\n", extraString);
			ShowDebug("web::listen packet: %d\n", packetString);
			ShowDebug("web::listen from: %d\n", fromString);
			//message_server_parse((MSGSERVTYPE)RBUFB(type.data(), 0), &extra, &packet, &from);
		}
	}
	void init(const char* webIp, uint16 webPort)
	{
		zContext = zmq::context_t(1);
		zSocket = new zmq::socket_t(zContext, ZMQ_ROUTER);

		uint32 to = 500;
		zSocket->setsockopt(ZMQ_RCVTIMEO, &to, sizeof to);

		string_t server = "tcp://";
		server.append("127.0.0.1");
		server.append(":");
		server.append("53230");

		try
		{
			zSocket->bind(server.c_str());
		}
		catch (zmq::error_t err)
		{
			ShowFatalError("Unable to bind chat socket: %s\n", err.what());
		}

		node_listen();
	}
	void node_close()
	{
		zContext.close();
		if (zSocket)
		{
			zSocket->close();
			delete zSocket;
			zSocket = nullptr;
		}
	}
}