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
#include "geneticalgorithm.h"
#include "gafactory.h"
#include "gamodule.h"
#include "log.h"
#include <enut/tcpv4socket.h>
#include <enut/tcppacketsocket.h>
#include <enut/socketwaiter.h>
#include <enut/packet.h>
#include <serut/memoryserializer.h>
#include <serut/dummyserializer.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <list>

using namespace mogal;

#define GASERVER_READTIMEOUT							600
#define GASERVER_FEEDBACKTIMEOUT						20
#define GASERVER_MAXCALCTIME							30.0
#define GASERVER_MAXCLOSETIME							60
#define GASERVER_RECALCTIMEOUT							5

bool endServerLoop = false;

void signalHandler(int val)
{
	endServerLoop = true;
}

inline double getCurrentTime()
{
	struct timeval tv;

	gettimeofday(&tv, 0);

	return (((double)tv.tv_sec)+((double)tv.tv_usec/1000000.0));
}

class ConnectionInfo;

class ServerAlgorithm : public GeneticAlgorithm
{
public:
	static ServerAlgorithm *ServerAlgorithm::instance()						{ return m_pInstance; }

	ServerAlgorithm(nut::TCPSocket &listenSocket, const std::string &baseDir);
	~ServerAlgorithm();

	void run();

	ConnectionInfo *getClientConnection() const							{ return m_pClientConnection; }
	void setClientConnection(ConnectionInfo *pConnection)						{ m_pClientConnection = pConnection; }

	size_t getNumberOfConnections() const								{ return m_connections.size(); }
	bool loadFactory(serut::SerializationInterface &si, uint8_t *pData, size_t dataLength);
	int getFactoryNumber() const									{ return m_factoryNumber; }

	const GAFactory *getFactory() const								{ return m_pFactory; }
	int getGenomeToProcess()									{ if (m_genomesToProcess.empty()) return -1; int val = *(m_genomesToProcess.begin()); m_genomesToProcess.pop_front(); return val; }
	bool gotCurrentGenomes() const									{ return (m_pCurrentGenomes == 0)?false:true; }
	GenomeWrapper *getGenome(int i)									{ return &((*m_pCurrentGenomes)[i]); }
	int getCurrentGenerationNumber() const								{ return m_currentGenerationNumber; }
	int getGenerationInfoCount() const								{ return m_generationInfoCount; }

	void increaseCalculatedGenomeCount(int num)							{ m_genomesCalculated += num; }
	void recalculate(int i)										{ m_genomesToProcess.push_back(i); }

	const uint8_t *getFactoryData() const								{ return m_pFactoryData; }
	size_t getFactoryDataLength() const								{ return m_factoryDataLength; }

	size_t getPopulationSize() const								{ return m_populationSize; }
private:
	void distributeExcessGenomes();
	void redistributeGenomes(double startTime, double endTime);

	bool calculateFitness(std::vector<GenomeWrapper> &population);
	bool onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged);

	nut::TCPSocket &m_listenSocket;
	std::list<ConnectionInfo *> m_connections;
	ConnectionInfo *m_pClientConnection;
	time_t m_prevRecalcTime;

	GAModule *m_pModule;
	GAFactory *m_pFactory;	
	GAFactoryParams *m_pFactoryParams;
	GeneticAlgorithmParams *m_pGAParams;
	std::string m_factoryFileName;
	size_t m_populationSize;
	
	const uint8_t *m_pFactoryData;
	size_t m_factoryDataLength;
	int m_factoryNumber;
	int m_generationInfoCount;
	std::list<int> m_genomesToProcess;
	int m_genomesCalculated;

	std::vector<GenomeWrapper> *m_pCurrentGenomes;
	int m_currentGenerationNumber;
	
	std::string m_baseDirectory;

	static ServerAlgorithm *m_pInstance;
};

ServerAlgorithm *ServerAlgorithm::m_pInstance = 0;

class ConnectionInfo
{
public:
	enum ConnectionState { Unidentified, Idle, Calculating };
	
	ConnectionInfo(nut::TCPPacketSocket *pSocket);
	~ConnectionInfo();
	
	nut::TCPPacketSocket *getSocket()								{ return m_pSocket; }
	
	bool processPacket(uint8_t *pData, size_t dataLength, bool *pDeleteData);
	bool processPacketWhenRunning(uint8_t *pData, size_t dataLength, bool *pDeleteData);
	bool sendData(int *pCantHelpCount);
	bool giveFeedback();
	void sendUnavailable();

	void waitForClose();
	ConnectionState getConnectionState() const							{ return m_state; }
	bool canHelp() const										{ return m_canHelp; }

	void resetNumberOfGenomesWritten()								{ m_numberOfGenomesWritten = 0; }
	int getNumberOfGenomesWritten() const								{ return m_numberOfGenomesWritten; }
	void setNumberOfGenomesToWrite(int n)								{ m_numberOfGenomesToWrite = n; }
	void addNumberOfGenomesToWrite(int a)								{ m_numberOfGenomesToAdd += a; }
	int getNumberOfGenomesToWrite() const								{ return m_numberOfGenomesToWrite; }
	int getAddedNumberOfGenomes() const								{ return m_numberOfGenomesToAdd; }
	void processAddedNumberOfGenomes()								{ m_numberOfGenomesToWrite += m_numberOfGenomesToAdd; m_numberOfGenomesToAdd = 0; }

	double getLastReceiveFitnessTime() const							{ return m_lastReceiveFitnessTime; }
	void resetLastReceiveFitnessTime()								{ m_lastReceiveFitnessTime = -1; }
private:
	nut::TCPPacketSocket *m_pSocket;
	ConnectionState m_state;
	bool writeSimpleCommand(int32_t commandNumber);

