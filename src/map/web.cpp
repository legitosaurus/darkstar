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

#include "web.h"

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

namespace web
{
    zmq::context_t zContext;
    zmq::socket_t* zSocket = nullptr;
    std::mutex send_mutex;
    std::queue<web_message_t> web_queue;

    void send_queue()
    {
		while (!web_queue.empty())
        {
            std::lock_guard<std::mutex> lk(send_mutex);
			web_message_t wbm = web_queue.front();
			web_queue.pop();
            try
            {
                zSocket->send(*wbm.type, ZMQ_SNDMORE);
                zSocket->send(*wbm.data, ZMQ_SNDMORE);
                zSocket->send(*wbm.packet);
            }
            catch (std::exception& e)
            {
                ShowError("Message: %s", e.what());
            }
        }
    }
	void sendJSON(rapidjson::StringBuffer & jsonBuffer)
	{
		string_t jsonString = jsonBuffer.GetString();
		zmq::message_t jsonPayload((void*)jsonBuffer.GetString(), jsonString.size(), NULL);
		zSocket->send(jsonPayload);
	};
    void parse(MSGSERVTYPE type, zmq::message_t* extra, zmq::message_t* packet)
    {
        ShowDebug("Message: Received message %d from message server\n", type);
        switch (type)
        {
        case MSG_LOGIN:
        {
            CCharEntity* PChar = zoneutils::GetChar(RBUFL(extra->data(), 0));

            if (!PChar)
            {
                Sql_Query(SqlHandle, "DELETE FROM accounts_sessions WHERE charid = %d;", RBUFL(extra->data(), 0));
            }
            else
            {
                PChar->status = STATUS_SHUTDOWN;
                //won't save their position (since this is the wrong thread) but not a huge deal
                PChar->pushPacket(new CServerIPPacket(PChar, 1, 0));
            }
            break;
        }
        case MSG_CHAT_TELL:
        {
            CCharEntity* PChar = zoneutils::GetCharByName((int8*)extra->data() + 4);
            if (PChar && PChar->status != STATUS_DISAPPEAR && !jailutils::InPrison(PChar))
            {
                if (PChar->nameflags.flags & FLAG_AWAY)
                {
                    send(MSG_DIRECT, extra->data(), sizeof(uint32), new CMessageStandardPacket(PChar, 0, 0, 181));
                }
                else
                {
                    CBasicPacket* newPacket = new CBasicPacket();
                    memcpy(*newPacket, packet->data(), dsp_min(packet->size(), PACKET_SIZE));
                    PChar->pushPacket(newPacket);
                }
            }
            else
            {
                send(MSG_DIRECT, extra->data(), sizeof(uint32), new CMessageStandardPacket(PChar, 0, 0, 125));
            }
            break;
        }
        case MSG_CHAT_PARTY:
        {
            CCharEntity* PChar = zoneutils::GetChar(RBUFL(extra->data(), 0));
            if (PChar)
            {
                if (PChar->PParty)
                {
                    if (PChar->PParty->m_PAlliance != nullptr)
                    {
                        for (uint8 i = 0; i < PChar->PParty->m_PAlliance->partyList.size(); ++i)
                        {
                            CBasicPacket* newPacket = new CBasicPacket();
                            memcpy(*newPacket, packet->data(), dsp_min(packet->size(), PACKET_SIZE));
                            ((CParty*)PChar->PParty->m_PAlliance->partyList.at(i))->PushPacket(RBUFL(extra->data(), 4), 0, newPacket);
                        }
                    }
                    else
                    {
                        CBasicPacket* newPacket = new CBasicPacket();
                        memcpy(*newPacket, packet->data(), dsp_min(packet->size(), PACKET_SIZE));
                        PChar->PParty->PushPacket(RBUFL(extra->data(), 4), 0, newPacket);
                    }
                }
            }
            break;
        }
        case MSG_CHAT_LINKSHELL:
        {
            uint32 linkshellID = RBUFL(extra->data(), 0);
            CLinkshell* PLinkshell = linkshell::GetLinkshell(linkshellID);
            if (PLinkshell)
            {
                CBasicPacket* newPacket = new CBasicPacket();
                memcpy(*newPacket, packet->data(), dsp_min(packet->size(), PACKET_SIZE));
                PLinkshell->PushPacket(RBUFL(extra->data(), 4), newPacket);
            }
            break;
        }
        case MSG_CHAT_YELL:
        {
            zoneutils::ForEachZone([&packet](CZone* PZone)
            {
                if (PZone->CanUseMisc(MISC_YELL))
                {
                    PZone->ForEachChar([&packet](CCharEntity* PChar)
                    {
                        CBasicPacket* newPacket = new CBasicPacket();
                        memcpy(*newPacket, packet->data(), dsp_min(packet->size(), PACKET_SIZE));
                        PChar->pushPacket(newPacket);
                    });
                }
            });
            break;
        }
        case MSG_CHAT_SERVMES:
        {
            zoneutils::ForEachZone([&packet](CZone* PZone)
            {
                PZone->ForEachChar([&packet](CCharEntity* PChar)
                {
                    CBasicPacket* newPacket = new CBasicPacket();
                    memcpy(*newPacket, packet->data(), dsp_min(packet->size(), PACKET_SIZE));
                    PChar->pushPacket(newPacket);
                });
            });
            break;
        }
        default:
        {
            ShowWarning("Message: unhandled message type %d\n", type);
        }
        }
    }

