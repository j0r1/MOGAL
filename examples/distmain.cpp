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
