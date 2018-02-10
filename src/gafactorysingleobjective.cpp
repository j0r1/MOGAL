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

#include "gafactorysingleobjective.h"
#include "genome.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <list>

namespace mogal
{

void GAFactorySingleObjective::breed(const std::vector<GenomeWrapper> &population, std::vector<GenomeWrapper> &newPopulation)
{
	int index = 0;
	GeneticAlgorithmParams gaParams = getCurrentAlgorithmParameters();

	if (gaParams.useElitism())
	{
		newPopulation[index] = GenomeWrapper(population[0].getGenome()->clone(), 0, -1, index);
		index++;
	}

	if (gaParams.alwaysIncludeBestGenome())
	{
		newPopulation[index] = GenomeWrapper(population[0].getGenome()->clone(), 0, -1, index);
		index++;
	}

	commonBreed(index, population, newPopulation);
}

void GAFactorySingleObjective::introduceMutations(std::vector<GenomeWrapper> &newPopulation)
{
	size_t index = 0;
	size_t populationSize = newPopulation.size();
	GeneticAlgorithmParams gaParams = getCurrentAlgorithmParameters();
	
	if (gaParams.useElitism())
		index++;

	for (size_t i = index ; i < populationSize ; i++)
		newPopulation[i].getGenome()->mutate();
}

void GAFactorySingleObjective::updateBestGenomes(std::vector<GenomeWrapper> &population)
{
	std::list<Genome *> genomeList;
	std::list<Genome *> newBest;
	GeneticAlgorithm *pCurrentAlgorithm = getCurrentAlgorithm();

	pCurrentAlgorithm->getBestGenomes(genomeList);
	
	if (genomeList.empty())
		newBest.push_back(population[0].getGenome());
	else
	{
		if (population[0].getGenome()->isFitterThan(*(genomeList.begin())))
			newBest.push_back(population[0].getGenome());
	}
	
	if (!newBest.empty())
		pCurrentAlgorithm->setBestGenomes(newBest);
}

Genome *GAFactorySingleObjective::selectPreferredGenome(const std::list<Genome *> &bestGenomes) const
{
	// Return first one by default.
	// This is what should be done for single-objective genetic algorithms
	if (bestGenomes.empty())
		return 0;
	return *(bestGenomes.begin());
}

} // end namespace

