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
#include <mogal/gafactorysingleobjective.h>
#include <mogal/genome.h>
#include <mogal/mpigeneticalgorithm.h>
#include <mogal/randomnumbergenerator.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

class DistGAFactoryParams : public mogal::GAFactoryParams
{
public:
	DistGAFactoryParams(double x, double y, double width);
	~DistGAFactoryParams();

	double getX() const								{ return m_x; }
	double getY() const								{ return m_y; }
	double getWidth() const								{ return m_width; }

	bool write(serut::SerializationInterface &si) const;
	bool read(serut::SerializationInterface &si);
private:
	double m_x, m_y, m_width;
};

class DistGAFactory;

class DistGenome : public mogal::Genome
{
public:
	DistGenome(double width, DistGAFactory *pFactory);
	DistGenome(double x, double y, DistGAFactory *pFactory);
	~DistGenome();

	bool calculateFitness();
	bool isFitterThan(const mogal::Genome *pGenome) const;
	Genome *reproduce(const mogal::Genome *pGenome) const;
	mogal::Genome *clone() const;
	void mutate();
	
	std::string getFitnessDescription() const;
private:
	double m_x, m_y;
	double m_fitness;
	DistGAFactory *m_pFactory;

	friend class DistGAFactory;
};

class SimpleRndGen : public mogal::RandomNumberGenerator
{
public:
	SimpleRndGen() { }
	~SimpleRndGen() { }
	
	double pickRandomNumber() const
	{
		double randomNumber = (1.0*((double)rand()/(RAND_MAX + 1.0)));

		return randomNumber;
	}
};

class DistGAFactory : public mogal::GAFactorySingleObjective
{
public:
	DistGAFactory();
	~DistGAFactory();

	mogal::GAFactoryParams *createParamsInstance() const;

	bool init(const mogal::GAFactoryParams *pParams);
	const mogal::GAFactoryParams *getCurrentParameters() const;

	const mogal::RandomNumberGenerator *getRandomNumberGenerator() const
	{
		return &m_rndGen;
	}

	mogal::Genome *createNewGenome() const;
	size_t getMaximalGenomeSize() const;
	size_t getMaximalFitnessSize() const;
	bool writeGenome(serut::SerializationInterface &si, const mogal::Genome *pGenome) const;
	bool writeGenomeFitness(serut::SerializationInterface &si, const mogal::Genome *pGenome) const;
	bool writeCommonGenerationInfo(serut::SerializationInterface &si) const;
	bool readGenome(serut::SerializationInterface &si, mogal::Genome **pGenome) const;
	bool readGenomeFitness(serut::SerializationInterface &si, mogal::Genome *pGenome) const;
	bool readCommonGenerationInfo(serut::SerializationInterface &si);

	void onGeneticAlgorithmStep(int generation, bool *generationInfoChanged, bool *stopAlgorithm);
	void onGeneticAlgorithmStart();
	void onGeneticAlgorithmStop();
	void onSortedPopulation(const std::vector<mogal::GenomeWrapper> &population);

	double getFunctionCenterX() const;
	double getFunctionCenterY() const;
private:
	DistGAFactoryParams m_factoryParams;
	double m_functionCenterX;
	double m_functionCenterY;
	double m_initialSearchWidth;
	SimpleRndGen m_rndGen;
};

// Genome implementation

DistGenome::DistGenome(double width, DistGAFactory *pFactory)
{
	m_x = (width*((double)rand()/(RAND_MAX + 1.0)))-10.0;
	m_y = (width*((double)rand()/(RAND_MAX + 1.0)))-10.0;
	m_pFactory = pFactory;
}

DistGenome::DistGenome(double x, double y, DistGAFactory *pFactory)
{
	m_x = x;
	m_y = y;
	m_pFactory = pFactory;
}

DistGenome::~DistGenome()
{
}

bool DistGenome::calculateFitness()
{
	double cx = m_pFactory->getFunctionCenterX();
	double cy = m_pFactory->getFunctionCenterY();

	m_fitness = (m_x-cx)*(m_x-cx) + (m_y-cy)*(m_y-cy);
	
	return true;
}

bool DistGenome::isFitterThan(const mogal::Genome *pGenome) const
{
	const DistGenome *pDistGenome = (const DistGenome *)pGenome;

	if (m_fitness < pDistGenome->m_fitness)
		return true;
	return false;
}