	int m_lastWrittenFactoryNumber;
	int m_ackFactoryNumber;
	bool m_canHelp;

	int m_lastWrittenGenerationInfo;
	int m_lastWrittenGenerationNumber;
	
	std::list<int> m_lastWrittenGenomeNumbers;
	int m_numberOfGenomesToWrite;
	int m_numberOfGenomesWritten;
	int m_numberOfGenomesToAdd;
	double m_lastReceiveFitnessTime;
};

ConnectionInfo::ConnectionInfo(nut::TCPPacketSocket *pSocket)
{
	writeLog(LOG_INFO, "Accepted incoming connection from %s:%d", 
		 pSocket->getBaseSocket()->getDestinationAddress()->getAddressString().c_str(),
		 (int)pSocket->getBaseSocket()->getDestinationPort());

	// Set socket buffer sizes
	int val = 1024*1024;
	pSocket->setSocketOption(SOL_SOCKET,SO_SNDBUF,&val,sizeof(int));
	val = 1024*1024;
	pSocket->setSocketOption(SOL_SOCKET,SO_RCVBUF,&val,sizeof(int));

	m_pSocket = pSocket;
	m_state = Unidentified;
	
	m_lastWrittenFactoryNumber = -1;
	m_ackFactoryNumber = -1;
	m_canHelp = false;
	m_lastWrittenGenerationInfo = -1;
	m_lastWrittenGenerationNumber = -1;

	m_numberOfGenomesToWrite = 1;
	m_numberOfGenomesWritten = 0;
	m_numberOfGenomesToAdd = 0;
}

ConnectionInfo::~ConnectionInfo()
{
	writeLog(LOG_INFO, "Removing connection from %s:%d", 
		 m_pSocket->getBaseSocket()->getDestinationAddress()->getAddressString().c_str(),
		 (int)m_pSocket->getBaseSocket()->getDestinationPort());

	// if calculating, add genome back to the list
	if (m_state == Calculating)
	{
		ServerAlgorithm *pAlg = ServerAlgorithm::instance();
		if (pAlg->gotCurrentGenomes())
		{
			if (pAlg->getFactoryNumber() == m_lastWrittenFactoryNumber && pAlg->getCurrentGenerationNumber() == m_lastWrittenGenerationNumber)
			{
				std::list<int>::const_iterator it;

				for (it = m_lastWrittenGenomeNumbers.begin() ; it != m_lastWrittenGenomeNumbers.end() ; it++)
				{
					pAlg->recalculate(*it);
					writeLog(LOG_DEBUG, "Genome %d will need to be recalculated", (*it));
				}
			}
		}
	}
	
	delete m_pSocket;
}

bool ConnectionInfo::writeSimpleCommand(int32_t commandNumber)
{
	uint8_t buf[sizeof(int32_t)];
	serut::MemorySerializer mser(0, 0, buf, sizeof(int32_t));

	mser.writeInt32(commandNumber);
	return m_pSocket->write(buf, sizeof(int32_t));
}

void ConnectionInfo::sendUnavailable()
{
	writeSimpleCommand(GASERVER_COMMAND_NOHELPERS);
}

bool ConnectionInfo::processPacket(uint8_t *pData, size_t dataLength, bool *pDeleteData)
{
	*pDeleteData = true;

	if (dataLength < sizeof(int32_t))
	{
		writeLog(LOG_DEBUG, "processPacket: invalid data length %d", (int)dataLength);
		return false;
	}

	serut::MemorySerializer mser(pData, dataLength, 0, 0);
	ServerAlgorithm *pAlg = ServerAlgorithm::instance();
	int32_t cmd;
	
	mser.readInt32(&cmd);

	if (m_state == Unidentified)
	{
		if (cmd == GASERVER_COMMAND_CLIENT)
		{
			if (pAlg->getClientConnection() != 0) // there aleady is a client
			{
				writeSimpleCommand(GASERVER_COMMAND_BUSY); // tell client we're busy
				writeLog(LOG_DEBUG, "processPacket: there already is a client");
				return false;
			}
			else // ok, there isn't any client yet
			{
				if (pAlg->getNumberOfConnections() == 1) // this is the only connection
				{
					sendUnavailable();
					writeLog(LOG_DEBUG, "processPacket: no helpers available");
					return false;
				}
				else
				{
					if (!writeSimpleCommand(GASERVER_COMMAND_ACCEPT))
					{
						writeLog(LOG_DEBUG, "processPacket: can't write accept");
						return false;
					}

					pAlg->setClientConnection(this);
					m_state = Idle;

					return true;
				}
			}
		}
		else if (cmd == GASERVER_COMMAND_HELPER)
		{
			m_state = Idle;
			return true;
		}
		else // received invalid packet
		{
			writeLog(LOG_DEBUG, "processPacket: received invalid command %d", cmd);
			return false;
		}
	}
	else
	{
		if (cmd == GASERVER_COMMAND_KEEPALIVE)
		{
			if (pAlg->getClientConnection() == this)
			{
				if (pAlg->getFactory() == 0) // no factory information has been received yet
				{
					writeLog(LOG_DEBUG, "processPacket: read invalid keepalive packet (no factory yet)");
					return false;
				}
			}
			if (dataLength != sizeof(int32_t))
			{
				writeLog(LOG_DEBUG, "processPacket: read invalid data length in keepalive packet");
				return false;
			}
			return true;
		}
		
		if (pAlg->getClientConnection() == this) // we're the client
		{
			if (cmd == GASERVER_COMMAND_FACTORY)
			{
				if (pAlg->getFactory() != 0)
				{
					writeLog(LOG_DEBUG, "processPacket: received factory command but we already have a factory");
					return false;
				}
				if (!pAlg->loadFactory(mser, pData, dataLength))
				{
					writeLog(LOG_DEBUG, "processPacket: Unable to load the factory");
					return false;
				}
				// storing the factory data!
				*pDeleteData = false;
				return true;
			}

			writeLog(LOG_DEBUG, "processPacket: unexpected packet from client connection");

			// clients shouldn't write anything except keepalive packets
			return false;
		}
		else // we're a helper
		{
			if (cmd == GASERVER_COMMAND_FITNESS) // it's possible that we're receiving a result from a previous request
			{                                    // (if the client disconnected prematurely
				// mark helper as idle
				m_lastWrittenGenomeNumbers.clear();
				m_state = Idle;
				return true;
			}
			if (cmd == GASERVER_COMMAND_FACTORYRESULT) // same as above
			{
				// mark helper as idle
				m_state = Idle;
				return true;
			}
			writeLog(LOG_DEBUG, "processPacket: unexpected packet from helper connection");
			return false;
		}
	}

	return false;
}

