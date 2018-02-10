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
 * \file gafactorydefaults.h
 */

#ifndef MOGAL_GAFACTORYDEFAULTS_H

#define MOGAL_GAFACTORYDEFAULTS_H

#include "gafactory.h"
#include <stdlib.h>
#include <cmath>

namespace mogal
{

// NOTE: the virtual inheritance is of extreme importance here!

/** Helper class for single-objective and multi-objective base classes. */
class GAFactoryDefaults : public virtual GAFactory
{
public:
	~GAFactoryDefaults()										{ }
protected:
	GAFactoryDefaults();

	void commonBreed(int startOffset, const std::vector<GenomeWrapper> &population, 
	                 std::vector<GenomeWrapper> &newPopulation);

	int pickGenome(int num)
	{
		double x;
		drand48_r(&m_drandBuffer, &x);

		double val = x*(double)num; 
		return (int)val;
	}

	int pickGenome(double beta, size_t populationSize)
	{
		double x;
		drand48_r(&m_drandBuffer, &x);

		double val = (1.0-std::pow(x, 1.0/(1.0+beta)))*((double)populationSize);
		return (int)val;
	}

	double pickDouble()
	{
		double x;
		drand48_r(&m_drandBuffer, &x);
		return x;
	}
private:
	struct drand48_data m_drandBuffer;
};

} // end namespace

#endif // MOGAL_GAFACTORYDEFAULTS_H
