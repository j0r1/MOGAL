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
 * \file geneticalgorithm.h
 */

#ifndef MOGAL_GENETICALGORITHM_H

#define MOGAL_GENETICALGORITHM_H

#include "genome.h"
#include <serut/serializationinterface.h>
#include <enut/networklayeraddress.h>
#include <stdlib.h>
#include <vector>
#include <list>

namespace mogal
{

class GAFactory;
class GAFactoryParams;
class GAModule;
class Genome;

/** Generic parameters for a genetic algorithm. 
 *  Generic parameters for a genetic algorithm.
 *  
 *  The selection pressure is specified by the parameter 'beta'. In the default
 *  implementations, genomes are sorted according to their fitness; the best genomes
 *  get low indices in the population array, the worst genomes get high index numbers.
 *  When selecting genomes from this sorted population, genomes are picked according
 *  to the following probability distribution:
 *  \f[
 *	p(x) = \frac{1+\beta}{s}\left(1-\frac{x}{s}\right)^\beta
 *  \f]
 *  where \c s is the population size. The default 'beta' value is 2.5; for a population
 *  size of one hundred, the above function is shown below. This probability function
 *  then clearly favors genomes with low index numbers, which are the genomes which are
 *  deemed more fit.
 *  \image html beta.png
 *  The best setting for the selection pressure will depend on the problem at hand.
 *  If the global optimum can be reached relatively easily, a high setting of this
 *  parameter will cause the algorithm to converge quickly. For a more difficult
 *  problem however, a high selection pressure may cause the algorithm to get stuck
 *  in a local optimum. A better exploration of the solution space can be obtained
 *  by using a lower selection pressure. Note that if this value becomes too small,
 *  all the genomes will appear equally good, and little progress will be made. 
 *
 *  If the 'elitism' flag is set, the default implementations will copy the best genome(s)
 *  to the next generation and these will not be mutated. The same thing will happen if
 *  the 'includeBest' flag is set, but then mutations are indeed allowed. By default,
 *  both of these flags are set to \c true.
 *
 *  The 'crossoverRate' parameter is used by the default implementations to select which
 *  fraction of the genomes in the new population should be created by combining two
 *  parent genomes. The rest of the new population is created by cloning.
 */
class GeneticAlgorithmParams : public errut::ErrorBase
{
public:
	/** Construct an instance with default parameter values. */
	GeneticAlgorithmParams();

	/** Construct an instance with specific parameter values. 
	 *  Construct an instance with specific parameter values. The meaning of
	 *  these parameters is described in the class documentation of GeneticAlgorithmParams.
	 */
	GeneticAlgorithmParams(double beta, bool elitism, bool includeBest, double crossoverRate);
	~GeneticAlgorithmParams();

	/** Returns the current selection pressure value. */
	double getBeta() const									{ return m_beta; }

	/** Returns a flag indicating if elitism should be used. */
	bool useElitism() const									{ return m_elitism; }

	/** If this flag is \c true, the best genomes of a generation are copied to the next
	 *  generation, but are still allowed to be mutated.
	 */
	bool alwaysIncludeBestGenome() const							{ return m_alwaysIncludeBest; }

	/** Returns the currently set cross-over rate. */
	double getCrossOverRate() const								{ return m_crossoverRate; }

	bool write(serut::SerializationInterface &si);
	bool read(serut::SerializationInterface &si);
private:
	double m_beta, m_crossoverRate;
	bool m_elitism, m_alwaysIncludeBest;
};
	
/** Interface for running a genetic algorithm. */
class GeneticAlgorithm : public errut::ErrorBase
{
public:
	GeneticAlgorithm();
	~GeneticAlgorithm();

	/** Run a genetic algorithm.
	 *  Run a genetic algorithm.
	 *  \param factory The factory which should be used. This factory has to be initialized before
	 *                 this function is called.
	 *  \param populationSize Size of the population.
	 *  \param pParams Parameters for the genetic algorithm.
	 */
	bool run(GAFactory &factory, size_t populationSize, const GeneticAlgorithmParams *pParams = 0);

