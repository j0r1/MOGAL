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

#include "geneticalgorithm.h"
#include "gafactory.h"
#include "gaserver.h"
#include "gamodule.h"
#include <enut/tcppacketsocket.h>
#include <enut/tcpv4socket.h>
#include <enut/packet.h>
#include <enut/socketwaiter.h>
#include <serut/dummyserializer.h>
#include <serut/memoryserializer.h>
#include <serut/tcpserializer.h>
#include <algorithm>

namespace mogal
{

#define GENETICALGORITHM_MINPOPSIZE				5
#define GENETICALGORITHM_REPORTINTERVAL				10
#define GENETICALGORITHM_KEEPALIVEINTERVAL			10
	
#define GENETICALGORITHM_ERRSTR_POPSIZETOOSMALL			"The population size must be at least 5"
#define GENETICALGORITHM_ERRSTR_CANTCONNECTTOSERVER		"Unable to connect to the specified server: "
#define GENETICALGORITHM_ERRSTR_CANTPOLL			"Error polling for new packets: "
#define GENETICALGORITHM_ERRSTR_CANTSENDFACTORYDATA		"Couldn't send factory data: "
#define GENETICALGORITHM_ERRSTR_CANTSENDID			"Error writing client identifier: "
#define GENETICALGORITHM_ERRSTR_CONNECTIONCLOSED		"Lost connection with the server"
#define GENETICALGORITHM_ERRSTR_FACTORYNOTINITIALIZED		"The specified genetic algorithm factory is not initialized"
#define GENETICALGORITHM_ERRSTR_INVALIDPACKET			"Received an invalid packet from the server"
#define GENETICALGORITHM_ERRSTR_NOHELPERS			"The server has no connected helper clients"
#define GENETICALGORITHM_ERRSTR_SERVERBUSY			"The server is already processing another genetic algorithm"
#define GENETICALGORITHM_ERRSTR_CANTLOADMODULE			"Can't load the specified module: "
#define GENETICALGORITHM_ERRSTR_CANTCREATEFACTORY		"The module was not able to create a valid factory"
#define GENETICALGORITHM_ERRSTR_CANTINITFACTORY			"Unable to initialize the factory: "
#define GENETICALGORITHM_ERRSTR_CANTREADBESTGENOMECOUNT		"Couldn't read best genome count: "
#define GENETICALGORITHM_ERRSTR_CANTREADGENOME			"Couldn't genome: "
#define GENETICALGORITHM_ERRSTR_CANTREADGENOMEFITNESS		"Couldn't genome fitness: "
#define GENETICALGORITHM_ERRSTR_NOBESTGENOMES			"The best genomes set is empty"
#define GENETICALGORITHM_ERRSTR_NOCURRENTFACTORY		"No current factory is available"
#define GENETICALGORITHM_ERRSTR_CANTWRITEKEEPALIVE		"Error writing keepalive packet: "
#define GENETICALGORITHM_ERRSTR_CANTWRITEFACTORYPARAMETERS	"Unable to serialize the factory parameters: "

GeneticAlgorithmParams::GeneticAlgorithmParams()
{ 
	m_beta = 2.5; 
	m_elitism = true; 
	m_alwaysIncludeBest = true; 
	m_crossoverRate = 0.9; 
}

GeneticAlgorithmParams::GeneticAlgorithmParams(double beta, bool elitism, bool includeBest, double crossoverRate)
{ 
	m_beta = beta; 
	m_elitism = elitism; 
	m_alwaysIncludeBest = includeBest; 
	m_crossoverRate = crossoverRate;
}

GeneticAlgorithmParams::~GeneticAlgorithmParams()
{
}

bool GeneticAlgorithmParams::write(serut::SerializationInterface &si)
{
	if (!si.writeDouble(m_beta))
	{
		setErrorString(si.getErrorString());
		return false;
	}

	if (!si.writeDouble(m_crossoverRate))
	{
		setErrorString(si.getErrorString());
		return false;
	}

	int32_t e,a;

	e = (m_elitism)?1:0;
	a = (m_alwaysIncludeBest)?1:0;

	if (!si.writeInt32(e))
	{
		setErrorString(si.getErrorString());
		return false;
	}
	if (!si.writeInt32(a))
	{
		setErrorString(si.getErrorString());
		return false;
	}

	return true;
}		

bool GeneticAlgorithmParams::read(serut::SerializationInterface &si)
{
	double b,c;
	int32_t e,a;

	if (!si.readDouble(&b))
	{
		setErrorString(si.getErrorString());
		return false;
	}

	if (!si.readDouble(&c))
	{
		setErrorString(si.getErrorString());
		return false;
	}

	if (!si.readInt32(&e))
	{
		setErrorString(si.getErrorString());
		return false;
	}

	if (!si.readInt32(&a))
	{
		setErrorString(si.getErrorString());
		return false;
	}

	m_beta = b;
	m_crossoverRate = c;
	m_elitism = (e == 0)?false:true;
	m_alwaysIncludeBest = (a == 0)?false:true;

	return true;
}


GeneticAlgorithm::GeneticAlgorithm()
{
	m_pGAModule = 0;
	m_pLoadedFactory = 0;
	m_pCurrentFactory = 0;
}

GeneticAlgorithm::~GeneticAlgorithm()
{
	clearBestGenomes();
}

void GeneticAlgorithm::clearBestGenomes()
{
	while (!m_bestGenomes.empty())
	{
		delete *(m_bestGenomes.begin());
		m_bestGenomes.pop_front();
	}
	if (m_pLoadedFactory)
	{
		delete m_pLoadedFactory;
		m_pLoadedFactory = 0;
	}
	if (m_pGAModule)
	{
		delete m_pGAModule;
		m_pGAModule = 0;
	}
	m_pCurrentFactory = 0;
}

bool GeneticAlgorithm::run(GAFactory &factory, size_t populationSize, const GeneticAlgorithmParams *pParams)
{
	if (populationSize < GENETICALGORITHM_MINPOPSIZE)
	{
		setErrorString(GENETICALGORITHM_ERRSTR_POPSIZETOOSMALL);
		return false;
	}
	
	clearBestGenomes();

	m_pCurrentFactory = &factory;
	
	std::vector<GenomeWrapper> population(populationSize);
	std::vector<GenomeWrapper> newPopulation(populationSize);
	
	GeneticAlgorithmParams params;
	if (pParams)
		params = *pParams;
	
	for (int i = 0 ; i < (int)populationSize ; i++)
		population[i] = GenomeWrapper(factory.createNewGenome(), -1, -1, i);

	bool done = false;
	bool generationInfoChanged = true;
	time_t prevTime = time(0);
	
	m_generationCount = 0;
	
	// Set the current algorithm pointer and parameters
	factory.setCurrentAlgorithm(this, params);
	
	factory.onGeneticAlgorithmStart();
	
	while (!done)
	{
		if (!onAlgorithmLoop(factory, generationInfoChanged))
		{
			for (size_t i = 0 ; i < populationSize ; i++)
				delete population[i].getGenome();
			
			factory.onGeneticAlgorithmStop();
			factory.setCurrentAlgorithm(0, params);
			return false;
		}
		
		// Calculate fitness of the genomes

		if (!calculateFitness(population))
		{
			for (size_t i = 0 ; i < populationSize ; i++)
				delete population[i].getGenome();

			factory.onGeneticAlgorithmStop();
			factory.setCurrentAlgorithm(0, params);
			return false;
		}

		// Sort population according to fitness
		
		factory.sort(population);		
		factory.onSortedPopulation(population);

		// Update best genomes

		factory.updateBestGenomes(population);
			
		// Build new population
		
		factory.breed(population, newPopulation);
	
		// Got new population, introduce mutations

		factory.introduceMutations(newPopulation);

		generationInfoChanged = false;

		factory.onGeneticAlgorithmStep(m_generationCount, &generationInfoChanged, &done);
		
		m_generationCount++;
		
		for (size_t i = 0 ; i < populationSize ; i++)
		{
			delete population[i].getGenome();
			population[i] = GenomeWrapper(newPopulation[i]); // copy constructor!
		}

		time_t currentTime = time(0);
		if (currentTime - prevTime > GENETICALGORITHM_REPORTINTERVAL)
		{
			prevTime = currentTime;
			onCurrentBest(m_bestGenomes);
		}
	}
	
	onCurrentBest(m_bestGenomes);

	for (size_t i = 0 ; i < populationSize ; i++)
		delete population[i].getGenome();

	factory.onGeneticAlgorithmStop();
	factory.setCurrentAlgorithm(0, params);

	return true;
}

bool GeneticAlgorithm::run(const nut::NetworkLayerAddress &serverAddress, uint16_t serverPort, 
		           const std::string &factoryName, size_t populationSize, 
		           GAFactory &factory, const GeneticAlgorithmParams *pParams)
{
	if (populationSize < GENETICALGORITHM_MINPOPSIZE)
	{
		setErrorString(GENETICALGORITHM_ERRSTR_POPSIZETOOSMALL);
		return false;
	}
	
	clearBestGenomes();
	m_pCurrentFactory = &factory;

	GeneticAlgorithmParams params;
	if (pParams)
		params = *pParams;
	
	// connect to the specified server
	
	nut::TCPv4Socket sock;
	nut::TCPPacketSocket connection(&sock, false, false, GASERVER_MAXPACKSIZE, sizeof(uint32_t), GASERVER_PACKID);

	// Set socket buffer sizes
	int val = 1024*1024;
	sock.setSocketOption(SOL_SOCKET, SO_SNDBUF, &val, sizeof(int));
	val = 1024*1024;
	sock.setSocketOption(SOL_SOCKET, SO_RCVBUF, &val, sizeof(int));

	feedbackStatus("Connecting to server");
	if (!sock.create())
	{
		setErrorString(sock.getErrorString());
		return false;
	}

	if (!sock.connect(serverAddress, serverPort))
	{
		setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTCONNECTTOSERVER) + connection.getErrorString());
		return false;
	}

	// Write identifier
	{
		uint8_t buf[sizeof(int32_t)];
		serut::MemorySerializer mser(0, 0, buf, sizeof(int32_t));

		mser.writeInt32(GASERVER_COMMAND_CLIENT);

		feedbackStatus("Sending identification to server");
		if (!connection.write(buf, sizeof(int32_t)))
		{
			setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTSENDID) + connection.getErrorString());
			return false;
		}
	}

	feedbackStatus("Waiting for reply from server");
	bool done = false;
	while (!done)
	{
		if (!connection.waitForData(0, 200000))
		{
			setErrorString(connection.getErrorString());
			return false;
		}

		if (connection.isDataAvailable())
		{
			size_t len = 0;

			connection.getAvailableDataLength(len);
			if (len == 0)
			{
				setErrorString(GENETICALGORITHM_ERRSTR_CONNECTIONCLOSED);
				return false;
			}
			if (!connection.poll())
			{
				setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTPOLL) + connection.getErrorString());
				return false;
			}

			nut::Packet packet;
			while (!done && connection.read(packet))
			{
				if (packet.getLength() < sizeof(int32_t))
				{
					setErrorString(GENETICALGORITHM_ERRSTR_INVALIDPACKET);
					return false;
				}
				
				if (packet.getLength() != 4)
				{
					setErrorString(GENETICALGORITHM_ERRSTR_INVALIDPACKET);
					return false;
				}

				serut::MemorySerializer mser(packet.getData(), packet.getLength(), 0, 0);
				int32_t cmd;

				mser.readInt32(&cmd);
				if (cmd == GASERVER_COMMAND_ACCEPT)
				{
					done = true;
				}
				else if (cmd == GASERVER_COMMAND_BUSY)
				{
					setErrorString(GENETICALGORITHM_ERRSTR_SERVERBUSY);
					return false;
				}
				else if (cmd == GASERVER_COMMAND_NOHELPERS)
				{
					setErrorString(GENETICALGORITHM_ERRSTR_NOHELPERS);
					return false;
				}
				else
				{
					setErrorString(GENETICALGORITHM_ERRSTR_INVALIDPACKET);
					return false;
				}
			}
		}
	}

	feedbackStatus("Got positive reply from server, sending genetic algorithm data");

	// Send factory name and parameters
	{
		// We'll use a dummy serializer to find out how large the packet should be
		// TODO: replace this with a dynamic memory serializer

		serut::DummySerializer ds;	

		ds.writeInt32(GASERVER_COMMAND_FACTORY);
		ds.writeInt32(0); // space reserved for factory number
		ds.writeString(factoryName);
		ds.writeInt32(populationSize);
		
		if (factory.getCurrentParameters() == 0)
		{
			setErrorString(GENETICALGORITHM_ERRSTR_FACTORYNOTINITIALIZED);
			return false;
		}
		
		if (!factory.getCurrentParameters()->write(ds))
		{
			setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTWRITEFACTORYPARAMETERS) + factory.getCurrentParameters()->getErrorString());
			return false;
		}

		params.write(ds);

		uint8_t *pBuf = new uint8_t[ds.getBytesWritten()];
		serut::MemorySerializer mser(0, 0, pBuf, ds.getBytesWritten());

		mser.writeInt32(GASERVER_COMMAND_FACTORY);
		mser.writeInt32(0); // space reserved for factory number
		mser.writeString(factoryName);
		mser.writeInt32(populationSize);
		
		factory.getCurrentParameters()->write(mser);
		params.write(mser);

		if (!connection.write(pBuf, ds.getBytesWritten()))
		{
			delete [] pBuf;
			setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTSENDFACTORYDATA) + connection.getErrorString());
			return false;
		}
		
		delete pBuf;
	}

	feedbackStatus("Running genetic algorithm");
	
	done = false;
	while (!done)
	{
		if (!connection.waitForData(0, 200000))
		{
			setErrorString(connection.getErrorString());
			return false;
		}

		if (connection.isDataAvailable())
		{
			size_t len = 0;

			connection.getAvailableDataLength(len);
			if (len == 0)
			{
				setErrorString(GENETICALGORITHM_ERRSTR_CONNECTIONCLOSED);
				return false;
			}
			if (!connection.poll())
			{
				setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTPOLL) + connection.getErrorString());
				return false;
			}

			nut::Packet packet;
			while (!done && connection.read(packet))
			{
				if (packet.getLength() < sizeof(int32_t))
				{
					setErrorString(GENETICALGORITHM_ERRSTR_INVALIDPACKET);
					return false;
				}
				
				serut::MemorySerializer mser(packet.getData(), packet.getLength(), 0, 0);
				int32_t cmd;

				mser.readInt32(&cmd);

				if (cmd == GASERVER_COMMAND_KEEPALIVE)
				{
					// ignore this
				}
				else if (cmd == GASERVER_COMMAND_CURRENTBEST)
				{
					if (!processNewBestGenomes(factory, mser))
						return false;
				}
				else if (cmd == GASERVER_COMMAND_RESULT)
				{
					feedbackStatus("Received final result from server");
					if (!processNewBestGenomes(factory, mser))
						return false;

					done = true;
				}
				else if (cmd == GASERVER_COMMAND_NOHELPERS)
				{
					setErrorString(GENETICALGORITHM_ERRSTR_NOHELPERS);
					return false;
				}
				else
				{
					setErrorString(GENETICALGORITHM_ERRSTR_INVALIDPACKET);
					return false;
				}
			}
		}

		if (!done)
		{
			time_t currentTime = time(0);

			if ((currentTime - connection.getLastWriteTime()) > GENETICALGORITHM_KEEPALIVEINTERVAL)
			{
				uint8_t buf[sizeof(int32_t)];
				serut::MemorySerializer mser(0, 0, buf, sizeof(int32_t));
				mser.writeInt32(GASERVER_COMMAND_KEEPALIVE);

				if (!connection.write(buf, sizeof(int32_t)))
				{
					setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTWRITEKEEPALIVE) + connection.getErrorString());
					return false;
				}
			}
		}
	}

	feedbackStatus("Genetic algorithm finished successfully");

	return true;
}

