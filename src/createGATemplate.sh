#!/bin/bash

echo "Enter genome class name: "
read GENOMENAME
echo "Enter factory class name: "
read FACTORYNAME
echo "Multi-objective (yes/no)"
read MULTIOBJECTIVE
echo "Add code for a module (yes/no)"
read MODULECODE
echo "Enter output file name:"
read OFILE

PARENT="mogal::GAFactorySingleObjective"

if [ "$MULTIOBJECTIVE" = "yes" ] || [ "$MULTIOBJECTIVE" = "y" ] ; then
	PARENT="mogal::GAFactoryMultiObjective"
	cat >$OFILE << EOF
#include <mogal/gafactorymultiobjective.h>
EOF
else
	cat >$OFILE << EOF
#include <mogal/gafactorysingleobjective.h>
EOF
fi

cat >>$OFILE << EOF
#include <mogal/genome.h>
#include <serut/serializationinterface.h>

class $GENOMENAME : public mogal::Genome
{
public:
	${GENOMENAME}();
	~${GENOMENAME}();

	void calculateFitness();
EOF

if [ "$MULTIOBJECTIVE" = "yes" ] || [ "$MULTIOBJECTIVE" = "y" ] ; then
	cat >>$OFILE << EOF
	void setActiveFitnessComponent(int i);
EOF
fi

cat >>$OFILE << EOF
	bool isFitterThan(const mogal::Genome *pGenome) const;
	Genome *reproduce(const mogal::Genome *pGenome) const;
	mogal::Genome *clone() const;
	void mutate();
	
	std::string getFitnessDescription() const;
};

class ${FACTORYNAME}Params : public mogal::GAFactoryParams
{
public:
	${FACTORYNAME}Params();
	~${FACTORYNAME}Params();
	bool write(serut::SerializationInterface &si) const;
	bool read(serut::SerializationInterface &si);
};

class $FACTORYNAME : public $PARENT
{
public:
	${FACTORYNAME}();
	~${FACTORYNAME}();

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
EOF

if [ "$MULTIOBJECTIVE" = "yes" ] || [ "$MULTIOBJECTIVE" = "y" ] ; then
	cat >>$OFILE << EOF

	size_t getNumberOfFitnessComponents() const;
	mogal::Genome *selectPreferredGenome(const std::list<mogal::Genome *> &bestGenomes) const;
EOF
fi

cat >>$OFILE << EOF
};

// Genome implementation

${GENOMENAME}::${GENOMENAME}()
{
	// TODO: Initialize object
}

${GENOMENAME}::~${GENOMENAME}()
{
	// TODO: Clean-up object
}

void ${GENOMENAME}::calculateFitness()
{
	// TODO: calculate the object's fitness
}

EOF

if [ "$MULTIOBJECTIVE" = "yes" ] || [ "$MULTIOBJECTIVE" = "y" ] ; then
	cat >>$OFILE << EOF
void ${GENOMENAME}::setActiveFitnessComponent(int i)
{
	// TODO: Select active fitness component
	//       This is necessary for the multi-objective algorithm
}

EOF
fi

cat >>$OFILE << EOF
bool ${GENOMENAME}::isFitterThan(const mogal::Genome *pGenome) const
{
	// TODO: Evaluate the previously calculated fitness measures for
	//       both genomes. Return true if this genome is strictly more
	//       fit than pGenome and false otherwise.
}

mogal::Genome *${GENOMENAME}::reproduce(const mogal::Genome *pGenome) const
{
	// TODO: Create a new genome, based on the current genome and 
	//       pGenome
}

mogal::Genome *${GENOMENAME}::clone() const
{
	// TODO: Create a copy of the current genome and return it. Note that
	//       this function should also copy fitness info etc, since it is 
	//       also used to store the best genomes of the current population
}

void ${GENOMENAME}::mutate()
{ 
	// TODO: Introduce mutations in the current genome
}

std::string ${GENOMENAME}::getFitnessDescription() const
{
	return std::string("TODO: store a description of the fitness of the current genome");
}

// Factory parameters implementation

${FACTORYNAME}Params::${FACTORYNAME}Params()
{
	// TODO: Initialize factory parameters instance
}

