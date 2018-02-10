/*
    
  This file is a part of MOGAL, a Multi-Objective Genetic Algorithm
  Library.
  
  Copyright (C) 2008 Jori Liesenborgs

  Contact: jori.liesenborgs@gmail.com

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  
  USA

*/

#include "gaserver.h"
#include "gafactory.h"
#include "gamodule.h"
#include "genome.h"
#include "geneticalgorithm.h"
#include "log.h"
#include <enut/socketwaiter.h>
#include <enut/tcppacketsocket.h>
#include <enut/tcpv4socket.h>
#include <enut/packet.h>
#include <enut/ipv4address.h>
#include <serut/memoryserializer.h>
#include <serut/dummyserializer.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <string>

#define GAHELPER_KEEPALIVETIMEOUT					10

using namespace mogal;

bool endServerLoop = false;

void signalHandler(int val)
{
	endServerLoop = true;
}

GAModule *m_pModule = 0;
GAFactory *m_pFactory = 0;
std::string m_baseDirectory;	

void sendFactoryAck(int factoryNumber, nut::TCPPacketSocket &connection, bool canHelp)
{
	uint8_t buf[sizeof(int32_t)*3];
	serut::MemorySerializer mser(0, 0, buf, sizeof(int32_t)*3);

	mser.writeInt32(GASERVER_COMMAND_FACTORYRESULT);
	mser.writeInt32(factoryNumber);
	
	int val = 0;

	if (canHelp)
		val = 1;

	mser.writeInt32(val);

	connection.write(buf, sizeof(int32_t)*3);
}

