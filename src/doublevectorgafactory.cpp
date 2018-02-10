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

#include "doublevectorgafactory.h"
#include "doublevectorgafactoryparams.h"
#include "doublevectorgenome.h"
#include "randomnumbergenerator.h"
#include "geneticalgorithm.h"
#include <serut/dummyserializer.h>
#include <serut/memoryserializer.h>

#include <iostream>

namespace mogal
{

DoubleVectorGAFactory::DoubleVectorGAFactory()
{
	m_init = false;
}

DoubleVectorGAFactory::~DoubleVectorGAFactory()
{
	if (m_init)
		delete m_pFactoryParams;
}

bool DoubleVectorGAFactory::init(const mogal::GAFactoryParams *pParams)
{
	if (m_init)
	{
		setErrorString("Already initialized");
		return false;
	}

	if (pParams == 0)
	{
		setErrorString("No parameter specified");
		return false;
	}

	// Create a copy of the parameters

	const DoubleVectorGAFactoryParams *pFactoryParams = (const DoubleVectorGAFactoryParams *)pParams;
	m_pFactoryParams = (DoubleVectorGAFactoryParams *)createParamsInstance();

	serut::DummySerializer dumSer;
	std::vector<uint8_t> copyBuffer;
	
	pParams->write(dumSer);
	copyBuffer.resize(dumSer.getBytesWritten());

	serut::MemorySerializer memSer(&(copyBuffer[0]), copyBuffer.size(), &(copyBuffer[0]), copyBuffer.size());

	pParams->write(memSer);
	m_pFactoryParams->read(memSer);

	// Count the number of parameters in a genome

	m_numGenomeParameters = m_pFactoryParams->getNumberOfParameters();

	if (!subInit(m_pFactoryParams)) // should call setErrorString itself if something goes wrong
	{
		delete m_pFactoryParams;
		return false;
	}

	m_numFitnessValues = getNumberOfFitnessComponents(); // Has to be done after subInit since the number of components can be in the parameters
	m_init = true;
	return true;
}

const mogal::GAFactoryParams *DoubleVectorGAFactory::getCurrentParameters() const
{
	return m_pFactoryParams;
}

/*
mogal::Genome *DoubleVectorGAFactory::createNewGenome() const
{
	return new DoubleVectorGenome((DoubleVectorGAFactory *)this);
}
*/

size_t DoubleVectorGAFactory::getMaximalGenomeSize() const
{
	// genome parameters, m_calculatedFitness flag, fitness values (need these in case the m_calculatedFitness flag was set)
	return m_numGenomeParameters*sizeof(double) + m_numFitnessValues*sizeof(double) + sizeof(int32_t);
}

size_t DoubleVectorGAFactory::getMaximalFitnessSize() const
{
	return m_numFitnessValues*sizeof(double) + sizeof(int32_t); // fitness values and m_calculatedFitness flag
}

bool DoubleVectorGAFactory::writeGenome(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	const DoubleVectorGenome *pGen = (const DoubleVectorGenome *)pGenome;
	int32_t gotFitness = (pGen->hasCalculatedFitness())?1:0;

	if (! (si.writeDoubles(pGen->getParameters(), pGen->getNumberOfParameters()) &&
	       si.writeDoubles(pGen->getFitnessValues(), pGen->getNumberOfFitnessValues()) &&
	       si.writeInt32(gotFitness) ) )
	{
		setErrorString("Couldn't write genome parameters: " + si.getErrorString());
		return false;
	}

	return true;
}

bool DoubleVectorGAFactory::writeGenomeFitness(serut::SerializationInterface &si, const mogal::Genome *pGenome) const
{
	const DoubleVectorGenome *pGen = (const DoubleVectorGenome *)pGenome;
	int32_t gotFitness = (pGen->hasCalculatedFitness())?1:0;

//	std::cout << "Wrote fitness: " << pGen->getFitnessValue() << std::endl;

	if (! (si.writeDoubles(pGen->getFitnessValues(), pGen->getNumberOfFitnessValues()) &&
	       si.writeInt32(gotFitness) ) )
	{
		setErrorString("Couldn't write genome fitness: " + si.getErrorString());
		return false;
	}
	return true;
}

bool DoubleVectorGAFactory::writeCommonGenerationInfo(serut::SerializationInterface &si) const
{
	// Nothing to do
	return true;
}

bool DoubleVectorGAFactory::readGenome(serut::SerializationInterface &si, mogal::Genome **pGenome) const
{
	std::vector<double> params(m_numGenomeParameters);
	std::vector<double> fitness(m_numFitnessValues);
	int32_t gotFitness;

	if (! (si.readDoubles(params) && 
	       si.readDoubles(fitness) &&
	       si.readInt32(&gotFitness) ) )
	{
		setErrorString("Couldn't read genome data: " + si.getErrorString());
		return false;
	}

	DoubleVectorGenome *pGen = (DoubleVectorGenome *)createNewGenome();

	bool gotFitnessFlag = (gotFitness == 0)?false:true;

	pGen->setParameters(&(params[0]));
	pGen->setFitnessValues(&(fitness[0]));
	pGen->setCalculatedFitness(gotFitnessFlag);

	*pGenome = pGen;
	return true;
}

bool DoubleVectorGAFactory::readGenomeFitness(serut::SerializationInterface &si, mogal::Genome *pGenome) const
{
	std::vector<double> fitness(m_numFitnessValues);
	int32_t gotFitness;

	if (! (si.readDoubles(fitness) && si.readInt32(&gotFitness) ) )
	{
		setErrorString("Couldn't read genome fitness and IV data: " + si.getErrorString());
		return false;
	}

	bool gotFitnessFlag = (gotFitness == 0)?false:true;

	DoubleVectorGenome *pGen = (DoubleVectorGenome *)pGenome;

	pGen->setFitnessValues(&(fitness[0]));
	pGen->setCalculatedFitness(gotFitnessFlag);

	return true;
}

bool DoubleVectorGAFactory::readCommonGenerationInfo(serut::SerializationInterface &si)
{
	// Nothing to do
	return true;
}

// TODO: improve this!!!
void DoubleVectorGAFactory::onGeneticAlgorithmStep(int generation, bool *generationInfoChanged, bool *pStopAlgorithm)
{
	// TODO: Adjust the stopping criterion
	if (generation >= 100)
		*pStopAlgorithm = true;

	GeneticAlgorithm *pGA = getCurrentAlgorithm();
	std::vector<Genome *> bestGenomes;

	//std::cout << "Generation: " << generation << std::endl;

	pGA->getBestGenomes(bestGenomes);
	if (bestGenomes.size() == 0)
	{
	//	std::cout << "Currently no best genome" << std::endl;
	}
	else
	{
		DoubleVectorGenome *pGenome = (DoubleVectorGenome *)bestGenomes[0];

		// std::cout << "Best genome: " << pGenome->getFitnessDescription() << std::endl;
		//pGenome->print();
	}
}

void DoubleVectorGAFactory::onGeneticAlgorithmStart()
{
}

void DoubleVectorGAFactory::onGeneticAlgorithmStop()
{
}

void DoubleVectorGAFactory::onSortedPopulation(const std::vector<mogal::GenomeWrapper> &population)
{
}

} // end namespace

