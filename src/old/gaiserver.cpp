#include "graleconfig.h"
#include "gaserver.h"
#include "geneticalgorithm.h"
#include "gafactory.h"
#include "tcpconnectionaccepter.h"
#include "tcppacketconnection.h"
#include "datapacket.h"
#include "memoryserializer.h"
#include "dummyserializer.h"
#include "uniformdistribution.h"
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <list>

#include "debugnew.h"

#define TIMEOUTVAL 600

using namespace grale;
bool endserverloop = false;

void SignalHandler(int val)
{
	endserverloop = true;
}

inline double GetCurrentTime()
{
	struct timeval tv;

	gettimeofday(&tv, 0);

	return (((double)tv.tv_sec)+((double)tv.tv_usec/1000000.0));
}

class ConnectionInfo;

class ServerAlgorithm
{
public:
	static ServerAlgorithm *ServerAlgorithm::Instance()						{ return instance; }

	ServerAlgorithm(TCPConnectionAccepter &ca, const std::string &baseDir) : connaccepter(ca),dist(0,1)
	{ 
		if (instance != 0)
		{
			std::cerr << "Only one instance at a time please!" << std::endl;
			exit(-1);
		}
		instance = this;
		baseDirectory = baseDir;
		factoryNumber = 0;
		clientconn = 0;
	}
	~ServerAlgorithm()										{ }
	void Run();
	bool MigrationLoop();

	ConnectionInfo *GetClientConnection() const							{ return clientconn; }
	void SetClientConnection(ConnectionInfo *c)							{ clientconn = c; }
	ConnectionInfo *ChooseNeighbouringIsland(const ConnectionInfo *c) const;
	ConnectionInfo *ChooseRandomIsland(const ConnectionInfo *c) const;

	int GetNumberOfConnections() const								{ return connections.size(); }
	bool LoadFactory(SerialInterface &si, uint8_t *data, int datalength);
	int GetFactoryNumber() const									{ return factoryNumber; }

	const GAFactory *GetFactory() const								{ return factory; }
	const uint8_t *GetFactoryData() const								{ return factoryData; }
	int GetFactoryDataLength() const								{ return factoryDataLength; }

	int GetPopulationSize() const									{ return populationsize; }

	void TryBestGenome(Genome *g);
	Genome *GetBestGenome() const									{ return bestgenome; }
private:
	TCPConnectionAccepter &connaccepter;
	std::list<ConnectionInfo *> connections;
	ConnectionInfo *clientconn;

	void *moduleHandle;
	GAFactory *factory;	
	GAFactoryParams *factoryParams;
	GeneticAlgorithmParams *gaParams;
	std::string factoryFilename;
	int populationsize;
	Genome *bestgenome;
	
	const uint8_t *factoryData;
	int factoryDataLength;
	int factoryNumber;

	std::string baseDirectory;
	mutable UniformDistribution dist;

	static ServerAlgorithm *instance;
};

ServerAlgorithm *ServerAlgorithm::instance = 0;

class ConnectionInfo
{
public:
	enum ConnectionState { Unidentified, Idle, Calculating };
	
	ConnectionInfo(TCPPacketConnection *conn, uint32_t IP, uint16_t port)
	{
		connection = conn;
		ConnectionInfo::IP = IP;
		ConnectionInfo::port = port;
		state = Unidentified;
		generationTime = 5;
		
		struct in_addr addr;
	  	addr.s_addr = ntohl(IP);

		std::cout << "Accepted incoming connection from " << inet_ntoa(addr) << ":" << port << std::endl;

		ResetFactoryInfo();
	}

	~ConnectionInfo()
	{
		delete connection;
	
		struct in_addr addr;
		addr.s_addr = ntohl(IP); 
					
		std::cout << "Removing connection from " << inet_ntoa(addr) << ":" << port << std::endl;
	}
	
	void ResetFactoryInfo()
	{
		lastWrittenFactoryNumber = -1;
		ackFactoryNumber = -1;
		canHelp = false;
		generationTime = 5;
	}

	TCPPacketConnection *GetConnection()								{ return connection; }
	ConnectionState GetState() const								{ return state; }
	
