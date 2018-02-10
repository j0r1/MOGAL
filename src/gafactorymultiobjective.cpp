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

#include "gafactorymultiobjective.h"
#include "genome.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <list>

// #include <iostream>

namespace mogal
{

void GAFactoryMultiObjective::breed(const std::vector<GenomeWrapper> &population, std::vector<GenomeWrapper> &newPopulation)
{
	int populationSize = population.size();
	int index = 0;
	int eliteCount = (int)(((double)(populationSize)*0.005)+0.5); // TODO: make this percentage configurable
	int nonDominatedSetSize = getCurrentAlgorithm()->getNumberOfBestGenomes();
	
	if (eliteCount < 1)
		eliteCount = 1;
	if (eliteCount > nonDominatedSetSize)
		eliteCount = nonDominatedSetSize;

	GeneticAlgorithmParams gaParams = getCurrentAlgorithmParameters();

	m_offset = 0;

	if ((gaParams.useElitism() || gaParams.alwaysIncludeBestGenome()) && eliteCount > 0)
	{
		std::vector<int> bestGenomeIndices(nonDominatedSetSize);
		std::vector<Genome *> bestGenomes;

		getCurrentAlgorithm()->getBestGenomes(bestGenomes);

		if (gaParams.useElitism())
		{
			for (int i = 0 ; i < nonDominatedSetSize ; i++)
				bestGenomeIndices[i] = i;

			for (int i = 0 ; i < eliteCount ; i++)
			{
				int randomIndex = pickGenome(nonDominatedSetSize-i);
				int idx = bestGenomeIndices[randomIndex];

				bestGenomeIndices[randomIndex] = bestGenomeIndices[nonDominatedSetSize-i-1];
			
				// TODO: this 0 as the parent index is not correct. perhaps a better
				//       system should be thought of?
				newPopulation[index] = GenomeWrapper(bestGenomes[idx]->clone(), 0, -1, index);
				index++;
				m_offset++;
			}
		}

		if (gaParams.alwaysIncludeBestGenome())
		{
			for (int i = 0 ; i < nonDominatedSetSize ; i++)
				bestGenomeIndices[i] = i;

			for (int i = 0 ; i < eliteCount ; i++)
			{
				int randomIndex = pickGenome(nonDominatedSetSize-i);
				int idx = bestGenomeIndices[randomIndex];

				bestGenomeIndices[randomIndex] = bestGenomeIndices[nonDominatedSetSize-i-1];
			
				// TODO: this 0 as the parent index is not correct. perhaps a better
				//       system should be thought of?
				newPopulation[index] = GenomeWrapper(bestGenomes[idx]->clone(), 0, -1, index);
				index++;
			}
		}
	}

	int startOffset = index;
	int numSets = m_orderedSets.size();

	for (int i = startOffset ; i < populationSize ; i++)
	{
		double x = pickDouble();

		if (x < gaParams.getCrossOverRate()) // cross over
		{
			int count = 0;
			bool ok;
			
			int index1, subIndex1;
			int index2, subIndex2;

			do 
			{
				ok = false;
				
				index1 = pickGenome(gaParams.getBeta(), numSets);
				index2 = pickGenome(gaParams.getBeta(), numSets);
				subIndex1 = pickGenome(m_orderedSets[index1].size());
				subIndex2 = pickGenome(m_orderedSets[index2].size());

				// prevent inbreeding

				int a1 = m_orderedSets[index1][subIndex1].getParent1();
				int a2 = m_orderedSets[index1][subIndex1].getParent2();
				int b1 = m_orderedSets[index2][subIndex2].getParent1();
				int b2 = m_orderedSets[index2][subIndex2].getParent2();

				if (a1 < 0 || b1 < 0) // one of them is a brand new genome
					ok = true;
				else
				{
					if (!(a1 == b1 || a1 == b2 || b1 == a2 || ((a2 >= 0) && a2 == b2)))
						ok = true;
				}
				count++;
			} while (count < 10 && !ok);
				
			newPopulation[i] = GenomeWrapper(m_orderedSets[index1][subIndex1].getGenome()->reproduce(m_orderedSets[index2][subIndex2].getGenome()), 
					                 m_orderedSets[index1][subIndex1].getPosition(), m_orderedSets[index2][subIndex2].getPosition(), i);
		}
		else
		{
			int index1 = pickGenome(gaParams.getBeta(), numSets);
			int subIndex1 = pickGenome(m_orderedSets[index1].size());
			
			newPopulation[i] = GenomeWrapper(m_orderedSets[index1][subIndex1].getGenome()->clone(), m_orderedSets[index1][subIndex1].getPosition(), -1, i);
		}
	}
}

void GAFactoryMultiObjective::introduceMutations(std::vector<GenomeWrapper> &newPopulation)
{
	int populationSize = newPopulation.size();
	
	for (int i = m_offset ; i < populationSize ; i++)
		newPopulation[i].getGenome()->mutate();
}

void GAFactoryMultiObjective::sort(std::vector<GenomeWrapper> &population)
{
	std::list<GenomeWrapper> sortedList;
	std::list<GenomeWrapper> nonDominatedSet;
	std::list<GenomeWrapper> populationLeft;
	std::list<GenomeWrapper> newPopulationLeft;
	std::vector<std::vector<GenomeWrapper> > orderedSets;
	int popSize = population.size();
	bool firstLoop = true;

	for (int i = 0 ; i < popSize ; i++)
		populationLeft.push_back(population[i]);

	m_populationNDS.clear();

	while (!populationLeft.empty())
	{
		std::list<GenomeWrapper>::const_iterator it1, it2;

		for (it1 = populationLeft.begin() ; it1 != populationLeft.end() ; it1++)
		{
			Genome *g = (*it1).getGenome(); // see if this one is not dominated by anyone
			bool dominated = false;

			for (it2 = populationLeft.begin() ; !dominated && it2 != populationLeft.end() ; it2++)
			{
				if (it2 == it1)
					continue;

				Genome *g2 = (*it2).getGenome();
				int count1 = 0;
				int count2 = 0;

				for (int i = 0 ; i < m_numComponents ; i++)
				{
					g->setActiveFitnessComponent(i);
					g2->setActiveFitnessComponent(i);
					
					if (g2->isFitterThan(g))
					{
						count1++;
						count2++;
					}
					else
					{
						if (!g->isFitterThan(g2))
							count1++;
					}
				}

				if (count1 == m_numComponents && count2 > 0)
					dominated = true;
			}

			if (dominated)
				newPopulationLeft.push_back(*it1);
			else
				nonDominatedSet.push_back(*it1);
		}

		populationLeft = newPopulationLeft;
		newPopulationLeft.clear();

		std::vector<GenomeWrapper> nonDominatedSet2(nonDominatedSet.size());
		int index = 0;

		while (!nonDominatedSet.empty())
		{
			nonDominatedSet2[index] = *(nonDominatedSet.begin());
			index++;
			
			sortedList.push_back(*(nonDominatedSet.begin()));
			if (firstLoop)
				m_populationNDS.push_back((*(nonDominatedSet.begin())).getGenome());

			nonDominatedSet.pop_front();
		}

		orderedSets.push_back(nonDominatedSet2);
	
		firstLoop = false;
	}

	std::list<GenomeWrapper>::const_iterator it = sortedList.begin();
	int i = 0;
	while (it != sortedList.end())
	{
		population[i] = (*it);
		i++;
		it++;
	}

	m_orderedSets = orderedSets;

	// std::cout << "Number of sets: " << m_orderedSets.size() << std::endl;
}

// TODO: There's probably a more efficient way to merge these
void GAFactoryMultiObjective::updateBestGenomes(std::vector<GenomeWrapper> &population)
{
	std::list<Genome *> populationLeft;
	std::list<Genome *> nonDominatedSet;
	std::list<Genome *> nonDominatedSet2;
	std::list<Genome *>::const_iterator it;

	getCurrentAlgorithm()->getBestGenomes(populationLeft);
	//std::cout << "Fitness components: " << m_numComponents << std::endl;
	//std::cout << "Previous best genomes: ";
	//for (it = populationLeft.begin() ; it != populationLeft.end() ; it++)
	//	std::cout << "  " << (*it)->getFitnessDescription() << std::endl;

	//std::cout << "Current non dominated set: ";
	for (it = m_populationNDS.begin() ; it != m_populationNDS.end() ; it++)
	{
		populationLeft.push_back(*it);
	//	std::cout << "  " << (*it)->getFitnessDescription() << std::endl;
	}
	std::list<Genome *>::const_iterator it1, it2;

	for (it1 = populationLeft.begin() ; it1 != populationLeft.end() ; it1++)
	{
		Genome *g = (*it1); 
		bool dominated = false;

		for (it2 = populationLeft.begin() ; !dominated && it2 != populationLeft.end() ; it2++)
		{
			if (it2 == it1)
				continue;

			Genome *g2 = (*it2);
			int count1 = 0;
			int count2 = 0;

			for (int i = 0 ; i < m_numComponents ; i++)
			{
				g->setActiveFitnessComponent(i);
				g2->setActiveFitnessComponent(i);
				
				if (g2->isFitterThan(g))
				{
					count1++;
					count2++;
				}
				else
				{
					if (!g->isFitterThan(g2))
						count1++;
				}
			}

			if (count1 == m_numComponents && count2 > 0)
				dominated = true;
		}

		if (!dominated)
			nonDominatedSet.push_back(*it1);
	}

	// remove doubles in the non-dominated set
	// TODO: it's probably not a good idea to always do this: the doubles should
	//       really be checked based on the actual genome information
	//  -> It's probably safe to only check the actual genomes if the fitness
	//     values are equal
	// REMARK: Perhaps this is not a good idea: may cause an excessively large
	//         non dominated set

	for (it1 = nonDominatedSet.begin() ; it1 != nonDominatedSet.end() ; it1++)
	{
		Genome *g = (*it1); 
		bool gotDouble = false;

		for (it2 = nonDominatedSet2.begin() ; !gotDouble && it2 != nonDominatedSet2.end() ; it2++)
		{
			Genome *g2 = (*it2);
			int count = 0;

			for (int i = 0 ; i < m_numComponents ; i++)
			{
				g->setActiveFitnessComponent(i);
				g2->setActiveFitnessComponent(i);
				
				if ((!g2->isFitterThan(g)) && (!g->isFitterThan(g2)))
					count++;
			}

			if (count == m_numComponents)
				gotDouble = true;
		}

		if (!gotDouble)
			nonDominatedSet2.push_back(*it1);
	}

	getCurrentAlgorithm()->setBestGenomes(nonDominatedSet2);

	//std::cout << "New best genomes: ";
	//for (it = nonDominatedSet2.begin() ; it != nonDominatedSet2.end() ; it++)
	//	std::cout << "  " << (*it)->getFitnessDescription() << std::endl;

}


} // end namespace

