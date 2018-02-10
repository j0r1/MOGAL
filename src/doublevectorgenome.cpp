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

#include "doublevectorgenome.h"
#include "doublevectorgafactory.h"
#include "doublevectorgafactoryparams.h"
#include <stdio.h>
#include <iostream>

namespace mogal
{

DoubleVectorGenome::DoubleVectorGenome(DoubleVectorGAFactory *pFactory, int numParams, int numFitnessValues)
{
	m_pFactory = pFactory;
	
	DoubleVectorGAFactoryParams *pParams = (DoubleVectorGAFactoryParams *)pFactory->getCurrentParameters();
	
	m_params.resize(numParams);
	m_fitness.resize(numFitnessValues);

	// Initialize object with random values

	const RandomNumberGenerator *pRndGen = m_pFactory->getRandomNumberGenerator();

	for (int i = 0 ; i < numParams ; i++)
		m_params[i] = pRndGen->pickRandomNumber();

	for (int i = 0 ; i < numFitnessValues ; i++)
		m_fitness[i] = -987;

	m_calculatedFitness = false;
	m_fitnessComponent = 0;
} 

DoubleVectorGenome::~DoubleVectorGenome()
{
}

bool DoubleVectorGenome::calculateFitness()
{
	if (m_calculatedFitness) // already calculated, nothing to do
	{
		//std::cout << "Already calculated fitness: " << m_fitness << std::endl;
		return true;
	}

	if (!calculateFitness(&(m_params[0]), m_params.size(), &(m_fitness[0]), m_fitness.size()))
		return false;

	m_calculatedFitness = true;

	return true;
}

bool DoubleVectorGenome::isFitterThan(const mogal::Genome *pGenome) const
{
	const DoubleVectorGenome *pGen = (const DoubleVectorGenome *)pGenome;
	double v1 = m_fitness[m_fitnessComponent];
	double v2 = pGen->m_fitness[m_fitnessComponent];
	return (v1 < v2);
}

mogal::Genome *DoubleVectorGenome::reproduce(const mogal::Genome *pGenome) const
{
	const DoubleVectorGenome *pGen = (const DoubleVectorGenome *)pGenome;
	const RandomNumberGenerator *pRndGen = m_pFactory->getRandomNumberGenerator();
	std::vector<double> newParams(m_params.size());

	for (int i = 0 ; i < m_params.size() ; i++)
	{
		if (pRndGen->pickRandomNumber() < 0.5)
			newParams[i] = m_params[i];
		else
			newParams[i] = pGen->m_params[i];
	}

	DoubleVectorGenome *pNewGenome = (DoubleVectorGenome *)m_pFactory->createNewGenome();

	pNewGenome->setParameters(&(newParams[0]));

	return pNewGenome;
}

mogal::Genome *DoubleVectorGenome::clone() const
{
	DoubleVectorGenome *pGenome = (DoubleVectorGenome *)m_pFactory->createNewGenome();

	pGenome->setParameters(&(m_params[0]));
	pGenome->setFitnessValues(&(m_fitness[0]));
	pGenome->setCalculatedFitness(m_calculatedFitness);

	return pGenome;
}

void DoubleVectorGenome::mutate()
{ 
	// std::cout << "Starting mutations" << std::endl;

	const RandomNumberGenerator *pRndGen = m_pFactory->getRandomNumberGenerator();
	double frac = 1.0/(double)(m_params.size()+1); // even is there's only one parameter, give it a 50/50 chance
	int mutationCount = 0;

	for (int i = 0 ; i < m_params.size() ; i++)
	{
		if (pRndGen->pickRandomNumber() < frac)
		{
			double val = m_params[i];
			double r = pRndGen->pickRandomNumber();

			if (r < 0.5) // 50% large mutations, rest smaller
			{
				double rnd = pRndGen->pickRandomNumber();
			
				val = rnd;

				// std::cout << "Large mutation: ";
			}
			else // smaller mutations
			{
				double amplitude = 0;

				amplitude = 0.05;

				double z = pRndGen->pickRandomNumber();
				//double x = std::tan(M_PI*(z-0.5)); // Cauchy distribution
				double x = 2.0*(z-0.5);

				x *= amplitude;
				val += x;

				if (val < 0)
					val = 0;
				if (val > 1.0)
					val = 1.0;

				// std::cout << "Smaller mutation: ";
			}
				
			// std::cout << m_params[count] << " -> " << val << std::endl;

			m_params[i] = val;
			mutationCount++;
		}
	}

	if (mutationCount > 0)
		m_calculatedFitness = false;
}

std::string DoubleVectorGenome::getFitnessDescription() const
{
	char str[1024];
	int i;

	std::string fitnessString = std::string("(");

	for (i = 0 ; i < m_fitness.size()-1 ; i++)
	{
		sprintf(str, "%g|", m_fitness[i]);
		fitnessString += std::string(str);
	}
	for (  ; i < m_fitness.size() ; i++)
	{
		sprintf(str, "%g", m_fitness[i]);
		fitnessString += std::string(str);
	}

	fitnessString += std::string(")");
	return fitnessString;
}

} // end namespace