	bool ProcessPacket(uint8_t *data, int datalength, bool *deletedata);
	bool ProcessPacketWhenRunning(uint8_t *data, int datalength, bool *deletedata);
	bool SendData(int *canhelpcount);
	bool GiveFeedback();
	void SendUnavailable();

	void MigrateGenome(DataPacket *p);
	void AddToMigrationQueue(DataPacket *p)								{ migrationqueue.push_back(p); }
	void ClearMigrationQueue()									{ while (!migrationqueue.empty()) { delete *(migrationqueue.begin()); migrationqueue.pop_front(); } }

	time_t GetGenerationTime() const								{ return generationTime; }
	int GetAckFactoryNumber() const									{ return ackFactoryNumber; }
	bool CanHelp() const										{ return canHelp; }

	void WaitForClose();
private:
	TCPPacketConnection *connection;
	uint32_t IP;
	uint16_t port;
	ConnectionState state;

	int lastWrittenFactoryNumber;
	int ackFactoryNumber;
	bool canHelp;
	time_t generationTime;

	std::list<DataPacket *> migrationqueue;
};

void ConnectionInfo::SendUnavailable()
{
	uint8_t buf[sizeof(int32_t)];
	MemorySerializer mser(0,0,buf,sizeof(int32_t));

	mser.WriteInt32(GASERVER_COMMAND_NOHELPERS); // tell client we're busy
	connection->WritePacket(buf,sizeof(int32_t));
}

bool ConnectionInfo::ProcessPacket(uint8_t *data, int datalength, bool *deletedata)
{
	*deletedata = true;

	if (datalength < sizeof(int32_t))
		return false;

	MemorySerializer mser(data,datalength,0,0);
	int32_t cmd;
	ServerAlgorithm *s = ServerAlgorithm::Instance();
	
	mser.ReadInt32(&cmd);

	if (state == Unidentified)
	{
		if (cmd == GASERVER_COMMAND_CLIENT)
		{
			if (s->GetClientConnection() != 0) // there aleady is a client
			{
				uint8_t buf[sizeof(int32_t)];
				MemorySerializer mser(0,0,buf,sizeof(int32_t));

				mser.WriteInt32(GASERVER_COMMAND_BUSY); // tell client we're busy
				connection->WritePacket(buf,sizeof(int32_t));
				
				return false;
			}
			else // ok, there isn't any client yet
			{
				if (s->GetNumberOfConnections() == 1) // this is the only connection
				{
					SendUnavailable();
					return false;
				}
				else
				{
					uint8_t buf[sizeof(int32_t)];
					MemorySerializer mser(0,0,buf,sizeof(int32_t));

					mser.WriteInt32(GASERVER_COMMAND_ACCEPT);
					if (!connection->WritePacket(buf,sizeof(int32_t)))
						return false;

					s->SetClientConnection(this);
					state = Idle;

					return true;
				}
			}
		}
		else if (cmd == GASERVER_COMMAND_ISLANDHELPER)
		{
			state = Idle;
			return true;
		}
		else // received invalid packet
			return false;
	}
	else
	{
		if (cmd == GASERVER_COMMAND_KEEPALIVE)
		{
			if (s->GetClientConnection() == this)
			{
				if (s->GetFactory() == 0) // no factory information has been received yet
					return false;
			}
			if (datalength != sizeof(int32_t))
				return false;
			return true;
		}
		
		if (s->GetClientConnection() == this) // we're the client
		{
			if (cmd == GASERVER_COMMAND_FACTORY)
			{
				if (s->GetFactory() != 0)
					return false;
				if (!s->LoadFactory(mser,data,datalength))
					return false;
				*deletedata = false;
				return true;
			}

			// clients shouldn't write anything except keepalive packets
			return false;
		}
		else // we're a helper
		{
			if (cmd == GASERVER_COMMAND_MIGRATEDGENOME || cmd == GASERVER_COMMAND_BESTLOCALGENOME) 
								    // it's possible that we're receiving a genome from a previous request
			{                                    	    // (if the client disconnected prematurely
				return true;
			}
			if (cmd == GASERVER_COMMAND_FACTORYRESULT) // same as above
			{
				return true;
			}
			if (cmd == GASERVER_COMMAND_ISLANDFINISHED) // same as above
			{
				// mark helper as idle
				state = Idle;
				ResetFactoryInfo();
				ClearMigrationQueue();
				return true;
			}			
			if (cmd == GASERVER_COMMAND_GENERATIONTIME)
			{
				return true;
			}
			return false;
		}
	}

	return false;
}

