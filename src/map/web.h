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
#ifndef WEB_HEADER
#define WEB_HEADER

#include "../common/socket.h"
#include "../common/sql.h"
#include "../common/mmo.h"
#include <zmq.hpp>
#include "../common/cbasetypes.h"
#include "../common/rapidjson/writer.h"
#include "../common/rapidjson/stringbuffer.h"


class CBasicPacket;

namespace web
{
	struct web_message_t
	{
		zmq::message_t* type;
		zmq::message_t* data;
		zmq::message_t* packet;
	};
    void init(const char* webIp, uint16 webPort);
	void sendJSON(rapidjson::StringBuffer& jsonBuffer);
    void send(MSGSERVTYPE type, void* data, size_t datalen, CBasicPacket* packet);
    void close();
};

#endif