bool ConnectionInfo::processPacketWhenRunning(uint8_t *pData, size_t dataLength, bool *pDeleteData)
{
	*pDeleteData = true;

	if (dataLength < sizeof(int32_t))
		return false;

	serut::MemorySerializer mser(pData, dataLength, 0, 0);
	ServerAlgorithm *pAlg = ServerAlgorithm::instance();
	int32_t cmd;
	
	mser.readInt32(&cmd);

	if (m_state == Unidentified)
	{
		if (cmd == GASERVER_COMMAND_CLIENT)
		{
			// there aleady is a client !
			writeSimpleCommand(GASERVER_COMMAND_BUSY); // tell client we're busy
			return false;
		}
		else if (cmd == GASERVER_COMMAND_HELPER)
		{
			m_state = Idle;
		}
		else // received invalid packet
			return false;
	}
	else
	{
		if (cmd == GASERVER_COMMAND_KEEPALIVE)
		{
			if (dataLength != sizeof(int32_t))
				return false;
			return true;
		}
		
		if (pAlg->getClientConnection() == this) // we're the client
		{
			// clients shouldn't write anything except keepalive packets
			return false;
		}
		else // we're a helper: the only things we should receive are fitness calculation results and factory feedback
		{
			if (cmd == GASERVER_COMMAND_FITNESS) 
			{
				if (m_state != Calculating) // invalid packet
					return false;

				int32_t facNum, numGenomes;

				if (!mser.readInt32(&facNum))
					return false;
				if (!mser.readInt32(&numGenomes))
					return false;

				if (numGenomes < 1 || numGenomes != m_lastWrittenGenomeNumbers.size())
					return false;

				// check if it's not some other result
				if (facNum == pAlg->getFactoryNumber())
				{
					std::list<int>::const_iterator it;

					it = m_lastWrittenGenomeNumbers.begin();
					for (int32_t i = 0 ; i < numGenomes ; i++)
					{
						int lastWrittenGenomeNumber = (*it);
						
						it++;
						
						GenomeWrapper *pGenomeWrapper = pAlg->getGenome(lastWrittenGenomeNumber);

						// read result and store it into genome

						if (!pAlg->getFactory()->readGenomeFitness(mser, pGenomeWrapper->getGenome()))
							return false;
					}

					pAlg->increaseCalculatedGenomeCount(numGenomes);
				}
					
				// mark helper as idle
				m_lastWrittenGenomeNumbers.clear();
				m_state = Idle;
				m_lastReceiveFitnessTime = getCurrentTime();
				return true;
			}
			else if (cmd == GASERVER_COMMAND_FACTORYRESULT)
			{
				// Note: it is possible that the state has already been set to
				// Calculating here!
			
				if (m_lastWrittenFactoryNumber == m_ackFactoryNumber) // don't expect a factory ack at this stage
					return false;
				
				if (dataLength != sizeof(int32_t)*3)
					return false;
				
				int32_t facNum, available;

				if (!mser.readInt32(&facNum))
					return false;
				if (!mser.readInt32(&available))
					return false;
				if (available != 0 && available != 1)
					return false;
				
				if (facNum == m_lastWrittenFactoryNumber)
				{
					m_ackFactoryNumber = m_lastWrittenFactoryNumber;
					if (available)
						m_canHelp = true;
					else
						m_canHelp = false;
				}
				
				return true;
			}
			return false;
		}
	}

	return true;
}

