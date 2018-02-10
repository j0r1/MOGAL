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

/**
 * \file gafactorymultiobjective.h
 */

#ifndef MOGAL_GAFACTORYMULTIOBJECTIVE_H

#define MOGAL_GAFACTORYMULTIOBJECTIVE_H

#include "gafactorydefaults.h"

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
class GAFactoryMultiObjective : public GAFactoryDefaults
{
protected:
	/** Constructs a factory for a multi-objective genetic algorithm with \c numComponents fitness measures. */
	GAFactoryMultiObjective(size_t numComponents = 2)						{ m_numComponents = numComponents; }
public:
	~GAFactoryMultiObjective()									{ }

	/** Sets the number of fitness components to \c n */
	void setNumberOfFitnessComponents(size_t n)							{ m_numComponents = n; }

	size_t getNumberOfFitnessComponents() const							{ return m_numComponents; }
	
	void introduceMutations(std::vector<GenomeWrapper> &newpopulation);
	void breed(const std::vector<GenomeWrapper> &population, std::vector<GenomeWrapper> &newPopulation);
	void sort(std::vector<GenomeWrapper> &population);
	void updateBestGenomes(std::vector<GenomeWrapper> &population);
private:
	size_t m_numComponents;
	int m_offset;
	std::list<Genome *> m_populationNDS;
	std::vector<std::vector<GenomeWrapper> > m_orderedSets;
};

} // end namespace

#endif // MOGAL_GAFACTORYMULTIOBJECTIVE_H

