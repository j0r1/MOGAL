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

/**
 * \file gafactorymultiobjective.h
 */

#ifndef MOGAL_GAFACTORYMULTIOBJECTIVE_H

#define MOGAL_GAFACTORYMULTIOBJECTIVE_H

#include "mogalconfig.h"
#include "gafactorysingleobjective.h"

//#define MOGAL_DEBUGTIMING

namespace mogal
{

/** Contains functions which can be used in a multi-objective genetic algorithm. 
 *  Contains functions which can be used in a multi-objective genetic algorithm.
 *  The following functions are implemented:
 *   - GAFactory::getNumberOfFitnessComponents
 *   - GAFactory::sort
 *   - GAFactory::updateBestGenomes
 *   - GAFactory::breed
 *   - GAFactory::introduceMutations
 */
class MOGAL_IMPORTEXPORT GAFactoryMultiObjective : public GAFactorySingleObjective
{
protected:
	/** Constructs a factory for a multi-objective genetic algorithm with \c numComponents fitness measures. */
	GAFactoryMultiObjective(size_t numComponents = 2);
public:
	enum SortType { Basic, Optimized, CUDA };

	~GAFactoryMultiObjective();

	/** Sets the number of fitness components to \c n */
	void setNumberOfFitnessComponents(size_t n)							{ m_numComponents = n; }

	size_t getNumberOfFitnessComponents() const							{ return m_numComponents; }
	
	void introduceMutations(std::vector<GenomeWrapper> &newpopulation);
	void breed(const std::vector<GenomeWrapper> &population, std::vector<GenomeWrapper> &newPopulation);
	void sort(std::vector<GenomeWrapper> &population);
	
	void sortOld(std::vector<GenomeWrapper> &population);
	void sortNew(std::vector<GenomeWrapper> &population);
	void sortCuda(std::vector<GenomeWrapper> &population);

	void updateBestGenomes(std::vector<GenomeWrapper> &population);
private:
	void initSort(int populationSize);
	void initSortCUDA(int populationSize);

	size_t m_numComponents;
	int m_offset;
	std::list<Genome *> m_populationNDS;
	std::vector<std::vector<GenomeWrapper> > m_orderedSets;

	// TODO: for debugging
#ifdef MOGAL_DEBUGTIMING
	double m_totalOldTime;
	double m_totalNewTime;
	double m_totalCudaTime;
#endif // MOGAL_DEBUGTIMING
	std::vector<int16_t> m_dominatesList;

	bool m_sortInit;
	bool m_hasCuda;
	SortType m_sortType;

	float *m_pDeviceFitnessArray;
	short *m_pDeviceDominationMap;
	int *m_pDeviceDominatedByCount;
	int *m_pDeviceDominatesCount;

	float *m_pHostFitnessArray;
	short *m_pHostDominationMap;
	int *m_pHostDominatedByCount;
	int *m_pHostDominatesCount;
	int *m_pOffsets;

	int m_blockSize1, m_blockSize2;
};

} // end namespace

#endif // MOGAL_GAFACTORYMULTIOBJECTIVE_H