bool GeneticAlgorithm::run(const nut::NetworkLayerAddress &serverAddress, uint16_t serverPort, 
		           const std::string &factoryName, size_t populationSize, 
		 	   const std::string &baseDir, const GAFactoryParams *pFactoryParams, 
			   const GeneticAlgorithmParams *pParams)
{
	GAModule *pModule = 0;
	GAFactory *pFactory = 0;
	
	clearBestGenomes();
	
	feedbackStatus("Loading module");
	
	// try to load factoryName

	pModule = new GAModule();
	if (!pModule->open(baseDir, factoryName))
	{
		setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTLOADMODULE) + pModule->getErrorString());
		delete pModule;
		return false;
	}

	pFactory = pModule->createFactoryInstance();
	if (pFactory == 0)
	{
		setErrorString(GENETICALGORITHM_ERRSTR_CANTCREATEFACTORY);
		delete pModule;
		return false;
	}

	if (!pFactory->init(pFactoryParams))
	{
		setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTINITFACTORY) + pFactory->getErrorString());
		delete pFactory;
		delete pModule;
		return false;
	}

	feedbackStatus("Running genetic algorithm");
	bool retval = run(serverAddress, serverPort, factoryName, populationSize,*pFactory, pParams);
	
	m_pLoadedFactory = pFactory;
	m_pGAModule = pModule;
	
	return retval;
}