mogal::Genome *DistGenome::reproduce(const mogal::Genome *pGenome) const
{
	double randomNumber = (1.0*((double)rand()/(RAND_MAX + 1.0)));
	const DistGenome *pDistGenome = (const DistGenome *)pGenome;
	DistGenome *pNewGenome = 0;
	
	if (randomNumber < 0.5)
		pNewGenome = new DistGenome(m_x, pDistGenome->m_y, m_pFactory);
	else
		pNewGenome = new DistGenome(pDistGenome->m_x, m_y, m_pFactory);

	return pNewGenome;
}

mogal::Genome *DistGenome::clone() const
{
	DistGenome *pNewGenome = new DistGenome(m_x, m_y, m_pFactory);

	pNewGenome->m_fitness = m_fitness;
	return pNewGenome;
}

void DistGenome::mutate()
{ 
	double randomNumber = (1.0*((double)rand()/(RAND_MAX + 1.0)));
	double smallRandomNumber = (0.1*((double)rand()/(RAND_MAX + 1.0)))-0.05;

	if (randomNumber < 0.33)
		m_x += smallRandomNumber;
	else if (randomNumber < 0.66)
		m_y += smallRandomNumber;
	else
	{
		// don't mutate
	}
}

std::string DistGenome::getFitnessDescription() const
{
	char str[256];

	sprintf(str, "%g (%g, %g)", m_fitness, m_x, m_y);

	return std::string(str);
}

// Factory implementation

DistGAFactory::DistGAFactory() : m_factoryParams(0, 0, 0)
{
}

DistGAFactory::~DistGAFactory()
{
}

mogal::GAFactoryParams *DistGAFactory::createParamsInstance() const
{
	return new DistGAFactoryParams(0, 0, 0);
}

bool DistGAFactory::init(const mogal::GAFactoryParams *pParams)
{
	const DistGAFactoryParams *pFactoryParams = (const DistGAFactoryParams *)pParams;

	m_functionCenterX = pFactoryParams->getX();
	m_functionCenterY = pFactoryParams->getY();
	m_initialSearchWidth = pFactoryParams->getWidth();

	m_factoryParams = *pFactoryParams; // we have to make a copy since pParams may be deleted

	return true;
}

const mogal::GAFactoryParams *DistGAFactory::getCurrentParameters() const
{
	return &m_factoryParams;
}

mogal::Genome *DistGAFactory::createNewGenome() const
{
	return new DistGenome(m_initialSearchWidth, (DistGAFactory *)this);
}

size_t DistGAFactory::getMaximalGenomeSize() const
{
	return sizeof(double)*2; // for x and y coordinate
}

size_t DistGAFactory::getMaximalFitnessSize() const
{
	return sizeof(double); // for fitness
}

bool DistGAFactory::writeGenome(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	const DistGenome *pDistGenome = (const DistGenome *)pGenome;	

	if (!si.writeDouble(pDistGenome->m_x))
	{
		setErrorString("Couldn't write genome x coordinate");
		return false;
	}
	if (!si.writeDouble(pDistGenome->m_y))
	{
		setErrorString("Couldn't write genome y coordinate");
		return false;
	}
	return true;
}

bool DistGAFactory::writeGenomeFitness(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	const DistGenome *pDistGenome = (const DistGenome *)pGenome;	

	if (!si.writeDouble(pDistGenome->m_fitness))
	{
		setErrorString("Couldn't write genome fitness");
		return false;
	}
	return true;
}

bool DistGAFactory::writeCommonGenerationInfo(serut::SerializationInterface &si) const
{
	// We don't need this
	return true;
}

bool DistGAFactory::readGenome(serut::SerializationInterface &si, mogal::Genome **pGenome) const
{
	double x, y;

	if (!si.readDouble(&x))
	{
		setErrorString("Couldn't read genome x coordinate");
		return false;
	}
	if (!si.readDouble(&y))
	{
		setErrorString("Couldn't read genome y coordinate");
		return false;
	}

	*pGenome = new DistGenome(x, y, (DistGAFactory *)this);
	return true;
}