bool ConnectionInfo::ProcessPacketWhenRunning(uint8_t *data, int datalength, bool *deletedata)
{
	*deletedata = true;

	if (datalength < sizeof(int32_t))
		return false;

	MemorySerializer mser(data,datalength,0,0);
	int32_t cmd;
	ServerAlgorithm *s = ServerAlgorithm::Instance();
	
	mser.ReadInt32(&cmd);

	if (state == Unidentified)
	{
		if (cmd == GASERVER_COMMAND_CLIENT)
		{
			// there aleady is a client !
			uint8_t buf[sizeof(int32_t)];
			MemorySerializer mser(0,0,buf,sizeof(int32_t));

			mser.WriteInt32(GASERVER_COMMAND_BUSY); // tell client we're busy
			connection->WritePacket(buf,sizeof(int32_t));
			
			return false;
		}
		else if (cmd == GASERVER_COMMAND_ISLANDHELPER)
		{
			state = Idle;
		}
		else // received invalid packet
			return false;
	}
	else
	{
		if (cmd == GASERVER_COMMAND_KEEPALIVE)
		{
			if (datalength != sizeof(int32_t))
				return false;
			return true;
		}
		
		if (s->GetClientConnection() == this) // we're the client
		{
			// clients shouldn't write anything except keepalive packets
			return false;
		}
		else // we're a helper: the only things we should receive are migrating genomes and new local best genomes
		     // factory loading result is also possible, as is a signal that a local genetic algorithm has
		     // finished
		{
			if (cmd == GASERVER_COMMAND_MIGRATEDGENOME) 
			{
				if (state != Calculating) // invalid packet
					return false;
			
				if (ackFactoryNumber != ServerAlgorithm::Instance()->GetFactoryNumber()) // is from another factory
					return true; // ignore it
				
				DataPacket *p = new DataPacket(data, datalength);
				*deletedata = false;

				MigrateGenome(p);
				return true;
			}
			if (cmd == GASERVER_COMMAND_BESTLOCALGENOME)
			{
				if (state != Calculating) // invalid packet
					return false;

				// first we check if the node has the current factory, since it may
				// still be finishing calculations on a previous one

				if (ackFactoryNumber != ServerAlgorithm::Instance()->GetFactoryNumber())
					return true; // just ignore it
				
				const GAFactory *factory = ServerAlgorithm::Instance()->GetFactory();
				Genome *genome = 0;
				
				if (!factory->ReadGenome(mser, &genome))
				{
					// perhaps a delayed result from another run, ignore it
				}
				else
				{
					if (!factory->ReadGenomeFitness(mser, genome))
					{
						// perhaps a delayed result from another run, ignore it
					}
					else
					{
						// ok, got a valid genome and fitness, compare it

						ServerAlgorithm::Instance()->TryBestGenome(genome);
					}
				}

				return true;
			}
			if (cmd == GASERVER_COMMAND_FACTORYRESULT)
			{
				if (state == Calculating) // invalid packet
					return false;
			
				if (lastWrittenFactoryNumber == ackFactoryNumber) // don't expect a factory ack at this stage
					return false;
				
				if (datalength != sizeof(int32_t)*3)
					return false;
				
				int32_t facnum,available;

				if (!mser.ReadInt32(&facnum))
					return false;
				if (!mser.ReadInt32(&available))
					return false;
				if (available != 0 && available != 1)
					return false;
				
				if (facnum == lastWrittenFactoryNumber)
				{
					ackFactoryNumber = lastWrittenFactoryNumber;
					if (available)
					{
						// When the client has sent a positive ack, it has started the
						// calculation
						canHelp = true;
						state = Calculating;
						ClearMigrationQueue(); // before we begin, clear the queue, just to be safe
					}
					else
						canHelp = false;
				}

				return true;
			}
			if (cmd == GASERVER_COMMAND_ISLANDFINISHED) // same as above
			{
				// mark helper as idle
				state = Idle;
				ResetFactoryInfo(); // since the helper will delete the factory, we'll send it again
				ClearMigrationQueue();
				return true;
			}				
			if (cmd == GASERVER_COMMAND_GENERATIONTIME)
			{
				int32_t t;

				if (!mser.ReadInt32(&t))
					return false;
				if (t < 5)
					t = 5;

				generationTime = t;
				return true;
			}
			return false;
		}
	}

	return true;
}

