#include "distparams.h"
#include <mogal/gafactorysingleobjective.h>
#include <mogal/genome.h>
#include <mogal/geneticalgorithm.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

class DistGAFactory;

class DistGenome : public mogal::Genome
{
public:
	DistGenome(double width, DistGAFactory *pFactory);
	DistGenome(double x, double y, DistGAFactory *pFactory);
	~DistGenome();

	void calculateFitness();
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

class DistGAFactory : public mogal::GAFactorySingleObjective
{
public:
	DistGAFactory();
	~DistGAFactory();

	mogal::GAFactoryParams *createParamsInstance() const;

	bool init(const mogal::GAFactoryParams *pParams);
	const mogal::GAFactoryParams *getCurrentParameters() const;

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

void DistGenome::calculateFitness()
{
	double cx = m_pFactory->getFunctionCenterX();
	double cy = m_pFactory->getFunctionCenterY();

	m_fitness = (m_x-cx)*(m_x-cx) + (m_y-cy)*(m_y-cy);
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

extern "C"
{
	mogal::GAFactory *CreateFactoryInstance()
	{
		return new DistGAFactory();
	}
}