bool processPacket(nut::TCPPacketSocket &connection, const uint8_t *pData, size_t dataLength)
{
	serut::MemorySerializer mser(pData, dataLength, 0, 0);
	int32_t cmd;
	
	mser.readInt32(&cmd);

	if (cmd == GASERVER_COMMAND_FACTORY)
	{
		// unload previous module
		
		if (m_pFactory)
			delete m_pFactory;
		if (m_pModule)
			delete m_pModule;
		
		m_pFactory = 0;
		m_pModule = 0;

		// process new module
		
		std::string moduleName;
		GAModule *pModule;
		int32_t factoryNumber;

		if (!mser.readInt32(&factoryNumber))
		{
			writeLog(LOG_ERROR, "Couldn't read factory number from packet");
			return false;
		}
		
		if (!mser.readString(moduleName))
		{
			writeLog(LOG_ERROR, "Couldn't read module name from packet");
			return false;
		}

		pModule = new GAModule();
		if (!pModule->open(m_baseDirectory, moduleName))
		{
			writeLog(LOG_ERROR, "Couldn't load module %s from directory %s: %s", moduleName.c_str(),
			         m_baseDirectory.c_str(), pModule->getErrorString().c_str());
			delete pModule;
			
			sendFactoryAck(factoryNumber, connection, false);
			return true;
		}
		
		GAFactory *pFactory = pModule->createFactoryInstance();
		if (pFactory == 0)
		{
			writeLog(LOG_ERROR, "Couldn't create factory instance: %s", pModule->getErrorString().c_str());
			delete pModule;
			
			sendFactoryAck(factoryNumber, connection, false);
			return true;
		}

		GAFactoryParams *pFactoryParams = pFactory->createParamsInstance();
		if (pFactoryParams == 0)
		{
			writeLog(LOG_ERROR, "Coulnd't create factory parameters instance: %s", pFactory->getErrorString().c_str());
			delete pFactory;
			delete pModule;
			
			sendFactoryAck(factoryNumber, connection, false);
			return true;
		}

		GeneticAlgorithmParams params;
		int32_t popSize;
		
		if (!mser.readInt32(&popSize))
		{
			delete pFactoryParams;
			delete pFactory;
			delete pModule;
			
			writeLog(LOG_ERROR, "Couldn't read population size from server packet");
			return false;
		}

		if (!pFactoryParams->read(mser))
		{
			writeLog(LOG_ERROR, "Couldn't read factory parameters from server packet: %s", pFactoryParams->getErrorString().c_str());
			delete pFactoryParams;
			delete pFactory;
			delete pModule;
			
			return false;
		}

		if (!params.read(mser))
		{
			delete pFactoryParams;
			delete pFactory;
			delete pModule;

			writeLog(LOG_ERROR, "Couldn't read GA parameters from server packet");
			return false;
		}

		if (popSize <= 0)
		{
			delete pFactoryParams;
			delete pFactory;
			delete pModule;

			writeLog(LOG_ERROR, "Read invalid population size %d from server", (int)popSize);
			return false;
		}

		if (!pFactory->init(pFactoryParams))
		{
			writeLog(LOG_ERROR, "Couldn't initialize factory instance: %s", pFactory->getErrorString().c_str());
			delete pFactoryParams;
			delete pFactory;
			delete pModule;
	
			sendFactoryAck(factoryNumber, connection, false);
			return true;
		}
		
		delete pFactoryParams;
		
		m_pModule = pModule;
		m_pFactory = pFactory;

		sendFactoryAck(factoryNumber, connection, true);
	}
	else if (cmd == GASERVER_COMMAND_GENERATIONINFO)
	{
		if (m_pFactory == 0)
		{
			writeLog(LOG_ERROR, "Received generation info, but no factory has been created yet");
			return false;
		}
		if (!m_pFactory->readCommonGenerationInfo(mser))
		{
			writeLog(LOG_ERROR, "Couldn't read common generation info: %s", m_pFactory->getErrorString().c_str());
			return false;
		}
	}
	else if (cmd == GASERVER_COMMAND_CALCULATE)
	{
		int32_t facNum, numGenomes;

		if (!mser.readInt32(&facNum))
		{
			writeLog(LOG_ERROR, "Couldn't read factory number from calculation packet");
			return false;
		}

		if (!mser.readInt32(&numGenomes))
		{
			writeLog(LOG_ERROR, "Couldn't read number of genomes from calculation packet");
			return false;
		}

		if (numGenomes < 1)
		{
			writeLog(LOG_ERROR, "Read invalid number of genomes %d from calculation packet", (int)numGenomes);
			return false;
		}
		
		size_t len = sizeof(int32_t)*3 + m_pFactory->getMaximalFitnessSize()*numGenomes;
		uint8_t *pBuf = new uint8_t[len];
		serut::MemorySerializer ms(0, 0, pBuf, len);
		
		ms.writeInt32(GASERVER_COMMAND_FITNESS);
		ms.writeInt32(facNum);
		ms.writeInt32(numGenomes);
		
		for (int32_t i = 0 ; i < numGenomes ; i++)
		{
			Genome *pGenome = 0;

			if (!m_pFactory->readGenome(mser, &pGenome))
			{
				writeLog(LOG_ERROR, "Coudln't read genome from calculation packet: %s", m_pFactory->getErrorString().c_str());
				delete [] pBuf;
				return false;
			}

			pGenome->calculateFitness();
			
			// write result
		
			m_pFactory->writeGenomeFitness(ms, pGenome);
			delete pGenome;

			if (time(0) - connection.getLastWriteTime() > GAHELPER_KEEPALIVETIMEOUT) // send keepalive if necessary
			{
				uint8_t buf[sizeof(int32_t)];
				serut::MemorySerializer mser(0,0,buf,sizeof(int32_t));

				mser.writeInt32(GASERVER_COMMAND_KEEPALIVE);

				connection.write(buf, sizeof(int32_t));
			}
		}
		
		connection.write(pBuf, ms.getBytesWritten());

		delete [] pBuf;
	}
	else
	{
		writeLog(LOG_ERROR, "Received invalid command %d from server", (int)cmd);
		return false;
	}
	return true;
}