bool ConnectionInfo::SendData(int *canhelpcount)
{
	if (state == Unidentified) // nothing to be done yet
		return true;

	if (state == Calculating) // node is running genetic algoritm, check if migrated genomes should be sent
	{
		while (!migrationqueue.empty())
		{
			DataPacket *p = *(migrationqueue.begin());
			
			if (!connection->WritePacket(p->GetData(), p->GetDataLength()))
				return false;
			migrationqueue.pop_front();
			delete p;
		}
		return true;
	}
	
	ServerAlgorithm *s = ServerAlgorithm::Instance();
	
	// check if we need to send new factory data
	
	if (lastWrittenFactoryNumber != s->GetFactoryNumber())
	{
		if (!connection->WritePacket(s->GetFactoryData(),s->GetFactoryDataLength()))
			return false;
		lastWrittenFactoryNumber = s->GetFactoryNumber();
		return true;
	}
	if (lastWrittenFactoryNumber != ackFactoryNumber) // wait for factory ack
		return true;

	if (!canHelp) // this helper can't help us
		(*canhelpcount)++;

	return true;
}

bool ConnectionInfo::GiveFeedback()
{
	time_t lasttime = connection->GetLastWriteTime();
	time_t curtime = time(0);
	
	if (curtime-lasttime > 20) // write something every 20 seconds
	{
		ServerAlgorithm *s = ServerAlgorithm::Instance();
		if (s->GetBestGenome() == 0)
		{
			uint8_t buf[sizeof(int32_t)];
			MemorySerializer mser(0,0,buf,sizeof(int32_t));

			mser.WriteInt32(GASERVER_COMMAND_KEEPALIVE);
			if (!connection->WritePacket(buf,sizeof(int32_t)))
				return false;
		}
		else
		{
			// Write result

			int len = sizeof(int32_t)+s->GetFactory()->GetMaximalGenomeSize()+s->GetFactory()->GetMaximalFitnessSize();
			uint8_t *buf = new uint8_t[len];
			MemorySerializer mser(0,0,buf,len);
			
			mser.WriteInt32(GASERVER_COMMAND_CURRENTBEST);
			s->GetFactory()->WriteGenome(mser,s->GetBestGenome());
			s->GetFactory()->WriteGenomeFitness(mser,s->GetBestGenome());

			connection->WritePacket(buf,mser.GetBytesWritten());

			delete [] buf;
		}
	}
	return true;
}

void ConnectionInfo::MigrateGenome(DataPacket *p)
{
	ConnectionInfo *inf = ServerAlgorithm::Instance()->ChooseRandomIsland(this);
	if (inf == 0)
	{
		std::cout << "Couldn't find island to migrate to " << port << std::endl;
		return;
	}

	inf->AddToMigrationQueue(p);
}

void ConnectionInfo::WaitForClose()
{
	time_t starttime = time(0);
	
	while ((time(0) - starttime) < 60) // wait at most one minute
	{
		struct timeval tv;
		fd_set fdset;

		FD_ZERO(&fdset);
		FD_SET(connection->GetSocket(),&fdset);
		tv.tv_sec = 0;
		tv.tv_usec = 200000;

		if (select(FD_SETSIZE,&fdset,0,0,&tv) == -1)
			return;

		if (FD_ISSET(connection->GetSocket(),&fdset))
		{
			if (connection->GetAvailableDataLength() <= 0)
			{
				std::cout << "Client closed connection" << std::endl;
				return;
			}
			if (!connection->Poll())
				return;

			DataPacket *p = 0;
			while ((p = connection->GetNextPacket()) != 0)
				delete p;
		}
	}
}

