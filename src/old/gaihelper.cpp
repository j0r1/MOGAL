#include "gaserver.h"
#include "tcppacketconnection.h"
#include "memoryserializer.h"
#include "datapacket.h"
#include "gafactory.h"
#include "genome.h"
#include "geneticalgorithm.h"
#include "dummyserializer.h"
#include "uniformdistribution.h"
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <string>

#include "debugnew.h"

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

class IslandAlgorithm : public GeneticAlgorithm
{
public:
	IslandAlgorithm(TCPPacketConnection *c, int maxqueue = 1)			{ conn = c; maxqueuelen = maxqueue; nextmigrationgeneration = -1; lastbesttime = time(0); prevgentime = -1; }
	~IslandAlgorithm()								{ while (!migrationqueue.empty()) { delete *(migrationqueue.begin()); migrationqueue.pop_front(); } }
private:
	bool CalculateFitness(std::vector<GenomeWrapper> &population, int populationsize);

	std::list<DataPacket *> migrationqueue; 
	int nextmigrationgeneration;
	TCPPacketConnection *conn;
	int maxqueuelen;
	time_t lastbesttime;
	double prevgentime;
};

void *moduleHandle = 0;
GAFactory *factory = 0;
std::string baseDir;	
GeneticAlgorithmParams params;
int32_t popsize = 0;
UniformDistribution dist(0,1);

typedef GAFactory *(*CreateFactoryInstanceFunction)();

void SendFactoryAck(int factoryNumber, TCPPacketConnection &connection,bool canhelp)
{
	uint8_t buf[sizeof(int32_t)*3];
	MemorySerializer mser(0,0,buf,sizeof(int32_t)*3);

	mser.WriteInt32(GASERVER_COMMAND_FACTORYRESULT);
	mser.WriteInt32(factoryNumber);
	
	int val = 0;

	if (canhelp)
		val = 1;

	mser.WriteInt32(val);

	connection.WritePacket(buf,sizeof(int32_t)*3);
}

bool ProcessPacket(TCPPacketConnection &connection,const uint8_t *data, int datalength)
{
	MemorySerializer mser(data,datalength,0,0);
	int32_t cmd;
	
	mser.ReadInt32(&cmd);

	if (cmd == GASERVER_COMMAND_FACTORY)
	{
		// unload previous module
		
		if (factory)
			delete factory;
		if (moduleHandle)
			dlclose(moduleHandle);
		
		factory = 0;
		moduleHandle = 0;

		// process new module
		
		std::string moduleName;
		void *m;
		CreateFactoryInstanceFunction CreateFactoryInstance;
		GAFactoryParams *factoryParams;
		int32_t factoryNumber;

		if (!mser.ReadInt32(&factoryNumber))
		{
			std::cerr << "Error: Read invalid factory packet from server" << std::endl;
			return false;
		}
		
		if (!mser.ReadString(&moduleName))
		{
			std::cerr << "Error: Read invalid factory packet from server" << std::endl;
			return false;
		}

		std::string fullPath = baseDir + std::string("/") + moduleName;
		m = dlopen(fullPath.c_str(),RTLD_NOW);
		if (m == 0)
		{
			std::cout << "Warning: Couldn't load module " + fullPath << std::endl;
			SendFactoryAck(factoryNumber,connection,false);
			return true;
		}

		CreateFactoryInstance = (CreateFactoryInstanceFunction)dlsym(m,"CreateFactoryInstance");
		if (CreateFactoryInstance == 0)
		{
			std::cout << "Invalid module" << std::endl;
			dlclose(m);
			SendFactoryAck(factoryNumber,connection,false);
			return true;
		}
		
		GAFactory *fact = CreateFactoryInstance();
		
		if (fact == 0)
		{
			std::cout << "Couldn't create factory instance" << std::endl;
			dlclose(m);
			SendFactoryAck(factoryNumber,connection,false);
			return true;
		}

		GAFactoryParams *factParams = fact->CreateParamsInstance();
		if (factParams == 0)
		{
			std::cout << "Coulnd't create factory parameters instance" << std::endl;
			delete fact;
			dlclose(m);
			SendFactoryAck(factoryNumber,connection,false);
			return true;
		}
	
		if (!mser.ReadInt32(&popsize))
		{
			delete factParams;
			delete fact;
			dlclose(m);
			std::cerr << "Error: Read invalid factory packet from server" << std::endl;
			return false;
		}

		if (!factParams->Read(mser))
		{
			delete factParams;
			delete fact;
			dlclose(m);
			std::cerr << "Error: Read invalid factory packet from server" << std::endl;
			return false;
		}

		if (!params.Read(mser))
		{
			delete factParams;
			delete fact;
			dlclose(m);
			std::cerr << "Error: Read invalid factory packet from server" << std::endl;
			return false;
		}

		if (popsize <= 0)
		{
			delete factParams;
			delete fact;
			dlclose(m);
			std::cerr << "Error: Read invalid factory packet from server" << std::endl;
			return false;
		}

		if (!fact->Init(factParams))
		{
			std::cout << "Coulnd't initialize factory instance" << std::endl;
			delete factParams;
			delete fact;
			dlclose(m);
			SendFactoryAck(factoryNumber,connection,false);
			return true;
		}
		
		delete factParams;
		
		moduleHandle = m;
		factory = fact;

		SendFactoryAck(factoryNumber,connection,true);
	}
	else if (cmd == GASERVER_COMMAND_MIGRATEDGENOME)
	{
		// ignore this
		std::cout << "Received a late migrated genome" << std::endl;
	}
	else if (cmd == GASERVER_COMMAND_ISLANDSTOP) 
	{
		// ignore this
		std::cout << "Received island stop command, but algorithm has already finished" << std::endl;
	}
	else
	{
		std::cerr << "Error: " << std::endl;
		std::cerr << " Received invalid command from server" << std::endl;
		return false;
	}
	return true;
}