bool ConnectionInfo::sendData(int *pCantHelpCount)
{
	if (m_state == Calculating) // node is not available now
		return true;
	
	ServerAlgorithm *pAlg = ServerAlgorithm::instance();
	
	// check if we need to send new factory data
	
	if (m_lastWrittenFactoryNumber != pAlg->getFactoryNumber())
	{
		if (!m_pSocket->write(pAlg->getFactoryData(), pAlg->getFactoryDataLength()))
			return false;
		m_lastWrittenFactoryNumber = pAlg->getFactoryNumber();
		return true;
	}
	if (m_lastWrittenFactoryNumber != m_ackFactoryNumber) // wait for factory ack
		return true;

	if (!m_canHelp) // this helper can't help us
	{
		(*pCantHelpCount)++;
		return true;
	}

	// check if we need to send an update of generation info
	
	if (pAlg->getGenerationInfoCount() != m_lastWrittenGenerationInfo)
	{
		// TODO: use a dynamic memory buffer

		serut::DummySerializer ds;

		ds.writeInt32(GASERVER_COMMAND_GENERATIONINFO);
		pAlg->getFactory()->writeCommonGenerationInfo(ds);

		uint8_t *pBuf = new uint8_t[ds.getBytesWritten()];
		serut::MemorySerializer mser(0, 0, pBuf, ds.getBytesWritten());

		mser.writeInt32(GASERVER_COMMAND_GENERATIONINFO);
		pAlg->getFactory()->writeCommonGenerationInfo(mser);

		if (!m_pSocket->write(pBuf, ds.getBytesWritten()))
		{
			delete [] pBuf;
			return false;
		}
		
		delete [] pBuf;
		m_lastWrittenGenerationInfo = pAlg->getGenerationInfoCount();

		// note: we do not return at this point, but see if we can also send a
		//       genome to be calculated
	}

	// If available, send a genome to be calculated

	int genomeNum = -1;
	int count = 0;

	m_lastWrittenGenomeNumbers.clear();
	while (m_numberOfGenomesWritten < m_numberOfGenomesToWrite && (genomeNum = pAlg->getGenomeToProcess()) >= 0)
	{
		m_numberOfGenomesWritten++;
		count++;
		m_lastWrittenGenomeNumbers.push_back(genomeNum);
	}

	if (count == 0) // nothing to do
		return true;

	size_t len = sizeof(int32_t)*3 + pAlg->getFactory()->getMaximalGenomeSize()*count;
	uint8_t *pBuf = new uint8_t[len];

	serut::MemorySerializer mser(0, 0, pBuf, len);

	mser.writeInt32(GASERVER_COMMAND_CALCULATE);
	mser.writeInt32(pAlg->getFactoryNumber());
	mser.writeInt32(count);

	std::list<int>::const_iterator it;
	
	for (it = m_lastWrittenGenomeNumbers.begin() ; it != m_lastWrittenGenomeNumbers.end() ; it++)
		pAlg->getFactory()->writeGenome(mser, pAlg->getGenome(*it)->getGenome());

	if (!m_pSocket->write(pBuf, mser.getBytesWritten()))
	{
		delete [] pBuf;

		for (it = m_lastWrittenGenomeNumbers.begin() ; it != m_lastWrittenGenomeNumbers.end() ; it++)
			pAlg->recalculate(*it);
		m_lastWrittenGenomeNumbers.clear();
		return false;
	}
	
	delete [] pBuf;
	
	m_lastWrittenGenerationNumber = pAlg->getCurrentGenerationNumber();
	m_state = Calculating;
	
	return true;
}

bool ConnectionInfo::giveFeedback()
{
	time_t lastTime = m_pSocket->getLastWriteTime();
	time_t curTime = time(0);
	
	if (curTime-lastTime > GASERVER_FEEDBACKTIMEOUT)
	{
		ServerAlgorithm *pAlg = ServerAlgorithm::instance();
		std::list<Genome *> bestGenomes;

		pAlg->getBestGenomes(bestGenomes);
			
		if (bestGenomes.empty())
		{
			if (!writeSimpleCommand(GASERVER_COMMAND_KEEPALIVE))
				return false;
		}
		else
		{
			// Write best genomes

			size_t num = bestGenomes.size();
			size_t len = sizeof(int32_t)*2 + (pAlg->getFactory()->getMaximalGenomeSize()+pAlg->getFactory()->getMaximalFitnessSize())*num;
			uint8_t *pBuf = new uint8_t[len];
			std::list<Genome *>::const_iterator it = bestGenomes.begin();
			serut::MemorySerializer mser(0, 0, pBuf, len);
			
			mser.writeInt32(GASERVER_COMMAND_CURRENTBEST);
			mser.writeInt32(num);

			for (int i = 0 ; i < num ; i++, it++)
			{
				pAlg->getFactory()->writeGenome(mser, (*it));
				pAlg->getFactory()->writeGenomeFitness(mser, (*it));
			}

			m_pSocket->write(pBuf, mser.getBytesWritten());

			delete [] pBuf;
		}
	}
	return true;
}

void ConnectionInfo::waitForClose()
{
	time_t startTime = time(0);

	writeLog(LOG_INFO, "Waiting for client to close connection");
	
	while ((time(0) - startTime) < GASERVER_MAXCLOSETIME)
	{
		if (!m_pSocket->waitForData(0, 200000))
		{ 
			writeLog(LOG_DEBUG, "Error while waiting for client to close connection: %s",
			         m_pSocket->getErrorString().c_str());
			return;
		}
		
		if (m_pSocket->isDataAvailable())
		{
			size_t length = 0;
			
			m_pSocket->getAvailableDataLength(length);
			
			if (length == 0)
			{
				writeLog(LOG_DEBUG, "Client closed connection");
				return;
			}
			if (!m_pSocket->poll())
			{
				writeLog(LOG_DEBUG, "Error polling while waiting for client to close connection: %s",
				         m_pSocket->getErrorString().c_str());
				return;
			}
			
			nut::Packet packet;
			while (m_pSocket->read(packet))
			{
				// nothing to do, each read destroys the previously stored data
			}
		}
	}
	writeLog(LOG_INFO, "Timeout while waiting for client to close connection");
}

ServerAlgorithm::ServerAlgorithm(nut::TCPSocket &listenSocket, const std::string &baseDir) : m_listenSocket(listenSocket)
{ 
	if (m_pInstance != 0)
	{
		std::cerr << "Only one instance can exist at a time" << std::endl;
		exit(-1);
	}
	m_pInstance = this;
	m_baseDirectory = baseDir;
	m_factoryNumber = 0;
	m_generationInfoCount = 0;
	m_currentGenerationNumber = 0;
	m_pClientConnection = 0;
}

ServerAlgorithm::~ServerAlgorithm()
{
}

