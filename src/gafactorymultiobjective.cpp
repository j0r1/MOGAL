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

#include "gafactorymultiobjective.h"
#include "genome.h"
#include <time.h>
//#include <unistd.h>
#include <stdlib.h>
#include <list>
#include <set>

// TODO for debugging
//#include <jrtplib3/rtptimeutilities.h>
#include <iostream>

#ifdef MOGAL_USECUDA
bool allocDeviceMemory(int numGenomes, int numFitnessMeasures, float **pDeviceFitnessArray, short **pDeviceDominationMap,
                       int **pDeviceDominatedByCount, int **pDeviceDominatesCount, short **pHostDominationMap);
void freeDeviceMemory(float *pDeviceFitnessArray, short *pDeviceDominationMap,
                      int *pDeviceDominatedByCount, int *pDeviceDominatesCount, short *pHostDominationMap);
void calcDominationInfo(int numGenomes, int numFitnessMeasures, float *pHostFitnessArray, short *pHostDominationMap,
			int *pHostDominatedByCount, int *pHostDominatesCount,
                        float *pDeviceFitnessArray, short *pDeviceDominationMap, 
			int *pDeviceDominatedByCount, int *pDeviceDominatesCount,
			int blockSize1, int blockSize2, int *pOffsets);
bool checkCuda(int *pBlockSize1, int *pBlockSize2);
#endif // MOGAL_USECUDA

namespace mogal
{

GAFactoryMultiObjective::GAFactoryMultiObjective(size_t numComponents)
{ 
	m_numComponents = numComponents; 
	m_sortInit = false;
	m_hasCuda = false;
	m_sortType = Basic;

#ifdef MOGAL_DEBUGTIMING
	m_totalNewTime = 0;
	m_totalOldTime = 0;
	m_totalCudaTime = 0;
#endif // MOGAL_DEBUGTIMING
}

GAFactoryMultiObjective::~GAFactoryMultiObjective()
{
#ifdef MOGAL_USECUDA
	if (m_hasCuda)
	{
		freeDeviceMemory(m_pDeviceFitnessArray, m_pDeviceDominationMap, m_pDeviceDominatedByCount, 
		                 m_pDeviceDominatesCount, m_pHostDominationMap);

		delete [] m_pHostFitnessArray;
		delete [] m_pHostDominatedByCount;
		delete [] m_pHostDominatesCount;
		delete [] m_pOffsets;
	}
#endif // MOGAL_USECUDA
}

void GAFactoryMultiObjective::breed(const std::vector<GenomeWrapper> &population, std::vector<GenomeWrapper> &newPopulation)
{
	if (m_numComponents == 1) // Fall back to single objective code
	{
		GAFactorySingleObjective::breed(population, newPopulation);
		return;
	}

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
				subIndex1 = 0;
				subIndex2 = 0;
				if (m_orderedSets[index1].size() > 1)
					subIndex1 = pickGenome(m_orderedSets[index1].size());
				if (m_orderedSets[index2].size() > 1)
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
	if (m_numComponents == 1) // Fall back to single objective code
	{
		GAFactorySingleObjective::introduceMutations(newPopulation);
		return;
	}

	int populationSize = newPopulation.size();
	
	for (int i = m_offset ; i < populationSize ; i++)
		newPopulation[i].getGenome()->mutate();
}

void GAFactoryMultiObjective::sort(std::vector<GenomeWrapper> &population)
{
	if (m_numComponents == 1) // Fall back to single objective code
	{
		GAFactorySingleObjective::sort(population);
		return;
	}

	if (!m_sortInit)
	{
		m_sortInit = true;
		initSort(population.size());
	}

#ifdef MOGAL_DEBUGTIMING
	RTPTime t1a = RTPTime::CurrentTime();
	sortOld(population);
	RTPTime t1b = RTPTime::CurrentTime();

	t1b -= t1a;

	m_totalOldTime += t1b.GetDouble();

	for (int i = 0 ; i < m_orderedSets.size() ; i++)
		std::cout << m_orderedSets[i].size() << " ";
	std::cout << "(" << m_totalOldTime << ")" << std::endl;

	RTPTime t2a = RTPTime::CurrentTime();
	sortNew(population);
	RTPTime t2b = RTPTime::CurrentTime();

	t2b -= t2a;

	m_totalNewTime += t2b.GetDouble();

	for (int i = 0 ; i < m_orderedSets.size() ; i++)
		std::cout << m_orderedSets[i].size() << " ";
	std::cout << "(" << m_totalNewTime << ")" <<  std::endl;

	RTPTime t3a = RTPTime::CurrentTime();
	sortCuda(population);
	RTPTime t3b = RTPTime::CurrentTime();

	t3b -= t3a;

	m_totalCudaTime += t3b.GetDouble();

	for (int i = 0 ; i < m_orderedSets.size() ; i++)
		std::cout << m_orderedSets[i].size() << " ";
	std::cout << "(" << m_totalCudaTime << ")" <<  std::endl;

	std::cout << std::endl;
#else
	if (m_sortType == CUDA)
		sortCuda(population);
	else if (m_sortType == Optimized)
		sortNew(population);
	else
		sortOld(population);
#endif // MOGAL_DEBUGTIMING
}

void GAFactoryMultiObjective::sortOld(std::vector<GenomeWrapper> &population)
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

