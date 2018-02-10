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

#ifndef MOGAL_DOUBLEVECTORGENOME_H

#define MOGAL_DOUBLEVECTORGENOME_H

#include "mogalconfig.h"
#include "genome.h"
#include <vector>

namespace mogal
{

class DoubleVectorGAFactory;

class MOGAL_IMPORTEXPORT DoubleVectorGenome : public Genome
{
public:
	DoubleVectorGenome(DoubleVectorGAFactory *pFactory, int numParams, int numFitnessValues);
	~DoubleVectorGenome();

	bool calculateFitness();
	void setActiveFitnessComponent(int componentNumber)							{ m_fitnessComponent = componentNumber; }
	bool isFitterThan(const mogal::Genome *pGenome) const;
	Genome *reproduce(const mogal::Genome *pGenome) const;
	mogal::Genome *clone() const;
	void mutate();
	
	std::string getFitnessDescription() const;

	void setParameters(const double *pValues)								{ for (int i = 0 ; i < m_params.size() ; i++) m_params[i] = pValues[i]; }
	const double *getParameters() const									{ return &(m_params[0]); }
	int getNumberOfParameters() const									{ return m_params.size(); }

	void setFitnessValues(const double *pValues)								{ for (int i = 0 ; i < m_fitness.size() ; i++) m_fitness[i] = pValues[i]; }
	const double *getFitnessValues() const									{ return &(m_fitness[0]); }
	int getNumberOfFitnessValues() const									{ return m_fitness.size(); }

	void setCalculatedFitness(bool f = true)								{ m_calculatedFitness = f; }
	bool hasCalculatedFitness() const									{ return m_calculatedFitness; }

	DoubleVectorGAFactory *getFactory()									{ return m_pFactory; }
protected:
	virtual bool calculateFitness(const double *pParams, int numParams, double *pFitnessValues, int numFitnessValues) = 0;
private:
	DoubleVectorGAFactory *m_pFactory;

	std::vector<double> m_params;
	std::vector<double> m_fitness;
	bool m_calculatedFitness;
	int m_fitnessComponent;
};

} // end namespace

#endif // MOGAL_DOUBLEVECTORGENOME_H

