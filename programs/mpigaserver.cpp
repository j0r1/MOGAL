/*
    
  This file is a part of MOGAL, a Multi-Objective Genetic Algorithm
  Library.
  
  Copyright (C) 2008-2012 Jori Liesenborgs

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

#include <mpi.h>
#include "mogalconfig.h"
#include "gaserver.h"
#include "mpigeneticalgorithm.h"
#include "gafactory.h"
#include "gamodule.h"
#include "log.h"
#include <enut/tcpv4socket.h>
#include <enut/tcppacketsocket.h>
#include <enut/socketwaiter.h>
#include <enut/packet.h>
#include <serut/memoryserializer.h>
#include <serut/dummyserializer.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <iostream>
#include <string>
#include <list>

using namespace mogal;

#define GASERVER_FEEDBACKTIMEOUT						20
#define GASERVER_MAXCLOSETIME							60

bool endServerLoop = false;

void signalHandler(int val)
{
	endServerLoop = true;
}

class DistributedGeneticAlgorithm : public MPIGeneticAlgorithm
{
public:
	DistributedGeneticAlgorithm(GAFactory *pFactory, nut::TCPPacketSocket *pSock);
	~DistributedGeneticAlgorithm();
protected:
	bool onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged);
	void writeLog(int level, const char *fmt, ...)
	{
		va_list ap;

		va_start(ap, fmt);
		::writeLog(level, fmt, ap);
		va_end(ap);
	}
private:
	GAFactory *m_pFactory;
	nut::TCPPacketSocket *m_pSocket;
};

bool readPacket(nut::TCPPacketSocket *pSocket, nut::Packet &packet)
{
	if (!pSocket->poll())
		return false;

	while (!pSocket->isPacketAvailable())
	{
		if (!pSocket->waitForData(-1, -1))
			return false;
		
		size_t len = 0;

		pSocket->getAvailableDataLength(len);
		if (len == 0)
		{
			writeLog(LOG_ERROR, "Lost connection while reading packet");
			return false;
		}

		if (!pSocket->poll())
			return false;
	}
	
	return pSocket->read(packet);
}

bool writeSimpleCommand(nut::TCPPacketSocket *pSocket, int32_t commandNumber)
{
	uint8_t buf[sizeof(int32_t)];
	serut::MemorySerializer mser(0, 0, buf, sizeof(int32_t));

	mser.writeInt32(commandNumber);
	return pSocket->write(buf, sizeof(int32_t));
}

void waitForClose(nut::TCPPacketSocket *pSocket)
{
	time_t startTime = time(0);

	writeLog(LOG_INFO, "Waiting for client to close connection");
	
	while ((time(0) - startTime) < GASERVER_MAXCLOSETIME)
	{
		if (!pSocket->waitForData(0, 200000))
		{ 
			writeLog(LOG_DEBUG, "Error while waiting for client to close connection: %s",
			         pSocket->getErrorString().c_str());
			return;
		}
		
		if (pSocket->isDataAvailable())
		{
			size_t length = 0;
			
			pSocket->getAvailableDataLength(length);
			
			if (length == 0)
			{
				writeLog(LOG_DEBUG, "Client closed connection");
				return;
			}

			if (!pSocket->poll())
			{
				writeLog(LOG_DEBUG, "Error polling while waiting for client to close connection: %s",
				         pSocket->getErrorString().c_str());
				return;
			}
			
			nut::Packet packet;
			while (pSocket->read(packet))
			{
				// nothing to do, each read destroys the previously stored data
			}
		}
	}
	writeLog(LOG_INFO, "Timeout while waiting for client to close connection");
}

int loadFactory(const uint8_t *pData, size_t length, const std::string &moduleDir, GAModule &gaModule, GeneticAlgorithmParams &gaParams,
		GAFactory **ppFactory, GAFactoryParams **ppFactParams)
{
	serut::MemorySerializer mser(pData, length, 0, 0);

	int32_t facCmd = 0;
	int32_t reserved = 0;
	int32_t popSize = 0;
	GAFactory *pFactory = 0;
	GAFactoryParams *pFactParams = 0;
	std::string facName;

	mser.readInt32(&facCmd);
	mser.readInt32(&reserved);

	if (facCmd != GASERVER_COMMAND_FACTORY)
	{
		writeLog(LOG_ERROR, "Expecting factory packet, but got packet with ID %d", facCmd);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}
	if (reserved != 0)
	{
		writeLog(LOG_ERROR, "Reserved space in factory packet should be 0, but is %d", reserved);
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if (!mser.readString(facName))
	{
		writeLog(LOG_ERROR, "Error retrieving factory name from factory packet: %s", mser.getErrorString().c_str());
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if (!mser.readInt32(&popSize))
	{
		writeLog(LOG_ERROR, "Couldn't read population size from factory packet: %s", mser.getErrorString().c_str());
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if (popSize <= 0)
	{
		writeLog(LOG_ERROR, "Population size in factory packet is negative");
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if (!gaModule.open(moduleDir, facName))
	{
		writeLog(LOG_ERROR, "Couldn't load module %s in directory %s: %s",
				facName.c_str(), moduleDir.c_str(), gaModule.getErrorString().c_str());
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	pFactory = gaModule.createFactoryInstance();
	if (pFactory == 0)
	{
		writeLog(LOG_ERROR, "Couldn't create factory instance for module %s: %s",
				facName.c_str(), gaModule.getErrorString().c_str());
		gaModule.close();
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	pFactParams = pFactory->createParamsInstance();
	if (pFactParams == 0)
	{
		writeLog(LOG_ERROR, "Couldn't create factory parameters: %s", pFactory->getErrorString().c_str());
		delete pFactory;
		gaModule.close();
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if (!pFactParams->read(mser))
	{
		writeLog(LOG_ERROR, "Couldn't read factory parameters: %s", pFactParams->getErrorString().c_str());
		delete pFactory;
		delete pFactParams;
		gaModule.close();
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if (!gaParams.read(mser))
	{
		writeLog(LOG_ERROR, "Couldn't read factory parameters: %s", gaParams.getErrorString().c_str());
		delete pFactory;
		delete pFactParams;
		gaModule.close();
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if (!pFactory->init(pFactParams))
	{
		writeLog(LOG_ERROR, "Couldn't initialize factory instance: ", pFactory->getErrorString().c_str());
		delete pFactory;
		delete pFactParams;
		gaModule.close();
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	*ppFactParams = pFactParams;
	*ppFactory = pFactory;

	return popSize;
}

void runServer(nut::TCPSocket *pSocket, const std::string &moduleDir, int numProcesses)
{
	// Set socket buffer sizes and keepalive
	int val = 1024*1024;
	pSocket->setSocketOption(SOL_SOCKET,SO_SNDBUF,&val,sizeof(int));
	val = 1024*1024;
	pSocket->setSocketOption(SOL_SOCKET,SO_RCVBUF,&val,sizeof(int));
	val = 1;
	pSocket->setSocketOption(SOL_SOCKET,SO_KEEPALIVE,&val,sizeof(int));
#ifdef __APPLE__
	val = 20;
	pSocket->setSocketOption(IPPROTO_TCP, TCP_KEEPALIVE, &val, sizeof(int));
#else
	val = 60;
	pSocket->setSocketOption(SOL_TCP,TCP_KEEPIDLE,&val,sizeof(int));
	val = 20;
	pSocket->setSocketOption(SOL_TCP,TCP_KEEPINTVL,&val,sizeof(int));
	val = 10;
	pSocket->setSocketOption(SOL_TCP,TCP_KEEPCNT,&val,sizeof(int));
#endif
	socklen_t valSize = sizeof(int);
	val = 0;
	pSocket->getSocketOption(SOL_SOCKET,SO_SNDBUF,&val,&valSize);
	writeLog(LOG_DEBUG, "  SO_SNDBUF = %d", val);

	valSize = sizeof(int);
	val = 0;
	pSocket->getSocketOption(SOL_SOCKET,SO_RCVBUF,&val,&valSize);
	writeLog(LOG_DEBUG, "  SO_RCVBUF = %d", val);

	valSize = sizeof(int);
	val = 0;
	pSocket->getSocketOption(SOL_SOCKET,SO_KEEPALIVE,&val,&valSize);
	writeLog(LOG_DEBUG, "  SO_KEEPALIVE = %d", val);
#ifdef __APPLE__
	valSize = sizeof(int);
	val = 0;
	pSocket->getSocketOption(IPPROTO_TCP, TCP_KEEPALIVE,&val,&valSize);
	writeLog(LOG_DEBUG, "  TCP_KEEPALIVE = %d", val);
#else
	valSize = sizeof(int);
	val = 0;
	pSocket->getSocketOption(SOL_TCP,TCP_KEEPIDLE,&val,&valSize);
	writeLog(LOG_DEBUG, "  TCP_KEEPIDLE = %d", val);

	valSize = sizeof(int);
	val = 0;
	pSocket->getSocketOption(SOL_TCP,TCP_KEEPINTVL,&val,&valSize);
	writeLog(LOG_DEBUG, "  TCP_KEEPINTVL = %d", val);

	valSize = sizeof(int);
	val = 0;
	pSocket->getSocketOption(SOL_TCP,TCP_KEEPCNT,&val,&valSize);
	writeLog(LOG_DEBUG, "  TCP_KEEPCNT = %d", val);
#endif
	// Create packet socket

	nut::TCPPacketSocket packetSocket(pSocket, true, false, GASERVER_MAXPACKSIZE, sizeof(uint32_t), GASERVER_PACKID);

	// Read identifier
	{
		nut::Packet packet;

		if (!readPacket(&packetSocket, packet))
		{
			writeLog(LOG_ERROR, "Error reading packet from client: %s", packetSocket.getErrorString().c_str());
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		size_t length = packet.getLength();

		if (length != sizeof(int32_t))
		{
			writeLog(LOG_ERROR, "Read invalid client identifier length");
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		serut::MemorySerializer mser(packet.getData(), length, 0, 0);
		int32_t id = 0;

		mser.readInt32(&id);
		if (id != GASERVER_COMMAND_CLIENT)
		{
			writeLog(LOG_ERROR, "Read invalid client identifier %d", id);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		
		// Write confirmation

		writeLog(LOG_INFO, "Detected client connection, writing confirmation");
		writeSimpleCommand(&packetSocket, GASERVER_COMMAND_ACCEPT);
	}

	GAModule gaModule;
	GeneticAlgorithmParams gaParams;
	GAFactory *pFactory = 0;
	GAFactoryParams *pFactParams = 0;
	int popSize = 0;

	{
		nut::Packet packet;

		if (!readPacket(&packetSocket, packet))
		{
			writeLog(LOG_ERROR, "Error while trying to read factory: %s", packetSocket.getErrorString().c_str());
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		if (packet.getLength() <= sizeof(int32_t)*3)
		{
			writeLog(LOG_ERROR, "Received too little data for a factory packet");
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		popSize = loadFactory(packet.getData(), packet.getLength(), moduleDir, gaModule, gaParams, &pFactory, &pFactParams);

		// Distribute factory

		int numBytes = packet.getLength();
		MPI_Bcast(&numBytes, 1, MPI_INT, 0, MPI_COMM_WORLD);

		size_t len;
		uint8_t *pData = packet.extractData(len);
		MPI_Bcast(pData, numBytes, MPI_BYTE, 0, MPI_COMM_WORLD);

		delete [] pData;
	}

	// Run GA
 
	DistributedGeneticAlgorithm distGA(pFactory, &packetSocket);

	if (!distGA.runMPI(*pFactory, popSize, &gaParams))
	{
		writeLog(LOG_ERROR, "Error running GA: %s", distGA.getErrorString().c_str());
		delete pFactory;
		delete pFactParams;
		gaModule.close();
		MPI_Abort(MPI_COMM_WORLD, -1);
	}
	
	// Successfully finished, send final result
	
	{
		std::list<Genome *> bestGenomes;

		distGA.getBestGenomes(bestGenomes);

		size_t num = bestGenomes.size();
		size_t len = sizeof(int32_t)*2 + (pFactory->getMaximalGenomeSize()+pFactory->getMaximalFitnessSize())*num;
		uint8_t *pBuf = new uint8_t[len];

		std::list<Genome *>::const_iterator it = bestGenomes.begin();
		serut::MemorySerializer mser(0, 0, pBuf, len);
		
		mser.writeInt32(GASERVER_COMMAND_RESULT);
		mser.writeInt32(num);
		
		for (int i = 0 ; i < num ; i++, it++)
		{
			Genome *pGenome = *it;
			
			pFactory->writeGenome(mser, pGenome);
			pFactory->writeGenomeFitness(mser, pGenome);
		}
		
		packetSocket.write(pBuf, mser.getBytesWritten());
		
		delete [] pBuf;
	
		// end of communication, wait a while for the client to close the connection
		waitForClose(&packetSocket);
	}

	// Clean up

	distGA.clearBestGenomes(); // Must be cleared before factory is destroyed, and definitely before it is unloaded from memory

	delete pFactory;
	delete pFactParams;
	gaModule.close();
}

DistributedGeneticAlgorithm::DistributedGeneticAlgorithm(GAFactory *pFactory, nut::TCPPacketSocket *pSocket)
{
	m_pFactory = pFactory;
	m_pSocket = pSocket;
}

DistributedGeneticAlgorithm::~DistributedGeneticAlgorithm()
{
}

bool DistributedGeneticAlgorithm::onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged)
{
	if (!MPIGeneticAlgorithm::onAlgorithmLoop(factory, generationInfoChanged))
		return false;

	// Provide some feedback
	
	time_t curTime = time(NULL);

	if ((curTime - m_pSocket->getLastWriteTime()) > GASERVER_FEEDBACKTIMEOUT)
	{
		std::list<Genome *> bestGenomes;

		getBestGenomes(bestGenomes);
			
		if (bestGenomes.empty())
		{
			if (!writeSimpleCommand(m_pSocket, GASERVER_COMMAND_KEEPALIVE))
			{
				setErrorString("Couldn't write keepalive message");
				return false;
			}
		}
		else
		{
			// Write best genomes

			size_t num = bestGenomes.size();
			size_t len = sizeof(int32_t)*2 + (m_pFactory->getMaximalGenomeSize()+m_pFactory->getMaximalFitnessSize())*num;
			uint8_t *pBuf = new uint8_t[len];
			std::list<Genome *>::const_iterator it = bestGenomes.begin();
			serut::MemorySerializer mser(0, 0, pBuf, len);
			
			mser.writeInt32(GASERVER_COMMAND_CURRENTBEST);
			mser.writeInt32(num);

			for (int i = 0 ; i < num ; i++, it++)
			{
				m_pFactory->writeGenome(mser, (*it));
				m_pFactory->writeGenomeFitness(mser, (*it));
			}

			if (!m_pSocket->write(pBuf, mser.getBytesWritten()))
			{
				setErrorString("Couldn't write best genomes to client");
				return false;
			}
			delete [] pBuf;
		}
	}

	return true;
}

void runHelper(const std::string &moduleDir)
{
	int numBytes = 0;
	MPI_Bcast(&numBytes, 1, MPI_INT, 0, MPI_COMM_WORLD);

	std::vector<uint8_t> factoryPacketBuffer(numBytes);
	MPI_Bcast(&(factoryPacketBuffer[0]), numBytes, MPI_BYTE, 0, MPI_COMM_WORLD);

	GAModule gaModule;
	GeneticAlgorithmParams gaParams;
	GAFactory *pFactory = 0;
	GAFactoryParams *pFactParams = 0;

	loadFactory(&(factoryPacketBuffer[0]), numBytes, moduleDir, gaModule, gaParams, &pFactory, &pFactParams);

	MPIGeneticAlgorithm mpiGA;

	if (!mpiGA.runMPIHelper(*pFactory))
	{
		writeLog(LOG_ERROR, "Error running MPI GA helper: %s", pFactory->getErrorString().c_str());
		delete pFactory;
		delete pFactParams;
		gaModule.close();
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	delete pFactory;
	delete pFactParams;
	gaModule.close();
}

int main(int argc, char **argv)
{
	int worldSize, myRank;

	MPI_Init(&argc, &argv);
	MPI_Comm_size( MPI_COMM_WORLD, &worldSize);

	if (worldSize <= 1)
	{
		std::cerr << "Need more than one process to be able to work" << std::endl;
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

	if (argc != 4) 
	{
		if (myRank == 0)
		{
			std::cerr << "Usage: " << std::endl;
			std::cerr << std::string(argv[0]) << " debuglevel portnumber moduledir" << std::endl; 
		}
		MPI_Abort(MPI_COMM_WORLD, -1);
	}
	setDebugLevel(atoi(argv[1]));

	std::string moduleDir(argv[3]);

	if (myRank == 0)
	{
		writeLog(LOG_INFO, "Starting server");

		signal(SIGTERM, signalHandler);
		signal(SIGHUP, signalHandler);
		signal(SIGQUIT, signalHandler);
		signal(SIGABRT, signalHandler);
		signal(SIGINT, signalHandler);

		nut::TCPv4Socket listenSocket;
		uint16_t listenPort = atoi(argv[2]);

		if (!listenSocket.create(listenPort))
		{
			writeLog(LOG_ERROR, "Error creating server at port %d: %s", listenPort, listenSocket.getErrorString().c_str());
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		if (listenPort == 0 && getDebugLevel() < 2)
			std::cout << "Listen socket created at port " << listenSocket.getLocalPortNumber() << std::endl;

		if (!listenSocket.listen(1))
		{
			writeLog(LOG_ERROR, "Couldn't place socket in listen mode: %s", listenSocket.getErrorString().c_str());
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		writeLog(LOG_INFO, "Listen socket created at port %d", (int)listenSocket.getLocalPortNumber());

		nut::SocketWaiter waiter;

		waiter.addSocket(listenSocket);

		writeLog(LOG_INFO, "Waiting for incoming connection");
		if (!waiter.wait(60, 0))
		{
			writeLog(LOG_ERROR, "Error waiting for incoming connection: %s", waiter.getErrorString().c_str());
			MPI_Abort(MPI_COMM_WORLD, -1);	
		}

		if (listenSocket.isDataAvailable())
		{
			nut::TCPSocket *pClientSock = 0;

			if (listenSocket.accept(&pClientSock))
			{
				listenSocket.destroy(); // make sure we don't accept any other connections
				runServer(pClientSock, moduleDir, worldSize); // deletes pClientSock when done
			}
			else
			{
				writeLog(LOG_ERROR, "Detected incoming connection, but couldn't accept it: %s", listenSocket.getErrorString().c_str());
				MPI_Abort(MPI_COMM_WORLD, -1);	
			}
		}
		else
		{
			writeLog(LOG_ERROR, "No incoming connection within 60 seconds, exiting");
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		writeLog(LOG_INFO, "Exiting");
	}
	else // Helper
	{
		runHelper(moduleDir);
	}
	
	MPI_Finalize();

	return 0;
}

