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
 * \file gafactorysingleobjective.h
 */

#ifndef MOGAL_GAFACTORYSINGLEOBJECTIVE_H

#define MOGAL_GAFACTORYSINGLEOBJECTIVE_H

#include "gafactorydefaults.h"
#include <algorithm>

namespace mogal
{

/** Contains implementations for a normal, single-objective genetic algorithm. 
 *  Contains implementations for a normal, single-objective genetic algorithm.
 *  The following functions are implemented:
 *   - GAFactory::getNumberOfFitnessComponents
 *   - GAFactory::sort
 *   - GAFactory::updateBestGenomes
 *   - GAFactory::breed
 *   - GAFactory::introduceMutations
 *   - GAFactory::selectPreferredGenome
 */
class GAFactorySingleObjective : public GAFactoryDefaults
{
protected:
	GAFactorySingleObjective()									{ }
public:
	~GAFactorySingleObjective()									{ }

	size_t getNumberOfFitnessComponents() const							{ return 1; }
	void introduceMutations(std::vector<GenomeWrapper> &newPopulation);
	void breed(const std::vector<GenomeWrapper> &population, std::vector<GenomeWrapper> &newPopulation);
	void sort(std::vector<GenomeWrapper> &population)						{ std::sort(population.begin(), population.end()); }
	void updateBestGenomes(std::vector<GenomeWrapper> &population);
	Genome *selectPreferredGenome(const std::list<Genome *> &bestGenomes) const;
};

} // end namespace

#endif // MOGAL_GAFACTORYSINGLEOBJECTIVE_H