void ServerAlgorithm::run()
{
	m_pFactory = 0;
	m_pGAParams = 0;
	m_pFactoryParams = 0;
	m_pModule = 0;
	m_pFactoryData = 0;
	m_factoryDataLength = 0;
	m_factoryNumber++;
	
	// wait till we've received our genetic algorithm factory info
	while (!m_pFactory && !endServerLoop)
	{
		nut::SocketWaiter waiter;
		std::list<ConnectionInfo *>::iterator it;

		waiter.addSocket(m_listenSocket);
		for (it = m_connections.begin() ; it != m_connections.end() ; it++)
		{
			ConnectionInfo *pInf = *it;
			waiter.addSocket(*(pInf->getSocket()));
		}

		if (!waiter.wait(1, 0))
		{
			writeLog(LOG_ERROR, "Error waiting for incoming data: %s", waiter.getErrorString().c_str());
			endServerLoop = true;
			return;
		}

		// Check for new connections
		
		if (m_listenSocket.isDataAvailable())
		{
			nut::TCPSocket *pTCPSocket = 0;
			
			if (m_listenSocket.accept(&pTCPSocket))
			{
				nut::TCPPacketSocket *pPacketSocket = new nut::TCPPacketSocket(pTCPSocket, true, false, GASERVER_MAXPACKSIZE, sizeof(uint32_t), GASERVER_PACKID);
				ConnectionInfo *pInf = new ConnectionInfo(pPacketSocket);
				
				m_connections.push_back(pInf);
			}
		}

		// Timeout old connections
		
		time_t curTime = time(0);
		
		it = m_connections.begin();
		while (it != m_connections.end())
		{
			ConnectionInfo *pInf = *it;

			if ((curTime - pInf->getSocket()->getLastReadTime()) > GASERVER_READTIMEOUT)
			{
				writeLog(LOG_DEBUG, "Timeout on connection");
				
				it = m_connections.erase(it);
				if (pInf == m_pClientConnection)
					m_pClientConnection = 0;
				delete pInf;
			}
			else
				it++;
		}

		// Process incoming packets
	
		it = m_connections.begin();
		while (it != m_connections.end())
		{
			ConnectionInfo *pInf = *it;
			
			if (pInf->getSocket()->isDataAvailable())
			{
				bool closed = false;
				size_t length = 0;
				
				pInf->getSocket()->getAvailableDataLength(length);
				
				if (length == 0)
					closed = true;
				else
				{
					if (!pInf->getSocket()->poll())
						closed = true;
				}
				
				if (closed)
				{
					writeLog(LOG_DEBUG, "Detected closed connection");
					
					if (pInf == m_pClientConnection)
						m_pClientConnection = 0;
					it = m_connections.erase(it);
					delete pInf;
				}
				else // connection not closed, process received packet (if available)
				{
					nut::Packet packet;
					bool gotError = false;
					
					while (!gotError && pInf->getSocket()->read(packet))
					{
						bool deleteData;
						size_t length = 0;
						uint8_t *pData = packet.extractData(length);

						if (!pInf->processPacket(pData, length, &deleteData))
							gotError = true;
						if (deleteData)
							delete [] pData;
					}

					if (gotError)
					{
						// some inconsistency was detected, remove connection

						writeLog(LOG_DEBUG, "Detected inconsistency while processing packets, removing connection");
		
						if (pInf == m_pClientConnection)
							m_pClientConnection = 0;
						it = m_connections.erase(it);
						delete pInf;
					}
					else
						it++;
				}
			}
			else
				it++;
		}
	}

	bool retval;
	
	if (m_pFactory) // ok, got a factory, now run the algorithm
	{
		std::list<ConnectionInfo *>::iterator it;

		for (it = m_connections.begin() ; it != m_connections.end() ; it++)
		{
			ConnectionInfo *pInf = *it;
			
			pInf->setNumberOfGenomesToWrite(1);
		}
		
		m_prevRecalcTime = time(0);

		retval = GeneticAlgorithm::run(*m_pFactory, m_populationSize, m_pGAParams);
		
		if (retval && m_pClientConnection) // we can report the result to the client
		{
			// Write result

			std::list<Genome *> bestGenomes;

			getBestGenomes(bestGenomes);

			size_t num = bestGenomes.size();
			size_t len = sizeof(int32_t)*2 + (m_pFactory->getMaximalGenomeSize()+m_pFactory->getMaximalFitnessSize())*num;
			uint8_t *pBuf = new uint8_t[len];
			std::list<Genome *>::const_iterator it = bestGenomes.begin();
			serut::MemorySerializer mser(0, 0, pBuf, len);
			
			mser.writeInt32(GASERVER_COMMAND_RESULT);
			mser.writeInt32(num);
			
			for (int i = 0 ; i < num ; i++, it++)
			{
				Genome *pGenome = *it;
				
				m_pFactory->writeGenome(mser, pGenome);
				m_pFactory->writeGenomeFitness(mser, pGenome);
			}
			
			m_pClientConnection->getSocket()->write(pBuf, mser.getBytesWritten());

			delete [] pBuf;

			// end of communication, wait a while for the client to close the connection
			m_pClientConnection->waitForClose();
		}
	}
				
	if (m_pClientConnection)
	{
		std::list<ConnectionInfo *>::iterator it;
		bool found = false;
		
		it = m_connections.begin();
		while (it != m_connections.end() && !found)
		{
			if (*it == m_pClientConnection)
			{
				m_connections.erase(it);
				found = true;
			}
			else
				it++;
		}

		writeLog(LOG_INFO, "Removing client connection at the end of the GA run");
			
		delete m_pClientConnection;
		m_pClientConnection = 0;
	}
	
	if (m_pFactory)
		delete m_pFactory;
	if (m_pFactoryParams)
		delete m_pFactoryParams;
	if (m_pGAParams)
		delete m_pGAParams;
	
	clearBestGenomes(); // make sure we delete the best genome before we close its factory module
	if (m_pModule)
		delete m_pModule;

	if (m_pFactoryData)
		delete [] m_pFactoryData;
}