template<class listClass, class listClass_const_iterator> 
listClass_const_iterator GotoPrev(listClass connections, listClass_const_iterator it)
{
	if (it == connections.begin())
	{
		it = connections.end();
		it--;
		return it;
	}

	it--;
	return it;
}

template<class listClass, class listClass_const_iterator> 
listClass_const_iterator GotoNext(listClass connections, listClass_const_iterator it)
{
	it++;
	if (it == connections.end())
		it = connections.begin();

	return it;
}

ConnectionInfo *ServerAlgorithm::ChooseNeighbouringIsland(const ConnectionInfo *c) const
{
	std::list<ConnectionInfo *>::const_iterator it = connections.begin();

	while (it != connections.end() && (*it) != c)
		it++;

	if (it == connections.end())
		return 0;

	double x = dist.pickNumber();
	ConnectionInfo *inf = 0;
	
	if (x < 0.5)
	{
		it = GotoPrev(connections, it);
		if ((*it) == clientconn)
			it = GotoPrev(connections, it);
	}
	else
	{
		it = GotoNext(connections, it);
		if ((*it) == clientconn)
			it = GotoNext(connections, it);
	}
	if ((*it) != c)
		inf = (*it);
	return inf;
}

ConnectionInfo *ServerAlgorithm::ChooseRandomIsland(const ConnectionInfo *c) const
{
	int num = connections.size()-2; // remove the clientconnection and this connection	
	if (num <= 0)
		return 0;

	int idx = (int)(dist.pickNumber()*(double)num);

	std::list<ConnectionInfo *>::const_iterator it = connections.begin();

	int count = 0;
	bool done = false;
	do
	{
		while ((*it) == c || (*it) == clientconn)
			it = GotoNext(connections,it);
		
		if (count == idx)
			done = true;
		else
			it = GotoNext(connections,it);
		count++;
	} while (!done);
	
	if ((*it) == c || (*it) == clientconn)
		return 0;
	return (*it);
}