bool DistGAFactory::readGenomeFitness(serut::SerializationInterface &si, mogal::Genome *pGenome) const
{
	double fitness;

	if (!si.readDouble(&fitness))
	{
		setErrorString("Couldn't read genome fitness");
		return false;
	}

	DistGenome *pDistGenome = (DistGenome *)pGenome;
	pDistGenome->m_fitness = fitness;

	return true;
}

bool DistGAFactory::readCommonGenerationInfo(serut::SerializationInterface &si)
{
	// We don't need this
	return true;
}

void DistGAFactory::onGeneticAlgorithmStep(int generation, bool *generationInfoChanged, bool *pStopAlgorithm)
{ 
	std::list<mogal::Genome *> bestGenomes;
	std::list<mogal::Genome *>::const_iterator it;

	getCurrentAlgorithm()->getBestGenomes(bestGenomes);

	std::cout << "Generation " << generation << ": ";
	
	for (it = bestGenomes.begin() ; it != bestGenomes.end() ; it++)
	{
		const mogal::Genome *pGenome = *it;
		
		std::cout << "  " << pGenome->getFitnessDescription() << std::endl;
	}

	if (generation >= 100)
		*pStopAlgorithm = true;
}

void DistGAFactory::onGeneticAlgorithmStart()
{
	// This gets called when the genetic algorithm is started. No 
	// implementation is necessary for the algorithm to work.
}

void DistGAFactory::onGeneticAlgorithmStop()
{
	// This gets called when the genetic algorithm is stopped. No 
	// implementation is necessary for the algorithm to work.
}

void DistGAFactory::onSortedPopulation(const std::vector<mogal::GenomeWrapper> &population)
{
	// This gets called each time the population is sorted. No
	// implementation is necessary for the algorithm to work.
}

double DistGAFactory::getFunctionCenterX() const
{
	return m_functionCenterX;
}

double DistGAFactory::getFunctionCenterY() const
{
	return m_functionCenterY;
}

DistGAFactoryParams::DistGAFactoryParams(double x, double y, double width)
{
	m_x = x;
	m_y = y;
	m_width = width;
}

DistGAFactoryParams::~DistGAFactoryParams()
{
}

bool DistGAFactoryParams::write(serut::SerializationInterface &si) const
{
	if (!si.writeDouble(m_x))
	{
		setErrorString("Couldn't write X coordinate for test function");
		return false;
	}
	if (!si.writeDouble(m_y))
	{
		setErrorString("Couldn't write Y coordinate for test function");
		return false;
	}
	if (!si.writeDouble(m_width))
	{
		setErrorString("Couldn't write initial search width");
		return false;
	}
	return true;
}

bool DistGAFactoryParams::read(serut::SerializationInterface &si)
{
	if (!si.readDouble(&m_x))
	{
		setErrorString("Couldn't read X coordinate for test function");
		return false;
	}
	if (!si.readDouble(&m_y))
	{
		setErrorString("Couldn't read Y coordinate for test function");
		return false;
	}
	if (!si.readDouble(&m_width))
	{
		setErrorString("Couldn't read initial search width");
		return false;
	}

	return true;
}

int main(int argc, char *argv[])
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

	int returnStatus = 0;
	DistGAFactoryParams factoryParams(-1,-1,20);

	if (myRank == 0)
	{
		DistGAFactory factory;

		factory.init(&factoryParams);
	
		mogal::MPIGeneticAlgorithm ga;

		if (!ga.runMPI(factory, 128))
		{
			std::cerr << ga.getErrorString() << std::endl;
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		std::list<mogal::Genome *> bestGenomes;
		std::list<mogal::Genome *>::const_iterator it;

		ga.getBestGenomes(bestGenomes);

		std::cout << "Best genomes: " << std::endl;
		
		for (it = bestGenomes.begin() ; it != bestGenomes.end() ; it++)
		{
			const mogal::Genome *pGenome = *it;
			
			std::cout << "  " << pGenome->getFitnessDescription() << std::endl;
		}
	}
	else
	{
		DistGAFactory factory;

		factory.init(&factoryParams);

		mogal::MPIGeneticAlgorithm ga;

		if (!ga.runMPIHelper(factory))
		{
			std::cerr << ga.getErrorString() << std::endl;
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
	}

	MPI_Finalize();

	return 0;
}



