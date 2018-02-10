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

/**
 * \file mpigeneticalgorithm.h
 */

#ifndef MOGAL_MPIGENETICALGORITHM_H

#define MOGAL_MPIGENETICALGORITHM_H

#include "mogalconfig.h"
#include "geneticalgorithm.h"

namespace mogal
{

/** Interface for running a genetic algorithm of which the genome fitnesses are
 *  calculated in a distributed way using MPI.
 *  Interface for running a genetic algorithm of which the genome fitnesses are
 *  calculated in a distributed way using MPI. Note that currently no load balancing
 *  is performed, each node will get approximately the same amount of genomes to
 *  calculate.
 */
class MOGAL_IMPORTEXPORT MPIGeneticAlgorithm : public GeneticAlgorithm
{
public:
	MPIGeneticAlgorithm();
	~MPIGeneticAlgorithm();

	/** Start this function on the root node (rank 0) to start a distributed GA.
	 *  Start this function on the root node (rank 0) to start a distributed GA.
	 *  \param factory The factory which should be used. This factory has to be initialized before
	 *                 this function is called.
	 *  \param populationSize Size of the population.
	 *  \param pParams Parameters for the genetic algorithm.
	 */
	bool runMPI(GAFactory &factory, size_t populationSize, const GeneticAlgorithmParams *pParams = 0);

	/** When the runMPI function is started on the root node, this function should be
	 *  called on the helper nodes.
	 *  When the runMPI function is started on the root node, this function should be
	 *  called on the helper nodes.
	 *  \param factory The factory which should be used. This factory has to be initialized before
	 *                 this function is called. Note that this is not distributed using the MPI
	 *                 mechanism, you'll need to make sure yourself that each node has an initialized
	 *                 version of the factory when this function is called.
	 */
	bool runMPIHelper(GAFactory &factory);
protected:
	virtual void writeLog(int level, const char *fmt, ...)							{ }
	bool onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged);
private:
	bool calculateFitness(std::vector<GenomeWrapper> &population);
	
	int m_numProcesses;
	int m_generationInfoCount, m_lastSentGenInfo;
	bool m_firstPopulation;
	std::vector<int> m_genomesPerNode;
	std::vector<int> m_genomeOffset;
	std::vector<uint8_t> m_genomeBuffer;
	std::vector<uint8_t> m_fitnessBuffer;
	GAFactory *m_pFactory;
};
	
} // end namespace

#endif // MOGAL_GENETICALGORITHM_H

