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

#include "gafactorydefaults.h"
#include "genome.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <list>
#include <algorithm>

namespace mogal
{

GAFactoryDefaults::GAFactoryDefaults()
{
	uint32_t x;

	x = (uint32_t)getpid();
	x += (uint32_t)time(0);
	x -= (uint32_t)clock();
	x ^= (uint32_t)(this);

	srand48_r(x, &m_drandBuffer);
}

void GAFactoryDefaults::commonBreed(int startOffset, const std::vector<GenomeWrapper> &population, std::vector<GenomeWrapper> &newPopulation)
{
	GeneticAlgorithmParams gaParams = getCurrentAlgorithmParameters();
	int populationSize = population.size();

	for (int i = startOffset ; i < populationSize ; i++)
	{
		double x;
		drand48_r(&m_drandBuffer, &x);

		if (x < gaParams.getCrossOverRate())
		{
			int count = 0;
			bool ok;
			
			int index1;
			int index2;

			do 
			{
				ok = false;
				
				index1 = pickGenome(gaParams.getBeta(), populationSize);
				index2 = pickGenome(gaParams.getBeta(), populationSize);

				// prevent inbreeding

				int a1 = population[index1].getParent1();
				int a2 = population[index1].getParent2();
				int b1 = population[index2].getParent1();
				int b2 = population[index2].getParent2();

				if (a1 < 0 || b1 < 0) // one of them is a brand new genome
					ok = true;
				else
				{
					if (!(a1 == b1 || a1 == b2 || b1 == a2 || ((a2 >= 0) && a2 == b2)))
						ok = true;
				}
				count++;
			} while (count < 10 && !ok);
				
			newPopulation[i] = GenomeWrapper(population[index1].getGenome()->reproduce(population[index2].getGenome()), index1, index2, i);
		}
		else
		{
			int index1 = pickGenome(gaParams.getBeta(), populationSize);
			
			newPopulation[i] = GenomeWrapper(population[index1].getGenome()->clone(), index1, -1, i);
		}
	}
}

} // end namespace