bool IslandAlgorithm::CalculateFitness(std::vector<GenomeWrapper> &population, int populationsize)
{
	int generation = GetCurrentGeneration();
	
	if (endserverloop)
		return false;

	if (prevgentime < 0)
	{
		prevgentime = GetCurrentTime();
	}
	else
	{
		double curtime = GetCurrentTime();
		double diff = curtime-prevgentime;
		int gentime = (int)(diff+1.0);
		
		prevgentime = curtime;
		
		uint8_t buf[sizeof(int32_t)*2];
		MemorySerializer mser(0,0,buf,sizeof(int32_t)*2);

		mser.WriteInt32(GASERVER_COMMAND_GENERATIONTIME);
		mser.WriteInt32(gentime);
		
		conn->WritePacket(buf,sizeof(int32_t)*2);
	}

	if (nextmigrationgeneration < 0)
	{
		nextmigrationgeneration = generation+20+(int)(5.0*dist.pickNumber()+0.5);
		std::cout << "Next migration generation is " << nextmigrationgeneration << std::endl;
	}
	
	std::list<DataPacket *> outqueue;
	
	// process incoming packets
	
	fd_set fdset;
	struct timeval tv;

	FD_ZERO(&fdset);
	FD_SET(conn->GetSocket(), &fdset);
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if (select(FD_SETSIZE, &fdset, 0, 0, &tv) == -1)
	{
		endserverloop = true;
		return false;
	}
	
	if (FD_ISSET(conn->GetSocket(), &fdset))
	{
		if (conn->GetAvailableDataLength() <= 0)
		{
			std::cerr << "Error: " << std::endl;
			std::cerr << " Server closed connection. " << std::endl;
			endserverloop = true;
			return false;
		}
		else
		{	
			if (!conn->Poll())
			{
				std::cerr << "Error: " << std::endl;
				std::cerr << " Error polling for new data: " << conn->getErrorString() << std::endl;
				endserverloop = true;
				return false;
			}
			
			DataPacket *p = 0;
			while ((p = conn->GetNextPacket()) != 0)
			{
				if (p->GetDataLength() < 4)
				{
					std::cerr << "Error: " << std::endl;
					std::cerr << " Received invalid packet from server" << std::endl;
					endserverloop = true;
					return false;
				}
				
				MemorySerializer mser(p->GetData(),p->GetDataLength(),0,0);
				int cmd;

				mser.ReadInt32(&cmd);
				if (cmd == GASERVER_COMMAND_ISLANDSTOP) // stop the current algorithm
				{
					std::cout << "Stopping current algorithm" << std::endl;
					delete p;
					return false;
				}
				
				if (cmd != GASERVER_COMMAND_MIGRATEDGENOME)
				{
					std::cerr << "Error: " << std::endl;
					std::cerr << " Received invalid packet (not a migrated genome) from server" << std::endl;
					endserverloop = true;
					return false;
				}

				//std::cout << "Received migrated genome" << std::endl;
				migrationqueue.push_back(p);
				while (migrationqueue.size() > maxqueuelen)
				{
					delete *(migrationqueue.begin());
					migrationqueue.pop_front();
				}
			}
		}
	}

	if (generation == nextmigrationgeneration)
	{
		// Add migrated genomes to the current generation
	
		while (!migrationqueue.empty())
		{
			DataPacket *p = *(migrationqueue.begin());
			int idx = (int)((double)populationsize*dist.pickNumber());
			Genome *g = 0;
			MemorySerializer mser(p->GetData(),p->GetDataLength(),0,0);
			int32_t dummy;

			migrationqueue.pop_front();
			mser.ReadInt32(&dummy); // command number has already been checked
			
			if (factory->ReadGenome(mser, &g))
			{
				// store received genome in current population and migrate the genome that's being replaced
				Genome *g2 = population[idx].GetGenome();
				population[idx] = GenomeWrapper(g);
			
				int len = factory->GetMaximalGenomeSize()+sizeof(int32_t);
				uint8_t *buf = new uint8_t[len];
				MemorySerializer mser(0,0,buf,len);
				
				mser.WriteInt32(GASERVER_COMMAND_MIGRATEDGENOME);
				factory->WriteGenome(mser, g2);
				outqueue.push_back(new DataPacket(buf, mser.GetBytesWritten()));

				delete g2;

				std::cout << "Swapped migrated genome" << std::endl;
			}
			
			delete p;
		}
	}
	
	if (!GeneticAlgorithm::CalculateFitness(population, populationsize))
		return false;

	// send outgoing packets

	if (generation == nextmigrationgeneration)
	{
		while (outqueue.size() < maxqueuelen)
		{
			int idx = (int)((double)populationsize*dist.pickNumber());
			Genome *g2 = population[idx].GetGenome();
			
			int len = factory->GetMaximalGenomeSize()+sizeof(int32_t);
			uint8_t *buf = new uint8_t[len];
			MemorySerializer mser(0,0,buf,len);
				
			mser.WriteInt32(GASERVER_COMMAND_MIGRATEDGENOME);
			factory->WriteGenome(mser, g2);
			outqueue.push_back(new DataPacket(buf, mser.GetBytesWritten()));
		}
		
		while (!outqueue.empty())
		{
			DataPacket *p = *(outqueue.begin());
			outqueue.pop_front();

			if (!conn->WritePacket(p->GetData(), p->GetDataLength()))
			{
				std::cerr << "Error: " << std::endl;
				std::cerr << " Error migrating genome: " << conn->getErrorString() << std::endl; 
				endserverloop = true;
				return false;
			}
			std::cout << "Sent migrated genome" << std::endl;
			delete p;
		}
		
		nextmigrationgeneration = -1;
	}
	
	time_t curtime = time(0);	
	
	if ((curtime - lastbesttime) > 5) // send best genome every five seconds
	{
		lastbesttime = curtime;
		
		const Genome *g = GetBestGenome();

		if (g != 0)
		{
			int len = sizeof(int32_t) + factory->GetMaximalGenomeSize() + factory->GetMaximalFitnessSize();
			uint8_t *buf = new uint8_t[len];
			MemorySerializer mser(0,0,buf,len);

			mser.WriteInt32(GASERVER_COMMAND_BESTLOCALGENOME);
			factory->WriteGenome(mser, g);
			factory->WriteGenomeFitness(mser, g);
			
			if (!conn->WritePacket(buf,mser.GetBytesWritten()))
			{	
				std::cerr << "Error: " << std::endl;
				std::cerr << " Error writing best genome: " << conn->getErrorString() << std::endl; 
				endserverloop = true;
				return false;
			}
			delete [] buf;
		}
	}

	if ((curtime - conn->GetLastWriteTime()) > 10) // write something every 10 seconds
	{
		uint8_t buf[sizeof(int32_t)];
		MemorySerializer mser(0,0,buf,sizeof(int32_t));

		mser.WriteInt32(GASERVER_COMMAND_KEEPALIVE);

		if (!conn->WritePacket(buf,sizeof(int32_t)))
		{
			std::cerr << "Error: " << std::endl;
			std::cerr << " Error writing keepalive packet: " << conn->getErrorString() << std::endl; 
			endserverloop = true;
			return false;
		}
	}
	
	return true;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		std::cerr << "Usage: " << std::endl;
		std::cerr << std::string(argv[0]) << " serverIP portnumber moduledir" << std::endl; 
		return -1;
	}

	uint32_t IP = ntohl(inet_addr(argv[1]));
	uint16_t port = (uint16_t)atoi(argv[2]);
	
	baseDir = std::string(argv[3]);
	
	TCPPacketConnection connection;

	std::cout << "Connecting to server" << std::endl;
	if (!connection.Open(IP,port))
	{
		std::cerr << "Error: " << std::endl;
		std::cerr << " Couldn't open connection: " << connection.getErrorString() << std::endl;
		return -1;
	}

	// Ok, connection opened, send identification
	uint8_t buf[sizeof(int32_t)];
	MemorySerializer mser(0,0,buf,sizeof(int32_t));

	mser.WriteInt32(GASERVER_COMMAND_ISLANDHELPER);

	std::cout << "Sending identification" << std::endl;
	if (!connection.WritePacket(buf,sizeof(int32_t)))
	{
		std::cerr << "Error: " << std::endl;
		std::cerr << " Error writing identification packet: " << connection.getErrorString() << std::endl;
		return -1;
	}

	std::cout << "Installing signal handlers" << std::endl;
	signal(SIGTERM,SignalHandler);
	signal(SIGHUP,SignalHandler);
	signal(SIGQUIT,SignalHandler);
	signal(SIGABRT,SignalHandler);
	signal(SIGINT,SignalHandler);

	// Ok, wait for incoming data from the server
	while (!endserverloop)
	{
		fd_set fdset;
		struct timeval tv;

		FD_ZERO(&fdset);
		FD_SET(connection.GetSocket(), &fdset);
		tv.tv_sec = 0;
		tv.tv_usec = 100;

		if (select(FD_SETSIZE, &fdset, 0, 0, &tv) == -1)
		{
			endserverloop = true;
			continue;
		}
		
		if (FD_ISSET(connection.GetSocket(), &fdset))
		{
			if (connection.GetAvailableDataLength() <= 0)
			{
				std::cerr << "Error: " << std::endl;
				std::cerr << " Server closed connection. " << std::endl;
				return -1;
			}
			else
			{	
				if (!connection.Poll())
				{
					std::cerr << "Error: " << std::endl;
					std::cerr << " Error polling for new data: " << connection.getErrorString() << std::endl;
					return -1;
				}				
			}
		}

		time_t curtime = time(0);	

		if ((curtime - connection.GetLastWriteTime()) > 10) // write something every 10 seconds
		{
			uint8_t buf[sizeof(int32_t)];
			MemorySerializer mser(0,0,buf,sizeof(int32_t));

			mser.WriteInt32(GASERVER_COMMAND_KEEPALIVE);

			if (!connection.WritePacket(buf,sizeof(int32_t)))
			{
				std::cerr << "Error: " << std::endl;
				std::cerr << " Error writing keepalive packet: " << connection.getErrorString() << std::endl; 
				return -1;
			}
		}

		DataPacket *p = 0;
		if ((p = connection.GetNextPacket()) != 0)
		{
			if (p->GetDataLength() < 4)
			{
				std::cerr << "Error: " << std::endl;
				std::cerr << " Received invalid packet from server" << std::endl;
				return -1;
			}
		
			if (!ProcessPacket(connection,p->GetData(),p->GetDataLength()))
				return -1;
					
			delete p;
		}

		if (factory) // ok, got a factory, run the algorithm
		{
			IslandAlgorithm ga(&connection);

			bool ret = ga.Run(*factory, popsize, &params);

			if (ret && ga.GetBestGenome() != 0)
			{
				int len = sizeof(int32_t) + factory->GetMaximalGenomeSize() + factory->GetMaximalFitnessSize();
				uint8_t *buf = new uint8_t[len];
				
				MemorySerializer mser(0,0,buf,len);

				mser.WriteInt32(GASERVER_COMMAND_BESTLOCALGENOME);
				factory->WriteGenome(mser, ga.GetBestGenome());
				factory->WriteGenomeFitness(mser, ga.GetBestGenome());
				
				if (!connection.WritePacket(buf,mser.GetBytesWritten()))
				{	
					std::cerr << "Error: " << std::endl;
					std::cerr << " Error writing best genome: " << connection.getErrorString() << std::endl; 
					return -1;
				}
				delete [] buf;
			}

			uint8_t buf[sizeof(int32_t)];
			MemorySerializer mser(0,0,buf,sizeof(int32_t));

			mser.WriteInt32(GASERVER_COMMAND_ISLANDFINISHED);
			if (!connection.WritePacket(buf,sizeof(int32_t)))
			{
				std::cerr << "Error: " << std::endl;
				std::cerr << " Error writing end of GA packet: " << connection.getErrorString() << std::endl; 
				return -1;
			}
			
			ga.ClearBestGenome();
			
			delete factory;
			dlclose(moduleHandle);
			factory = 0;
			moduleHandle = 0;
		}
	}
	
	if (factory)
		delete factory;
	if (moduleHandle)
		dlclose(moduleHandle);
	
	return 0;
}