bool ServerAlgorithm::loadFactory(serut::SerializationInterface &si, uint8_t *pData, size_t dataLength)
{
	std::string moduleName;
	int32_t facNum;

	if (!si.readInt32(&facNum))
	{
		writeLog(LOG_ERROR, "Couldn't read the factory number");
		return false;
	}

	if (facNum != 0)
	{
		writeLog(LOG_ERROR, "The received factory number is not zero");
		return false;
	}

	if (!si.readString(moduleName))
	{
		writeLog(LOG_ERROR, "Unable to read module name");
		return false;
	}

	GAModule *pModule = new GAModule();

	if (!pModule->open(m_baseDirectory, moduleName))
	{
		writeLog(LOG_ERROR, "Couldn't load module %s in directory %s: %s",
		         moduleName.c_str(), m_baseDirectory.c_str(), pModule->getErrorString().c_str());
		delete pModule;
		return false;
	}

	GAFactory *pFactory = pModule->createFactoryInstance();
	
	if (pFactory == 0)
	{
		writeLog(LOG_ERROR, "Couldn't create factory instance: %s", pModule->getErrorString().c_str());
		delete pModule;
		return false;
	}

	GAFactoryParams *pFactoryParams = pFactory->createParamsInstance();
	if (pFactoryParams == 0)
	{
		writeLog(LOG_ERROR, "Coulnd't create factory parameters instance: %s", pFactory->getErrorString().c_str());
		delete pFactory;
		delete pModule;
		return false;
	}

	GeneticAlgorithmParams *pParams = new GeneticAlgorithmParams();
	int32_t popSize;
	
	if (!si.readInt32(&popSize))
	{
		writeLog(LOG_ERROR, "Couldn't read population size: ", si.getErrorString().c_str());
		delete pParams;
		delete pFactoryParams;
		delete pFactory;
		delete pModule;
		return false;
	}

	if (!pFactoryParams->read(si))
	{
		writeLog(LOG_ERROR, "Couldn't read factory parameters: %s", pFactoryParams->getErrorString().c_str());
		delete pParams;
		delete pFactoryParams;
		delete pFactory;
		delete pModule;
		return false;
	}

	if (!pParams->read(si))
	{
		writeLog(LOG_ERROR, "Couldn't read GA parameters: ", pParams->getErrorString().c_str());
		delete pParams;
		delete pFactoryParams;
		delete pFactory;
		delete pModule;
		return false;
	}

	if (popSize <= 0)
	{
		writeLog(LOG_ERROR, "Read invalid population size %d", (int)popSize);
		delete pParams;
		delete pFactoryParams;
		delete pFactory;
		delete pModule;
		return false;
	}

	if (!pFactory->init(pFactoryParams))
	{
		writeLog(LOG_ERROR, "Couldn't initialize factory instance: ", pFactory->getErrorString().c_str());
		delete pParams;
		delete pFactoryParams;
		delete pFactory;
		delete pModule;
		return false;
	}
	
	m_pModule = pModule;
	m_pFactory = pFactory;
	m_pFactoryParams = pFactoryParams;
	m_pGAParams = pParams;
	m_factoryFileName = moduleName;
	m_populationSize = popSize;
	
	m_pFactoryData = pData;
	m_factoryDataLength = dataLength;

	// write the correct factory number into the packet
	
	serut::MemorySerializer mser(0, 0, pData, dataLength);
	mser.writeInt32(GASERVER_COMMAND_FACTORY);
	mser.writeInt32(m_factoryNumber);
	
	return true;
}