				for (size_t i = 0 ; i < m_numComponents ; i++)
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

void GAFactoryMultiObjective::sortNew(std::vector<GenomeWrapper> &population)
{
//	RTPTime t2a = RTPTime::CurrentTime();

	int popSize = population.size();

	//std::vector<std::set<int> > dominatesGenomes(popSize);

	std::vector<int> dominatedCount(popSize);
	std::vector<int> dominatesCount(popSize);

	for (int i = 0 ; i < popSize ; i++)
	{
		dominatedCount[i] = 0;
		dominatesCount[i] = 0;
	}

	for (int i = 0 ; i < popSize-1 ; i++)
	{
		Genome *pGenome1 = population[i].getGenome();

		for (int j = i+1 ; j < popSize ; j++)
		{
			Genome *pGenome2 = population[j].getGenome();
			// compare genome i and j
			
			size_t betterOrEqualCount1 = 0;
			size_t betterOrEqualCount2 = 0;
			size_t equalCount = 0;

			for (size_t k = 0 ; k < m_numComponents ; k++)
			{
				pGenome1->setActiveFitnessComponent(k);
				pGenome2->setActiveFitnessComponent(k);

				if (pGenome1->isFitterThan(pGenome2))
					betterOrEqualCount1++;
				else if (pGenome2->isFitterThan(pGenome1))
					betterOrEqualCount2++;
				else // equally fit
				{
					equalCount++;
					betterOrEqualCount1++;
					betterOrEqualCount2++;
				}
			}

			if (betterOrEqualCount1 == m_numComponents && equalCount < m_numComponents)
			{
				// i dominates j
				dominatedCount[j]++;
				//dominatesGenomes[i].insert(j);

				int offset = i*popSize + dominatesCount[i];
				m_dominatesList[offset] = j;

				dominatesCount[i]++;
			}
			if (betterOrEqualCount2 == m_numComponents && equalCount < m_numComponents)
			{
				// j dominates i
				dominatedCount[i]++;
				//dominatesGenomes[j].insert(i);
				
				int offset = j*popSize + dominatesCount[j];
				m_dominatesList[offset] = i;

				dominatesCount[j]++;
			}
		}
	}

	/*
	RTPTime t2b = RTPTime::CurrentTime();
	RTPTime t2c = t2b;

	t2b -= t2a;

	std::cout << "Time for part 1 is " << t2b.GetDouble() << std::endl; 
*/
	// We now have all the information we need.
	
	bool done = false;

	m_orderedSets.clear();

	while (!done)
	{
		std::vector<GenomeWrapper> currentNonDominatedSet;
		std::vector<int> selectedGenomes;

		for (int i = 0 ; i < popSize ; i++)
		{
			if (dominatedCount[i] == 0) // this is not dominated by any other genome
			{
				dominatedCount[i] = -1; // make sure we skip it in the future
				currentNonDominatedSet.push_back(population[i]); // store it
				selectedGenomes.push_back(i);
			}
		}
				
		if (currentNonDominatedSet.empty())
			done = true;
		else
		{
			for (size_t k = 0 ; k < selectedGenomes.size() ; k++)
			{
				int i = selectedGenomes[k];

				// check which genomes this one dominates

				/*
				std::set<int>::const_iterator it = dominatesGenomes[i].begin();
				std::set<int>::const_iterator endIt = dominatesGenomes[i].end();
				
				for (   ; it != endIt ; it++)
				{
					int idx = *it;
				*/

				int offset = i*popSize;
				int count = dominatesCount[i];

				for (int j = 0 ; j < count ; j++, offset++)
				{
					int idx = m_dominatesList[offset];

					// genome i dominates genome idx, but we're going to remove i from the set,
					// so we can decrease the count

					dominatedCount[idx]--;

					// TODO: for debugging
					if (dominatedCount[idx] < 0)
					{
						std::cerr << "ERROR: dominated count became negative unexpectedly" << std::endl;
					}
				}
			}

			m_orderedSets.push_back(currentNonDominatedSet);
		}
	}

	m_populationNDS.clear();

	for (size_t i = 0 ; i < m_orderedSets[0].size() ; i++)
		m_populationNDS.push_back(m_orderedSets[0][i].getGenome());

/*	std::cout << "Number of sets: " << m_orderedSets.size() << std::endl;
	for (int i = 0 ; i < m_orderedSets.size() ; i++)
	{
		std::cout << "set " << (i+1) << std::endl;
		for (int j = 0 ; j < m_orderedSets[i].size() ; j++)
			std::cout << "  " << m_orderedSets[i][j].getGenome()->getFitnessDescription() << std::endl;
	}
	std::cout << std::endl;
	*/
/*
	RTPTime t2d = RTPTime::CurrentTime();
	t2d -= t2c;

	std::cout << "Time for part 2 is " << t2d.GetDouble() << std::endl; */
}

void GAFactoryMultiObjective::sortCuda(std::vector<GenomeWrapper> &population)
{
	if (!m_hasCuda)
	{
		std::cout << "No cuda support" << std::endl;
		return;
	}
#ifdef MOGAL_USECUDA
	// std::cout << "Preparing fitness array" << std::endl;

	float *pFitness = m_pHostFitnessArray;
	int numGenomes = population.size();

	for (int i = 0 ; i < numGenomes ; i++, pFitness += m_numComponents)
		population[i].getGenome()->copyFitnessValuesTo(pFitness, m_numComponents);

	calcDominationInfo(numGenomes, m_numComponents, m_pHostFitnessArray, m_pHostDominationMap, m_pHostDominatedByCount, 
			   m_pHostDominatesCount, m_pDeviceFitnessArray, m_pDeviceDominationMap, m_pDeviceDominatedByCount, 
			   m_pDeviceDominatesCount, m_blockSize1, m_blockSize2, m_pOffsets);

	bool done = false;

	m_orderedSets.clear();

	while (!done)
	{
		std::vector<GenomeWrapper> currentNonDominatedSet;
		std::vector<int> selectedGenomes;

		for (int i = 0 ; i < numGenomes ; i++)
		{
			if (m_pHostDominatedByCount[i] == 0) // this is not dominated by any other genome
			{
				m_pHostDominatedByCount[i] = -1; // make sure we skip it in the future
				currentNonDominatedSet.push_back(population[i]); // store it
				selectedGenomes.push_back(i);
			}
		}
				
		if (currentNonDominatedSet.empty())
			done = true;
		else
		{
			for (int k = 0 ; k < selectedGenomes.size() ; k++)
			{
				int i = selectedGenomes[k];
				int count = m_pHostDominatesCount[i];
				int offset = m_pOffsets[i];

				// check which genomes this one dominates

				for (int j = 0 ; j < count ; j++, offset++)
				{
					int idx = m_pHostDominationMap[offset];

					m_pHostDominatedByCount[idx]--;

					// TODO: for debugging
					if (m_pHostDominatedByCount[idx] < 0)
					{
						std::cerr << "ERROR: dominated count became negative unexpectedly" << std::endl;
					}
				}
			}

			m_orderedSets.push_back(currentNonDominatedSet);
		}
	}

	m_populationNDS.clear();

	for (int i = 0 ; i < m_orderedSets[0].size() ; i++)
		m_populationNDS.push_back(m_orderedSets[0][i].getGenome());
#endif // MOGAL_USECUDA
}

// TODO: There's probably a more efficient way to merge these
//       Probably not all that useful to optimize this. Doesn't seem to
//       take up much processing time
void GAFactoryMultiObjective::updateBestGenomes(std::vector<GenomeWrapper> &population)
{
	if (m_numComponents == 1) // Fall back to single objective code
	{
		GAFactorySingleObjective::updateBestGenomes(population);
		return;
	}

	//RTPTime t2a = RTPTime::CurrentTime();

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

			for (size_t i = 0 ; i < m_numComponents ; i++)
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

	
	/*
	RTPTime t2b = RTPTime::CurrentTime();
	RTPTime t2c = t2b;

	t2b -= t2a;

	std::cout << "Time for part 3 is " << t2b.GetDouble() << std::endl; 
*/

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

			for (size_t i = 0 ; i < m_numComponents ; i++)
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

/*	RTPTime t2d = RTPTime::CurrentTime();
	t2d -= t2c;

	std::cout << "Time for part 4 is " << t2d.GetDouble() << std::endl;
*/
}

#ifdef MOGAL_DEBUGTIMING
void GAFactoryMultiObjective::initSort(int populationSize)
{
#ifdef MOGAL_USECUDA
	initSortCUDA(populationSize);
#endif // MOGAL_USECUDA
	
	m_dominatesList.resize(populationSize*populationSize);
}
#else
void GAFactoryMultiObjective::initSort(int populationSize)
{
#ifdef MOGAL_USECUDA
	if (populationSize < 65536)
	{
		initSortCUDA(populationSize);
		if (m_hasCuda)
		{
			m_sortType = CUDA;
			std::cout << "Using CUDA sort" << std::endl;
			return;
		}
	}
	
#endif // MOGAL_USECUDA
	
	if (populationSize < 65536)
	{
		m_dominatesList.resize(populationSize*populationSize);
		m_sortType = Optimized;
		std::cout << "Using optimized sort" << std::endl;
		return;
	}

	// No init necessary for old algorithm
	
	m_sortType = Basic;
	std::cout <<  "Using basic sort" << std::endl;
}
#endif // MOGAL_DEBUGTIMING

void GAFactoryMultiObjective::initSortCUDA(int populationSize)
{
#ifdef MOGAL_USECUDA
	if (!hasFloatingPointFitnessValues())
	{
		std::cout << "Genomes do not have floating point fitness values, not using CUDA" << std::endl;
		m_hasCuda = false;
		return;
	}

	if (!checkCuda(&m_blockSize1, &m_blockSize2))
	{
		std::cout << "Unable to init cuda" << std::endl;
		m_hasCuda = false;
		return;
	}

	if (!allocDeviceMemory(populationSize, m_numComponents, &m_pDeviceFitnessArray, &m_pDeviceDominationMap,
	                       &m_pDeviceDominatedByCount, &m_pDeviceDominatesCount, &m_pHostDominationMap))
	{
		std::cout << "Unable to allocate memory" << std::endl;
		m_hasCuda = false;
		return;
	}

	m_pHostFitnessArray = new float[m_numComponents*populationSize];
	m_pHostDominatedByCount = new int[populationSize];
	m_pHostDominatesCount = new int[populationSize];
	m_pOffsets = new int[populationSize];

	m_hasCuda = true;
#endif // MOGAL_USECUDA
}

} // end namespace