bool GeneticAlgorithm::run(const std::string &factoryName, size_t populationSize, 
		 	   const std::string &baseDir, const GAFactoryParams *pFactoryParams, 
			   const GeneticAlgorithmParams *pParams)
{
	GAModule *pModule = 0;
	GAFactory *pFactory = 0;
	
	clearBestGenomes();
	
	feedbackStatus("Loading module");
	
	// try to load factoryName

	pModule = new GAModule();
	if (!pModule->open(baseDir, factoryName))
	{
		setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTLOADMODULE) + pModule->getErrorString());
		delete pModule;
		return false;
	}

	pFactory = pModule->createFactoryInstance();
	if (pFactory == 0)
	{
		setErrorString(GENETICALGORITHM_ERRSTR_CANTCREATEFACTORY);
		delete pModule;
		return false;
	}

	if (!pFactory->init(pFactoryParams))
	{
		setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTINITFACTORY) + pFactory->getErrorString());
		delete pFactory;
		delete pModule;
		return false;
	}

	feedbackStatus("Running genetic algorithm");
	bool retval = run(*pFactory, populationSize, pParams);

	m_pLoadedFactory = pFactory;
	m_pGAModule = pModule;

	return retval;
}

bool GeneticAlgorithm::processNewBestGenomes(GAFactory &factory, serut::SerializationInterface &si)
{
	int32_t count = 0;
	std::list<Genome *> genomeList;
	
	if (!si.readInt32(&count))
	{
		setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTREADBESTGENOMECOUNT) + si.getErrorString());
		return false;
	}
	
	for (int32_t i = 0 ; i < count ; i++)
	{
		Genome *pGenome = 0;

		if (!factory.readGenome(si, &pGenome))
		{
			setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTREADGENOME) + factory.getErrorString());
			while (!genomeList.empty())
			{
				delete *(genomeList.begin());
				genomeList.pop_front();
			}
			return false;
		}

		if (!factory.readGenomeFitness(si, pGenome))
		{
			delete pGenome;
			while (!genomeList.empty())
			{
				delete *(genomeList.begin());
				genomeList.pop_front();
			}
			setErrorString(std::string(GENETICALGORITHM_ERRSTR_CANTREADGENOMEFITNESS) + factory.getErrorString());
			return false;
		}

		genomeList.push_back(pGenome);
	}

	while (!m_bestGenomes.empty())
	{
		delete *(m_bestGenomes.begin());
		m_bestGenomes.pop_front();
	}
	
	m_bestGenomes = genomeList;
	
	onCurrentBest(m_bestGenomes);
	return true;
}

