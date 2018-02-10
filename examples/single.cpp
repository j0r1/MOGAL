#include <mogal/gafactorysingleobjective.h>
#include <mogal/genome.h>
#include <mogal/geneticalgorithm.h>
#include <serut/serializationinterface.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

class MinGenome : public mogal::Genome
{
public:
	MinGenome();
	MinGenome(double x, double y);
	~MinGenome();

	void calculateFitness();
	bool isFitterThan(const mogal::Genome *pGenome) const;
	Genome *reproduce(const mogal::Genome *pGenome) const;
	mogal::Genome *clone() const;
	void mutate();
	
	std::string getFitnessDescription() const;
private:
	double m_x, m_y;
	double m_fitness;
};

class MinGAFactoryParams : public mogal::GAFactoryParams
{
public:
	MinGAFactoryParams();
	~MinGAFactoryParams();
	bool write(serut::SerializationInterface &si) const;
	bool read(serut::SerializationInterface &si);
};

class MinGAFactory : public mogal::GAFactorySingleObjective
{
public:
	MinGAFactory();
	~MinGAFactory();

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
private:
	MinGAFactoryParams m_factoryParams;
};

// Genome implementation

MinGenome::MinGenome()
{
	m_x = (20.0*((double)rand()/(RAND_MAX + 1.0)))-10.0;
	m_y = (20.0*((double)rand()/(RAND_MAX + 1.0)))-10.0;
}

MinGenome::MinGenome(double x, double y)
{
	m_x = x;
	m_y = y;
}

MinGenome::~MinGenome()
{
}

void MinGenome::calculateFitness()
{
	m_fitness = m_x*m_x + m_y*m_y;
}

bool MinGenome::isFitterThan(const mogal::Genome *pGenome) const
{
	const MinGenome *pMinGenome = (const MinGenome *)pGenome;

	if (m_fitness < pMinGenome->m_fitness)
		return true;
	return false;
}

mogal::Genome *MinGenome::reproduce(const mogal::Genome *pGenome) const
{
	double randomNumber = (1.0*((double)rand()/(RAND_MAX + 1.0)));
	const MinGenome *pMinGenome = (const MinGenome *)pGenome;
	MinGenome *pNewGenome = 0;
	
	if (randomNumber < 0.5)
		pNewGenome = new MinGenome(m_x, pMinGenome->m_y);
	else
		pNewGenome = new MinGenome(pMinGenome->m_x, m_y);

	return pNewGenome;
}

mogal::Genome *MinGenome::clone() const
{
	MinGenome *pNewGenome = new MinGenome(m_x, m_y);

	pNewGenome->m_fitness = m_fitness;
	return pNewGenome;
}

void MinGenome::mutate()
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

std::string MinGenome::getFitnessDescription() const
{
	char str[256];

	sprintf(str, "%g (%g, %g)", m_fitness, m_x, m_y);

	return std::string(str);
}

// Factory parameters implementation

MinGAFactoryParams::MinGAFactoryParams()
{
}

MinGAFactoryParams::~MinGAFactoryParams()
{
}

bool MinGAFactoryParams::write(serut::SerializationInterface &si) const
{
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool MinGAFactoryParams::read(serut::SerializationInterface &si)
{
	setErrorString("Serialization is not implemented yet");
	return false;
}

// Factory implementation

MinGAFactory::MinGAFactory()
{
}

MinGAFactory::~MinGAFactory()
{
}

mogal::GAFactoryParams *MinGAFactory::createParamsInstance() const
{
	return new MinGAFactoryParams();
}

bool MinGAFactory::init(const mogal::GAFactoryParams *pParams)
{
	// Currently no initialization is needed
	return true;
}

const mogal::GAFactoryParams *MinGAFactory::getCurrentParameters() const
{
	return &m_factoryParams;
}


mogal::Genome *MinGAFactory::createNewGenome() const
{
	return new MinGenome();
}

size_t MinGAFactory::getMaximalGenomeSize() const
{
	// This function is only needed if you'll be using distributed calculation.
	// In that case, it should return the maximum number of bytes a genome will
	// need for serialization.
	return 0;
}

size_t MinGAFactory::getMaximalFitnessSize() const
{
	// This function is only needed if you'll be using distributed calculation.
	// In that case, it should return the maximum number of bytes the fitness 
	// measure(s) need for serialization.
	return 0;
}

bool MinGAFactory::writeGenome(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool MinGAFactory::writeGenomeFitness(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool MinGAFactory::writeCommonGenerationInfo(serut::SerializationInterface &si) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool MinGAFactory::readGenome(serut::SerializationInterface &si, mogal::Genome **pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool MinGAFactory::readGenomeFitness(serut::SerializationInterface &si, mogal::Genome *pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool MinGAFactory::readCommonGenerationInfo(serut::SerializationInterface &si)
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

void MinGAFactory::onGeneticAlgorithmStep(int generation, bool *generationInfoChanged, bool *pStopAlgorithm)
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

void MinGAFactory::onGeneticAlgorithmStart()
{
	// This gets called when the genetic algorithm is started. No 
	// implementation is necessary for the algorithm to work.
}

void MinGAFactory::onGeneticAlgorithmStop()
{
	// This gets called when the genetic algorithm is stopped. No 
	// implementation is necessary for the algorithm to work.
}

void MinGAFactory::onSortedPopulation(const std::vector<mogal::GenomeWrapper> &population)
{
	// This gets called each time the population is sorted. No
	// implementation is necessary for the algorithm to work.
}

int main(void)
{
	mogal::GeneticAlgorithm ga;
	MinGAFactoryParams factoryParams;
	MinGAFactory gaFactory;

	srand(time(0));

	if (!gaFactory.init(&factoryParams))
	{
		std::cerr << gaFactory.getErrorString() << std::endl;
		return -1;
	}

	if (!ga.run(gaFactory, 128))
	{
		std::cerr << ga.getErrorString() << std::endl;
		return -1;
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

	return 0;
}