    void listen()
    {
        while (true)
        {
            zmq::message_t type;
            zmq::message_t extra;
            zmq::message_t packet;

            try
            {
                if (!zSocket)
                {
                    return;
                }
                if (!zSocket->recv(&type))
                {
					if (!web_queue.empty())
                        send_queue();
                    continue;
                }

                int more;
                size_t size = sizeof(more);
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
            catch (zmq::error_t e)
            {
                if (!zSocket)
                {
                    return;
                }
                ShowError("Message: %s\n", e.what());
                continue;
            }

            parse((MSGSERVTYPE)RBUFB(type.data(), 0), &extra, &packet);
        }
    }

    void init(const char* webIp, uint16 webPort)
    {
        zContext = zmq::context_t(1);
        zSocket = new zmq::socket_t(zContext, ZMQ_DEALER);

        uint64 ipp = node_ip.s_addr;
        uint64 port = node_port;
		string_t myIdent;
		//ShowWarning("web::init webIp: %d\n", webIp);
		//ShowWarning("web::init webPort: %d\n", webPort);
		if (node_ip.s_addr == 0 && node_port == 0)
		{
			myIdent = "mapServer";
			port = 54220;
		}
		ipp |= (port << 32);
		zSocket->setsockopt(ZMQ_IDENTITY, "mapServer", 9);

        uint32 to = 500;
        zSocket->setsockopt(ZMQ_RCVTIMEO, &to, sizeof to);

        string_t server = "tcp://";
		server.append(webIp);
        server.append(":");
		server.append(std::to_string(webPort));

        try
        {
            zSocket->connect(server.c_str());
        }
        catch (zmq::error_t err)
        {
            ShowFatalError("Message: Unable to connect web socket: %s\n", err.what());
        }
        listen();
    }

    void close()
    {
        if (zSocket)
        {
            zSocket->close();
            delete zSocket;
            zSocket = nullptr;
        }
        zContext.close();
    }
	void web_server_send(uint64 ipp, MSGSERVTYPE type, zmq::message_t* extra, zmq::message_t* packet)
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
    void send(MSGSERVTYPE type, void* data, size_t datalen, CBasicPacket* packet)
    {
        std::lock_guard<std::mutex> lk(send_mutex);
        web_message_t msg;
        msg.type = new zmq::message_t(sizeof(MSGSERVTYPE));
        WBUFB(msg.type->data(), 0) = type;

        msg.data = new zmq::message_t(datalen);
        if (datalen > 0)
            memcpy(msg.data->data(), data, datalen);

        if (packet)
        {
            msg.packet = new zmq::message_t(*packet, packet->length(), [](void *data, void *hint) {delete[](uint8*) data; });
        }
        else
        {
            msg.packet = new zmq::message_t(0);
        }
        web_queue.push(msg);
    }
};