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

#include "distparams.h"
#include <mogal/geneticalgorithm.h>
#include <enut/ipv4address.h>
#include <iostream>

int main(void)
{
	DistGAFactoryParams factoryParams(-1,-1,20);
	nut::IPv4Address serverIP(127,0,0,1);
	uint16_t serverPort = 22000;
	mogal::GeneticAlgorithm ga;

	if (!ga.run(serverIP, serverPort, "distmodule.so", 128, "./", &factoryParams))
	{
		std::cerr << ga.getErrorString() << std::endl;
		return -1;
	}

	std::list<mogal::Genome *> bestGenomes;
	std::list<mogal::Genome *>::const_iterator it;

	ga.getBestGenomes(bestGenomes);

	std::cout << "Best genomes: " << std::endl;
	
	for (it = bestGenomes.begin() ; it != bestGenomes.end() ; it++)
	{
		const mogal::Genome *pGenome = *it;
		
		std::cout << "  " << pGenome->getFitnessDescription() << std::endl;
	}

	return 0;
}
