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

#ifndef MOGAL_DOUBLEVECTORGAFACTORY_H

#define MOGAL_DOUBLEVECTORGAFACTORY_H

#include "mogalconfig.h"
#include "gafactorydefaults.h"
#include "doublevectorgafactoryparams.h"

namespace mogal
{

class RandomNumberGenerator;

class MOGAL_IMPORTEXPORT DoubleVectorGAFactory : public GAFactoryDefaults
{
public:
	DoubleVectorGAFactory();
	~DoubleVectorGAFactory();

	// Can only be implemented in a derived class, the class returned should be derived from DoubleVectorGAFactoryParams
	//mogal::GAFactoryParams *createParamsInstance() const;

	bool init(const GAFactoryParams *pParams);
	const GAFactoryParams *getCurrentParameters() const;

	size_t getNumberOfParameters() const								{ return (size_t)m_numGenomeParameters; }

	// Can only be implemented in a derived class, the class returned should be derived from DoubleVectorGenome
	//Genome *createNewGenome() const;
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
protected:
	virtual bool subInit(const DoubleVectorGAFactoryParams *pParams)					{ return true; }
private:
	DoubleVectorGAFactoryParams *m_pFactoryParams;
	int m_numGenomeParameters;
	int m_numFitnessValues;
	bool m_init;
};

} // end namespace

#endif // MOGAL_DOUBLEVECTORGAFACTORY_H