bool ServerAlgorithm::calculateFitness(std::vector<GenomeWrapper> &population)
{
	std::list<ConnectionInfo *>::iterator it;
	int populationSize = population.size();
	int generation = getCurrentGeneration();
	int waitTimeSec = 0;

	m_genomesCalculated = 0;
	m_genomesToProcess.clear();
	m_pCurrentGenomes = &population;
	m_currentGenerationNumber = generation;

	for (int i = 0 ; i < populationSize ; i++)
		m_genomesToProcess.push_back(i);

	writeLog(LOG_DEBUG, "Current connections associated number of genomes:");
	
	int counter = 0;
	for (it = m_connections.begin() ; it != m_connections.end() ; it++)
	{
		ConnectionInfo *pConnInf = *it;
	
		if (pConnInf != m_pClientConnection)
		{
			pConnInf->resetNumberOfGenomesWritten();
			pConnInf->resetLastReceiveFitnessTime();

			writeLog(LOG_DEBUG, "  %s:%d \t %d \t %s",
			         pConnInf->getSocket()->getBaseSocket()->getDestinationAddress()->getAddressString().c_str(),
				 (int)pConnInf->getSocket()->getBaseSocket()->getDestinationPort(),
				 (int)pConnInf->getNumberOfGenomesToWrite(),
				 ((pConnInf->canHelp())?"active":"inactive"));
		}
		else
		{
			writeLog(LOG_DEBUG, "  %s:%d \t %d \t client",
			         pConnInf->getSocket()->getBaseSocket()->getDestinationAddress()->getAddressString().c_str(),
				 (int)pConnInf->getSocket()->getBaseSocket()->getDestinationPort(),
				 0);
		}
	}

	double startTime = getCurrentTime();

	while (m_genomesCalculated != populationSize && m_pClientConnection && !endServerLoop)
	{
		nut::SocketWaiter waiter;

		waiter.addSocket(m_listenSocket);

		for (it = m_connections.begin() ; it != m_connections.end() ; it++)
		{
			ConnectionInfo *pInf = *it;
			
			waiter.addSocket(*(pInf->getSocket()));
		}
		
		if (!waiter.wait(waitTimeSec, 0))
		{
			writeLog(LOG_ERROR, "Error waiting for data in calculate fitness procedure: %s", waiter.getErrorString().c_str());
			endServerLoop = true;
			m_pCurrentGenomes = 0;
			return false;
		}

		// Check for new connections
		
		if (m_listenSocket.isDataAvailable())
		{
			nut::TCPSocket *pTCPSocket = 0;

			if (m_listenSocket.accept(&pTCPSocket))
			{
				nut::TCPPacketSocket *pPacketSocket = new nut::TCPPacketSocket(pTCPSocket, true, false, GASERVER_MAXPACKSIZE, sizeof(uint32_t), GASERVER_PACKID);
				ConnectionInfo *pInf = new ConnectionInfo(pPacketSocket);
				
				m_connections.push_back(pInf);
			}
		}

		// Timeout old connections
		
		time_t curTime = time(0);
		
		it = m_connections.begin();
		while (it != m_connections.end())
		{
			ConnectionInfo *pInf = *it;

			if ((curTime - pInf->getSocket()->getLastReadTime()) > GASERVER_READTIMEOUT)
			{
				writeLog(LOG_DEBUG, "Timeout on connection");

				it = m_connections.erase(it);
				if (pInf == m_pClientConnection)
					m_pClientConnection = 0;
				delete pInf;
			}
			else
				it++;
		}

		// Process incoming packets
	
		if (m_pClientConnection)
		{
			it = m_connections.begin();
			while (it != m_connections.end())
			{
				ConnectionInfo *pInf = *it;
				
				if (pInf->getSocket()->isDataAvailable())
				{
					bool closed = false;
					size_t length = 0;
					
					pInf->getSocket()->getAvailableDataLength(length);
					if (length == 0)
						closed = true;
					else
					{
						if (!pInf->getSocket()->poll())
							closed = true;
					}
					
					if (closed)
					{
						writeLog(LOG_DEBUG, "Detected closed connection");

						if (pInf == m_pClientConnection)
							m_pClientConnection = 0;
						
						it = m_connections.erase(it);
						delete pInf;
					}
					else // connection not closed, process received packet (if available)
					{
						nut::Packet packet;
						bool gotError = false;
						
						while (!gotError && pInf->getSocket()->read(packet))
						{
							bool deleteData;
							size_t length = 0;
							uint8_t *pData = packet.extractData(length);
	
							if (!pInf->processPacketWhenRunning(pData, length, &deleteData))
								gotError = true;
							if (deleteData)
								delete [] pData;
						}
	
						if (gotError)
						{
							writeLog(LOG_DEBUG, "Detected inconsistency while processing packets (when running), removing connection");

							// some inconsistency was detected, remove connection
			
							if (pInf == m_pClientConnection)
								m_pClientConnection = 0;
							
							it = m_connections.erase(it);
							delete pInf;
						}
						else
							it++;
					}
				}
				else
					it++;
			}
		}

		if (m_pClientConnection)
		{
			// send outgoing packets
			
			int cantHelpCount = 0;
			int maxCantHelpCount = 0;
			
			it = m_connections.begin();
			while (it != m_connections.end())
			{
				ConnectionInfo *pInf = *it;

				if (pInf != m_pClientConnection)
				{
					int prevCantHelpCount = cantHelpCount;

					if (!pInf->sendData(&cantHelpCount))
					{
						writeLog(LOG_DEBUG, "Removing helper connection due to error in sending data");
						
						it = m_connections.erase(it);
						delete pInf;
					}
					else
					{
						it++;
						maxCantHelpCount++;
					}
				}
				else
				{
					if (!pInf->giveFeedback())
					{
						writeLog(LOG_DEBUG, "Removing client connection due to error in sending feedback");
						
						it = m_connections.erase(it);

						delete pInf;
						m_pClientConnection = 0;
					}
					else
						it++;
				}
			}
	
			// check if there are valid helpers
		
			if (m_pClientConnection != 0 && cantHelpCount == maxCantHelpCount)
			{
				m_pClientConnection->sendUnavailable();
				m_pCurrentGenomes = 0;
				return false;
			}

			waitTimeSec = 15;
			if (!m_genomesToProcess.empty())
			{
				// if not all genomes have been distributed yet, we're either at the very
				// first step of the algorithm (in which m_numberOfGenomesToWrite == 1), or
				// some gahelper process can't help us. We'll have to check if we need to
				// distribute the excess of genomes.
				
				distributeExcessGenomes();
				
				for (it = m_connections.begin() ; waitTimeSec != 0 && it != m_connections.end() ; it++)
				{
					ConnectionInfo *pInf = *it;

					if (pInf != m_pClientConnection)
					{
						if (pInf->getConnectionState() == ConnectionInfo::Idle && pInf->canHelp())
						{
							// There's at least one connection on which data can still
							// be sent
							waitTimeSec = 0;
						}
					}
				}
			}
		}
	}
	
	m_pCurrentGenomes = 0;
	if (!m_pClientConnection || endServerLoop)
		return false;

	double endTime = getCurrentTime();

	redistributeGenomes(startTime, endTime);

	return true;
}