bool GeneticAlgorithm::calculateFitness(std::vector<GenomeWrapper> &population)
{
	for (size_t i = 0 ; i < population.size() ; i++)
		population[i].getGenome()->calculateFitness();
	return true;
}

bool GeneticAlgorithm::onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged)
{
	return true;
}

void GeneticAlgorithm::getBestGenomes(std::list<Genome *> &genomes) const
{
	genomes = m_bestGenomes;
}

void GeneticAlgorithm::getBestGenomes(std::vector<Genome *> &genomes) const
{
	genomes.resize(m_bestGenomes.size());

	std::list<Genome *>::const_iterator it;
	int i;

	for (it = m_bestGenomes.begin(), i = 0 ; i < genomes.size() ; it++, i++)
		genomes[i] = *it;
}

void GeneticAlgorithm::setBestGenomes(const std::list<Genome *> &genomeList)
{
	std::list<Genome *>::const_iterator it;
	std::list<Genome *> newBestGenomes;

	for (it = genomeList.begin() ; it != genomeList.end() ; it++)
		newBestGenomes.push_back((*it)->clone());
	
	while (!m_bestGenomes.empty())
	{
		delete *(m_bestGenomes.begin());
		m_bestGenomes.pop_front();
	}

	m_bestGenomes = newBestGenomes;
}

Genome* GeneticAlgorithm::selectPreferredGenome() const
{
	if (m_bestGenomes.empty())
	{
		setErrorString(GENETICALGORITHM_ERRSTR_NOBESTGENOMES);
		return 0;
	}
	
	if (m_pCurrentFactory == 0)
	{
		setErrorString(GENETICALGORITHM_ERRSTR_NOCURRENTFACTORY);
		return 0;
	}

	Genome *pGenome = m_pCurrentFactory->selectPreferredGenome(m_bestGenomes);
	if (pGenome == 0)
	{
		setErrorString(m_pCurrentFactory->getErrorString());
		return 0;
	}
	return pGenome;
}

} // end namespace