void ServerAlgorithm::Run()
{
	factory = 0;
	gaParams = 0;
	factoryParams = 0;
	moduleHandle = 0;
	factoryDataLength = 0;
	factoryData = 0;
	bestgenome = 0;
	factoryNumber++;
	
	while (!factory && !endserverloop)
	{
		fd_set fdset;
		struct timeval tv;
		std::list<ConnectionInfo *>::iterator it;

		FD_ZERO(&fdset);
		FD_SET(connaccepter.GetSocket(),&fdset);
		for (it = connections.begin() ; it != connections.end() ; it++)
			FD_SET((*it)->GetConnection()->GetSocket(),&fdset);

		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (select(FD_SETSIZE,&fdset,0,0,&tv) == -1)
		{
			std::cout << "ServerAlgorithm::Run: Error in select" << std::endl;
			endserverloop = true;
			return;
		}

		// Check for new connections
		
		if (FD_ISSET(connaccepter.GetSocket(),&fdset))
		{
			TCPConnection *conn = connaccepter.AcceptConnection();
			if (conn)
			{
				TCPPacketConnection *packconn = new TCPPacketConnection(*conn);
				delete conn;

				ConnectionInfo *inf = new ConnectionInfo(packconn,connaccepter.GetLastAcceptedIP(),connaccepter.GetLastAcceptedPort());
				connections.push_back(inf);
			}
		}

		// Timeout old connections
		
		time_t curtime = time(0);
		
		it = connections.begin();
		while (it != connections.end())
		{
			ConnectionInfo *inf = (*it);

			if ((curtime - inf->GetConnection()->GetLastReadTime()) > TIMEOUTVAL)
			{
				it = connections.erase(it);
				if (inf == clientconn)
				{
					std::cout << "Timing out client connection" << std::endl;
					clientconn = 0;
				}
				delete inf;
			}
			else
				it++;
		}

		// Process incoming packets
	
		it = connections.begin();
		while (it != connections.end())
		{
			if (FD_ISSET((*it)->GetConnection()->GetSocket(),&fdset))
			{
				bool closed = false;
				
				if ((*it)->GetConnection()->GetAvailableDataLength() <= 0)
					closed = true;
				else
				{
					if (!(*it)->GetConnection()->Poll())
						closed = true;
				}
				
				ConnectionInfo *inf = (*it);

				if (closed)
				{
					if (inf == clientconn)
					{
						std::cout << "Client closed connection" << std::endl;
						clientconn = 0;
					}
					it = connections.erase(it);
					delete inf;
				}
				else // connection not closed, process received packet (if available)
				{
					DataPacket *p;
					bool goterror = false;
					
					while (!goterror && (p = inf->GetConnection()->GetNextPacket()) != 0)
					{
						bool deletedata;

						if (!inf->ProcessPacket(p->GetData(), p->GetDataLength(),&deletedata))
							goterror = true;
						if (!deletedata)
							p->ExtractData();
						delete p;
					}

					if (goterror)
					{
						// some inconsistency was detected, remove connection
		
						if (inf == clientconn)
						{
							std::cout << "Error processing client packet, closing connection" << std::endl;
							clientconn = 0;
						}
						it = connections.erase(it);
						delete inf;
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
	
	if (factory)
	{
		std::list<ConnectionInfo *>::iterator it;

		for (it = connections.begin() ; it != connections.end() ; it++)
			(*it)->ClearMigrationQueue();

		retval = MigrationLoop();
		if (retval && clientconn) // loop finished successfully
		{
			std::cout << "Genetic algorithm finished successfully" << std::endl;
			// Write result

			int len = sizeof(int32_t)+factory->GetMaximalGenomeSize()+factory->GetMaximalFitnessSize();
			uint8_t *buf = new uint8_t[len];
			MemorySerializer mser(0,0,buf,len);
			
			mser.WriteInt32(GASERVER_COMMAND_RESULT);
			factory->WriteGenome(mser,GetBestGenome());
			factory->WriteGenomeFitness(mser,GetBestGenome());

			clientconn->GetConnection()->WritePacket(buf,mser.GetBytesWritten());

			delete [] buf;

			// end of communication, wait a while for the client to close the connection
			clientconn->WaitForClose();
		}
	}
				
	if (clientconn)
	{
		std::cout << "Ending Run, destroying client connection" << std::endl;
		
		std::list<ConnectionInfo *>::iterator it;
		bool found = false;
		
		it = connections.begin();
		while (it != connections.end() && !found)
		{
			if ((*it) == clientconn)
			{
				connections.erase(it);
				found = true;
			}
			else
				it++;
		}

		delete clientconn;
		clientconn = 0;
	}
	
	// Signal end to islands
	
	uint8_t buf[sizeof(int32_t)];
	MemorySerializer mser(0,0,buf,sizeof(int32_t));
	std::list<ConnectionInfo *>::const_iterator it;

	mser.WriteInt32(GASERVER_COMMAND_ISLANDSTOP);
	for (it = connections.begin() ; it != connections.end() ; it++)
	{
		if ((*it)->GetState() == ConnectionInfo::Calculating)
			(*it)->GetConnection()->WritePacket(buf,sizeof(int32_t));
	}
	
	if (factory)
		delete factory;
	if (factoryParams)
		delete factoryParams;
	if (gaParams)
		delete gaParams;
	
	// make sure we delete the best genome before we close its factory module
	if (bestgenome)
		delete bestgenome;
	if (moduleHandle)
		dlclose(moduleHandle);

	if (factoryData)
		delete [] factoryData;
}

typedef GAFactory *(*CreateFactoryInstanceFunction)();

bool ServerAlgorithm::LoadFactory(SerialInterface &si, uint8_t *data, int datalength)
{
	std::string moduleName;
	void *m;
	CreateFactoryInstanceFunction CreateFactoryInstance;
	int32_t facNum;

	if (!si.ReadInt32(&facNum))
		return false;

	if (facNum != 0)
	{
		std::cout << "Incoming factory number should be zero!" << std::endl;
		return false;
	}

	if (!si.ReadString(&moduleName))
		return false;

	std::string fullPath = baseDirectory + std::string("/") + moduleName;
	m = dlopen(fullPath.c_str(),RTLD_NOW);
	if (m == 0)
	{
		std::cout << "Couldn't load module " + fullPath << std::endl;
		return false;
	}

	CreateFactoryInstance = (CreateFactoryInstanceFunction)dlsym(m,"CreateFactoryInstance");
	if (CreateFactoryInstance == 0)
	{
		std::cout << "Invalid module" << std::endl;
		dlclose(m);
		return false;
	}
	
	GAFactory *fact = CreateFactoryInstance();
	
	if (fact == 0)
	{
		std::cout << "Couldn't create factory instance" << std::endl;
		dlclose(m);
		return false;
	}

	GAFactoryParams *factParams = fact->CreateParamsInstance();
	if (factParams == 0)
	{
		std::cout << "Coulnd't create factory parameters instance" << std::endl;
		delete fact;
		dlclose(m);
		return false;
	}

	GeneticAlgorithmParams *params = new GeneticAlgorithmParams();
	int32_t popsize;
	
	if (!si.ReadInt32(&popsize))
	{
		delete params;
		delete factParams;
		delete fact;
		dlclose(m);
		return false;
	}

	if (!factParams->Read(si))
	{
		delete params;
		delete factParams;
		delete fact;
		dlclose(m);
		return false;
	}

	if (!params->Read(si))
	{
		delete params;
		delete factParams;
		delete fact;
		dlclose(m);
		return false;
	}

	if (popsize <= 0)
	{
		delete params;
		delete factParams;
		delete fact;
		dlclose(m);
		return false;
	}

	if (!fact->Init(factParams))
	{
		std::cout << "Coulnd't initialize factory instance" << std::endl;
		delete params;
		delete factParams;
		delete fact;
		dlclose(m);
		return false;
	}
	
	moduleHandle = m;
	factory = fact;
	factoryParams = factParams;
	gaParams = params;
	factoryFilename = moduleName;
	populationsize = popsize;
	
	factoryData = data;
	factoryDataLength = datalength;

	// write the correct factory number into the packet
	
	MemorySerializer mser2(0,0,data,datalength);
	mser2.WriteInt32(GASERVER_COMMAND_FACTORY);
	mser2.WriteInt32(factoryNumber);
	
	return true;
}

bool ServerAlgorithm::MigrationLoop()
{
	bool finished = false;
	int virtualgeneration = 0;
	time_t prevtime = time(0);

	while (!finished && clientconn && !endserverloop)
	{
		fd_set fdset;
		struct timeval tv;
		std::list<ConnectionInfo *>::iterator it;

		FD_ZERO(&fdset);
		FD_SET(connaccepter.GetSocket(),&fdset);
		for (it = connections.begin() ; it != connections.end() ; it++)
			FD_SET((*it)->GetConnection()->GetSocket(),&fdset);

		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (select(FD_SETSIZE,&fdset,0,0,&tv) == -1)
		{
			std::cout << "ServerAlgorithm::MigrationLoop: Error in select" << std::endl;
			endserverloop = true;
			return false;
		}

		// Check for new connections
		
		if (FD_ISSET(connaccepter.GetSocket(),&fdset))
		{
			TCPConnection *conn = connaccepter.AcceptConnection();
			if (conn)
			{
				TCPPacketConnection *packconn = new TCPPacketConnection(*conn);
				delete conn;

				ConnectionInfo *inf = new ConnectionInfo(packconn,connaccepter.GetLastAcceptedIP(),connaccepter.GetLastAcceptedPort());
				connections.push_back(inf);
			}
		}

		// Timeout old connections
		
		time_t curtime = time(0);
		
		it = connections.begin();
		while (it != connections.end())
		{
			ConnectionInfo *inf = (*it);

			if ((curtime - inf->GetConnection()->GetLastReadTime()) > TIMEOUTVAL)
			{
				it = connections.erase(it);
				if (inf == clientconn)
				{
					clientconn = 0;
					std::cout << "Timeout on client connection, removing it" << std::endl;
				}
				delete inf;
			}
			else
				it++;
		}

		// Process incoming packets
	
		if (clientconn)
		{
			it = connections.begin();
			while (it != connections.end())
			{
				if (FD_ISSET((*it)->GetConnection()->GetSocket(),&fdset))
				{
					bool closed = false;
					
					if ((*it)->GetConnection()->GetAvailableDataLength() <= 0)
						closed = true;
					else
					{
						if (!(*it)->GetConnection()->Poll())
							closed = true;
					}
					
					ConnectionInfo *inf = (*it);
	
					if (closed)
					{
						if (inf == clientconn)
						{
							std::cout << "The client closed his connection, removing it" << std::endl;
							clientconn = 0;
						}
						it = connections.erase(it);
						delete inf;
					}
					else // connection not closed, process received packet (if available)
					{
						DataPacket *p;
						bool goterror = false;
						
						while (!goterror && (p = inf->GetConnection()->GetNextPacket()) != 0)
						{
							bool deletedata;
	
							if (!inf->ProcessPacketWhenRunning(p->GetData(), p->GetDataLength(),&deletedata))
								goterror = true;
							if (!deletedata)
								p->ExtractData();
							delete p;
						}
	
						if (goterror)
						{
							// some inconsistency was detected, remove connection
			
							if (inf == clientconn)
							{
								clientconn = 0;
								std::cout << "Invalid packet from client when running" << std::endl;
							}
							it = connections.erase(it);
							delete inf;
						}
						else
							it++;
					}
				}
				else
					it++;
			}
		}

		if (clientconn)
		{
			// send outgoing packets
			
			int canthelpcount = 0;
			int maxcanthelpcount = 0;
			
			it = connections.begin();
			while (it != connections.end())
			{
				ConnectionInfo *inf = (*it);

				if (inf != clientconn)
				{
					if (!inf->SendData(&canthelpcount))
					{
						it = connections.erase(it);
						delete inf;
					}
					else
					{
						it++;
						maxcanthelpcount++;
					}
				}
				else
				{
					if (!inf->GiveFeedback())
					{
						it = connections.erase(it);
						delete inf;
						clientconn = 0;
						std::cout << "Couldn't write feedback to client, removed connection" << std::endl;
					}
					else
						it++;
				}
			}
	
			// check if there are valid helpers
		
			if (clientconn != 0 && canthelpcount == maxcanthelpcount)
			{
				clientconn->SendUnavailable();
				return false;
			}
		}

		if (clientconn)
		{
			time_t gentime = 5;

			std::list<ConnectionInfo *>::const_iterator it;

			for (it = connections.begin() ; it != connections.end() ; it++)
			{
				if ((*it)->GetAckFactoryNumber() == factoryNumber && (*it)->CanHelp())
					gentime = MAX(gentime, (*it)->GetGenerationTime());
			}
			
			time_t curtime = time(0);
			bool dummy = false;;
			bool stoploop= false;
			if ((curtime - prevtime) > gentime && bestgenome != 0) // we'll simulate generations here
			{
				factory->OnGeneticAlgorithmStep(virtualgeneration, bestgenome, bestgenome, &dummy, &stoploop);
				virtualgeneration++;
				if (stoploop)
					finished = true;
				prevtime = curtime;
			}
		}
	}
	
	if (!clientconn || endserverloop)
		return false;
	return true;
}

void ServerAlgorithm::TryBestGenome(Genome *g)
{
	if (bestgenome == 0)
		bestgenome = g;
	else
	{
		if (g->IsFitterThan(bestgenome))
		{
			delete bestgenome;
			bestgenome = g;
		}
		else
			delete g;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << std::endl;
		std::cerr << std::string(argv[0]) << " portnumber moduledir" << std::endl; 
		return -1;
	}

	signal(SIGTERM,SignalHandler);
	signal(SIGHUP,SignalHandler);
	signal(SIGQUIT,SignalHandler);
	signal(SIGABRT,SignalHandler);
	signal(SIGINT,SignalHandler);

	TCPConnectionAccepter connaccepter;

	if (!connaccepter.Open(atoi(argv[1])))
	{
		std::cerr << "Error creating server at port " << atoi(argv[1]) << ":" << std::endl;
		std::cerr << connaccepter.getErrorString() << std::endl;
		return -1;
	}

	std::cout << "Listen socket created at port " << atoi(argv[1]) << std::endl;

	ServerAlgorithm servAlg(connaccepter,argv[2]); // we won't put this inside the next loop to avoid helper connections being closed.

	while (!endserverloop)
	{
		std::cout << "Looping in main" << std::endl;
		servAlg.Run();
	}

	std::cout << "Exiting" << std::endl;
	return 0;
}