${FACTORYNAME}Params::~${FACTORYNAME}Params()
{
	// TODO: Clean-up factory parameters instance
}

bool ${FACTORYNAME}Params::write(serut::SerializationInterface &si) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool ${FACTORYNAME}Params::read(serut::SerializationInterface &si)
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

// Factory implementation

${FACTORYNAME}::${FACTORYNAME}()
{
	// TODO: Factory construction
}

${FACTORYNAME}::~${FACTORYNAME}()
{
	// TODO: Factory destruction
}

mogal::GAFactoryParams *${FACTORYNAME}::createParamsInstance() const
{
	return new ${FACTORYNAME}Params();
}

bool ${FACTORYNAME}::init(const mogal::GAFactoryParams *pParams)
{
	const ${FACTORYNAME}Params *pFactoryParams = (const ${FACTORYNAME}Params *)pParams;

	// TODO: Initialize the factory, based on the settings in pFactoryParams;

	setErrorString("Initializating still needs to be implemented");
	return false;
}

const mogal::GAFactoryParams *${FACTORYNAME}::getCurrentParameters() const
{
	// TODO: Return a pointer to the current parameter settings
	setErrorString("Not implemented yet");
	return 0;
}


mogal::Genome *${FACTORYNAME}::createNewGenome() const
{
	// TODO: Create a new ${GENOMENAME} instance (randomly initialized) and return it
}

size_t ${FACTORYNAME}::getMaximalGenomeSize() const
{
	// This function is only needed if you'll be using distributed calculation.
	// In that case, it should return the maximum number of bytes a genome will
	// need for serialization.
	return 0;
}

size_t ${FACTORYNAME}::getMaximalFitnessSize() const
{
	// This function is only needed if you'll be using distributed calculation.
	// In that case, it should return the maximum number of bytes the fitness 
	// measure(s) need for serialization.
	return 0;
}

bool ${FACTORYNAME}::writeGenome(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool ${FACTORYNAME}::writeGenomeFitness(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool ${FACTORYNAME}::writeCommonGenerationInfo(serut::SerializationInterface &si) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool ${FACTORYNAME}::readGenome(serut::SerializationInterface &si, mogal::Genome **pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool ${FACTORYNAME}::readGenomeFitness(serut::SerializationInterface &si, mogal::Genome *pGenome) const
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

bool ${FACTORYNAME}::readCommonGenerationInfo(serut::SerializationInterface &si)
{
	// This is only needed for distributed calculation
	setErrorString("Serialization is not implemented yet");
	return false;
}

void ${FACTORYNAME}::onGeneticAlgorithmStep(int generation, bool *generationInfoChanged, bool *pStopAlgorithm)
{ 
	// TODO: Adjust the stopping criterion
	if (generation >= 100)
		*pStopAlgorithm = true;
}

void ${FACTORYNAME}::onGeneticAlgorithmStart()
{
	// This gets called when the genetic algorithm is started. No 
	// implementation is necessary for the algorithm to work.
}

void ${FACTORYNAME}::onGeneticAlgorithmStop()
{
	// This gets called when the genetic algorithm is stopped. No 
	// implementation is necessary for the algorithm to work.
}

void ${FACTORYNAME}::onSortedPopulation(const std::vector<mogal::GenomeWrapper> &population)
{
	// This gets called each time the population is sorted. No
	// implementation is necessary for the algorithm to work.
}

EOF

if [ "$MULTIOBJECTIVE" = "yes" ] || [ "$MULTIOBJECTIVE" = "y" ] ; then
	cat >>$OFILE << EOF

size_t ${FACTORYNAME}::getNumberOfFitnessComponents() const
{
	// TODO: return the number of fitness measures the multi-objective
	//       algorithm will use
}

// Select preferred genome
mogal::Genome *${FACTORYNAME}::selectPreferredGenome(const std::list<mogal::Genome *> &bestGenomes) const
{
	// TODO: from the genomes in bestGenomes, pick a single one.
}

EOF
fi

if [ "$MODULECODE" = "yes" ] || [ "$MODULECODE" = "y" ] ; then
	cat >> $OFILE<< EOF

extern "C"
{
	mogal::GAFactory *CreateFactoryInstance()
	{
		return new ${FACTORYNAME}();
	}
}

EOF
fi