// TODO: What happens when the system clock is adjusted?
void ServerAlgorithm::redistributeGenomes(double startTime, double endTime)
{
	std::list<ConnectionInfo *>::iterator it;
	double timeDiff = endTime - startTime;

	// Redistribute the current genomes

	std::vector<double> times;
	std::vector<double> timePerGenome;
	std::vector<ConnectionInfo *> connections;
	std::vector<int> genomes;

	for (it = m_connections.begin() ; it != m_connections.end() ; it++)
	{
		ConnectionInfo *pConnInf = *it;

		if (pConnInf != m_pClientConnection && pConnInf->canHelp() && pConnInf->getNumberOfGenomesToWrite() > 0)
		{
			double t = pConnInf->getLastReceiveFitnessTime();

			if (t >= startTime)
			{
				double interval = t - startTime;
				times.push_back(interval);
				timePerGenome.push_back(interval/(double)pConnInf->getNumberOfGenomesWritten());
				connections.push_back(pConnInf);
				genomes.push_back(pConnInf->getNumberOfGenomesToWrite());
			}
		}
	}

	writeLog(LOG_DEBUG, "Redistributing genomes");
	
	bool done = false;

	while (!done)
	{
		int lowestIndex = 0;
		int highestIndex = 0;
		double lowestNewTime = times[0] + timePerGenome[0];
		double highestTime = times[0];

		for (int i = 1 ; i < times.size() ; i++)
		{
			double t1 = times[i];
			double t2 = t1 + timePerGenome[i];
		
			if (t1 > highestTime && genomes[i] > 1 && (t1 - timePerGenome[i]) > 0) // don't remove the last genome, always send one
			{
				highestTime = t1;
				highestIndex = i;
			}

			if (t2 < lowestNewTime)
			{
				lowestNewTime = t2;
				lowestIndex = i;
			}
		}

		if (highestIndex == lowestIndex)
			done = true;
		else if (lowestNewTime >= highestTime - timePerGenome[highestIndex]) // won't create an improvement
			done = true;
		else
		{
			// will probably cause an improvement, swap some genomes

			times[lowestIndex] += timePerGenome[lowestIndex];
			times[highestIndex] -= timePerGenome[highestIndex];
			genomes[lowestIndex]++;
			genomes[highestIndex]--;

			writeLog(LOG_DEBUG, "  Moving one genome from %d to %d; new times %g and %g",
			         highestIndex, lowestIndex,
				 times[highestIndex], times[lowestIndex]);
		}
	}

	for (int i = 0 ; i < genomes.size() ; i++)
		connections[i]->setNumberOfGenomesToWrite(genomes[i]);

	// Make sure that the amount of genomes add up to population size

	int genomeCount = 0;

	for (it = m_connections.begin() ; it != m_connections.end() ; it++)
	{
		ConnectionInfo *pConnInf = *it;

		if (pConnInf != m_pClientConnection && pConnInf->canHelp())
			genomeCount += pConnInf->getNumberOfGenomesToWrite();
	}

	if (genomeCount <= m_populationSize) // This case is covered in the distributeExcessGenomes function
		return;

	// Adjust the amounts to make sure that everything adds up to the population size
	
	genomeCount -= m_populationSize;
	done = false;

	while (!done && genomeCount > 0)
	{
		done = true;
		for (it = m_connections.begin() ; genomeCount > 0 && it != m_connections.end() ; it++)
		{
			ConnectionInfo *pConnInf = *it;

			if (pConnInf != m_pClientConnection && pConnInf->canHelp() && pConnInf->getNumberOfGenomesToWrite() > 1)
			{
				done = false;
				pConnInf->addNumberOfGenomesToWrite(-1);
				pConnInf->processAddedNumberOfGenomes();
				genomeCount--;
			}
		}
	}
}

void ServerAlgorithm::distributeExcessGenomes()
{
	std::list<ConnectionInfo *>::iterator it;
	int genomeCount = 0;

	// First we count how many genomes are being distributed at this moment

	for (it = m_connections.begin() ; it != m_connections.end() ; it++)
	{
		ConnectionInfo *pConnInf = *it;

		if (pConnInf != m_pClientConnection && pConnInf->canHelp())
			genomeCount += pConnInf->getNumberOfGenomesToWrite();
	}

	if (genomeCount >= m_populationSize) // No excess genomes to redistribute
		return;

	int genomesToDistribute = m_populationSize-genomeCount;

	// We'll just distribute the excess evenly over the existing connections,
	// while trying to avoid that all genomes end up at a single helper at the
	// beginning of the algorithm.
	
	bool done = false;

	while (!done && genomesToDistribute > 0)
	{
		done = true;
		for (it = m_connections.begin() ; genomesToDistribute > 0 && it != m_connections.end() ; it++)
		{
			ConnectionInfo *pConnInf = *it;

			if (pConnInf != m_pClientConnection && pConnInf->canHelp() && pConnInf->getAddedNumberOfGenomes() < pConnInf->getNumberOfGenomesToWrite())
			{
				done = false;
				pConnInf->addNumberOfGenomesToWrite(1);
				genomesToDistribute--;
			}
		}
	}

	for (it = m_connections.begin() ; it != m_connections.end() ; it++)
	{
		ConnectionInfo *pConnInf = *it;

		pConnInf->processAddedNumberOfGenomes();
	}
}

bool ServerAlgorithm::onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged)
{
	if (generationInfoChanged)
		m_generationInfoCount++;
	
	return true;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		std::cerr << "Usage: " << std::endl;
		std::cerr << std::string(argv[0]) << " debuglevel portnumber moduledir" << std::endl; 
		return -1;
	}

	setDebugLevel(atoi(argv[1]));
	
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
		return -1;
	}

	if (!listenSocket.listen(32))
	{
		writeLog(LOG_ERROR, "Couldn't place socket in listen mode: %s", listenSocket.getErrorString().c_str());
		return -1;
	}
	
	writeLog(LOG_INFO, "Listen socket created at port %d", (int)listenPort);

	// We won't put this inside the next loop to avoid helper connections being closed.
	ServerAlgorithm servAlg(listenSocket, std::string(argv[3])); 

	while (!endServerLoop)
	{
		writeLog(LOG_DEBUG, "Looping");
		servAlg.run();
	}

	writeLog(LOG_INFO, "Exiting");
	return 0;
}