	/** Run a genetic algorithm.
	 *  Run a genetic algorithm.
	 *  \param factoryName Name of the module from which the genetic algorithm factory should be loaded.
	 *  \param populationSize Size of the population.
	 *  \param baseDir Directory in which the module resides.
	 *  \param pFactoryParams Parameters which should be used in the initialization of the factory.
	 *  \param pParams Parameters for the genetic algorithm.
	 */
	bool run(const std::string &factoryName, size_t populationSize, 
	         const std::string &baseDir, const GAFactoryParams *pFactoryParams, 
		 const GeneticAlgorithmParams *pParams = 0);

	/** Run a genetic algorithm.
	 *  Run a genetic algorithm.
	 *  \param serverAddress IP address of the server which should be used for distributed execution of
	 *                       the genetic algorithm.
	 *  \param serverPort Port number on which the server is listening for incoming connections.
	 *  \param factoryName Name of the module from which the genetic algorithm factory should be loaded
	 *                     at the server.
	 *  \param populationSize Size of the population.
	 *  \param factory The factory which should be used. This factory has to be initialized before
	 *                 this function is called.
	 *  \param pParams Parameters for the genetic algorithm.
	 */
	bool run(const nut::NetworkLayerAddress &serverAddress, uint16_t serverPort, 
		 const std::string &factoryName, size_t populationSize, 
		 GAFactory &factory, const GeneticAlgorithmParams *pParams = 0);

	/** Run a genetic algorithm.
	 *  Run a genetic algorithm.
	 *  \param serverAddress IP address of the server which should be used for distributed execution of
	 *                       the genetic algorithm.
	 *  \param serverPort Port number on which the server is listening for incoming connections.
	 *  \param factoryName Name of the module from which the genetic algorithm factory should be loaded
	 *                     both locally and at the server.
	 *  \param populationSize Size of the population.
	 *  \param baseDir Directory in which the module resides locally.
	 *  \param pFactoryParams Parameters which should be used in the initialization of the factory.
	 *  \param pParams Parameters for the genetic algorithm.
	 */
	bool run(const nut::NetworkLayerAddress &serverAddress, uint16_t serverPort, const std::string &factoryName, 
	         size_t populationSize, const std::string &baseDir, const GAFactoryParams *pFactoryParams, 
		 const GeneticAlgorithmParams *pParams = 0);

	/** Stores the current best genomes in \c genomes (these should not be deleted). */
	void getBestGenomes(std::list<Genome *> &genomes) const;

	/** Stores the current best genomes in \c genomes (these should not be deleted). */
	void getBestGenomes(std::vector<Genome *> &genomes) const;

	/** Returns the current number of best genomes. */
	size_t getNumberOfBestGenomes() const							{ return m_bestGenomes.size(); }

	/** Copies the genomes in \c genomes and sets these as the current set of best genomes. */
	void setBestGenomes(const std::list<Genome *> &genomes);

	/** This function selects the preferred genome from the current set of best genomes,
	 *  according to the preferences in the current factory.
	 */
	Genome *selectPreferredGenome() const;
	
	/** Clears the current set of best genomes. */
	void clearBestGenomes();

	/** Returns the current generation number. */
	int getCurrentGeneration() const							{ return m_generationCount; }
protected:
	virtual bool calculateFitness(std::vector<GenomeWrapper> &population);
	virtual bool onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged);
	virtual void onCurrentBest(const std::list<Genome *> &bestGenomes) const		{ }
	virtual void feedbackStatus(const std::string &str) const				{ }
private:
	bool processNewBestGenomes(GAFactory &factory, serut::SerializationInterface &si);
	
	std::list<Genome *> m_bestGenomes;
	GAModule *m_pGAModule;
	GAFactory *m_pLoadedFactory;
	GAFactory *m_pCurrentFactory;
	int m_generationCount;
};
	
} // end namespace

#endif // MOGAL_GENETICALGORITHM_H

