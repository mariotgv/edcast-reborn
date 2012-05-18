#include "Server.h"

Server::Server(void)
{
}

Server::~Server(void)
{
	if(myEncoder)
		myEncoder->removeRef();
}

void 
Server::setEncoder(STD_PARMS) { 
	if(myEncoder) myEncoder->removeRef();
	myEncoder = Encoder::getEncoder(STD_PARMS_PASS);
	myEncoder->addRef();
}

void 
Server::startEncoder(void)
{
	myEncoder->addUse();
}

void 
Server::stopEncoder(void)
{
	myEncoder->removeUse();
}