int main(int argc, char *argv[])
{
	if (argc != 5)
	{
		std::cerr << "Usage: " << std::endl;
		std::cerr << std::string(argv[0]) << " debuglevel serverIP portnumber moduledir" << std::endl; 
		return -1;
	}

	setDebugLevel(atoi(argv[1]));

	writeLog(LOG_INFO, "Starting helper");

	m_baseDirectory = std::string(argv[4]);

	uint16_t port = (uint16_t)atoi(argv[3]);
	nut::IPv4Address address;

	address.setAddress(argv[2]);
	
	nut::TCPv4Socket tcpSocket;
	nut::TCPPacketSocket connection(&tcpSocket, false, false, GASERVER_MAXPACKSIZE, sizeof(uint32_t), GASERVER_PACKID);

	writeLog(LOG_INFO, "Connecting to server");

	if (!tcpSocket.create())
	{
		writeLog(LOG_ERROR, "Couldn't create socket: %s", tcpSocket.getErrorString().c_str());
		return -1;
	}

	if (!tcpSocket.connect(address, port))
	{
		writeLog(LOG_ERROR, "Couldn't open connection to %s:%d: %s",
		         address.getAddressString().c_str(), (int)port, tcpSocket.getErrorString().c_str());
		return -1;
	}
	
	// Set socket buffer sizes

	int val = 1024*1024;
	tcpSocket.setSocketOption(SOL_SOCKET, SO_SNDBUF, &val, sizeof(int));
	val = 1024*1024;
	tcpSocket.setSocketOption(SOL_SOCKET, SO_RCVBUF, &val, sizeof(int));

	// Ok, connection opened, send identification

	writeLog(LOG_INFO, "Sending identification");
	
	uint8_t buf[sizeof(int32_t)];
	serut::MemorySerializer mser(0, 0, buf, sizeof(int32_t));

	mser.writeInt32(GASERVER_COMMAND_HELPER);

	if (!connection.write(buf, sizeof(int32_t)))
	{
		writeLog(LOG_ERROR, "Couldn't send identification packet: %s", connection.getErrorString().c_str());
		return -1;
	}

	signal(SIGTERM, signalHandler);
	signal(SIGHUP, signalHandler);
	signal(SIGQUIT, signalHandler);
	signal(SIGABRT, signalHandler);
	signal(SIGINT, signalHandler);

	// Ok, wait for incoming data from the server
	while (!endServerLoop)
	{
		if (!connection.waitForData(0, 100000))
		{
			writeLog(LOG_ERROR, "Error while waiting for incoming data: %s", connection.getErrorString().c_str());
			endServerLoop = true;
			continue;
		}

		if (connection.isDataAvailable())
		{
			size_t length = 0;

			connection.getAvailableDataLength(length);
			if (length == 0)
			{
				writeLog(LOG_ERROR, "Server closed connection");
				return -1;
			}
			else
			{	
				if (!connection.poll())
				{
					writeLog(LOG_ERROR, "Couldn't poll for new data: %s", connection.getErrorString().c_str());
					return -1;
				}
				
				nut::Packet packet;
				while (connection.read(packet))
				{
					if (packet.getLength() < 4)
					{
						writeLog(LOG_ERROR, "Received packet with invalid length %d from server", (int)packet.getLength());
						return -1;
					}
					
					if (!processPacket(connection, packet.getData(), packet.getLength()))
					{
						writeLog(LOG_ERROR, "Error while processing packet");
						return -1;
					}
				}
			}
		}

		time_t curTime = time(0);	

		if ((curTime - connection.getLastWriteTime()) > GAHELPER_KEEPALIVETIMEOUT)
		{
			uint8_t buf[sizeof(int32_t)];
			serut::MemorySerializer mser(0,0,buf,sizeof(int32_t));

			mser.writeInt32(GASERVER_COMMAND_KEEPALIVE);

			if (!connection.write(buf, sizeof(int32_t)))
			{
				writeLog(LOG_ERROR, "Couldn't write keepalive packet: %s", connection.getErrorString().c_str());
				return -1;
			}
		}
	}
	
	if (m_pFactory)
		delete m_pFactory;
	if (m_pModule)
		delete m_pModule;
	
	writeLog(LOG_INFO, "Exiting");

	return 0;
}

