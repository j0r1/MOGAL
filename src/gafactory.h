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
 * \file gafactory.h
 */

#ifndef MOGAL_GAFACTORY_H

#define MOGAL_GAFACTORY_H

#include "geneticalgorithm.h"
#include <vector>

namespace mogal
{

/** Base class for the parameters of a genetic algorithm factory. */
class GAFactoryParams : public errut::ErrorBase
{
protected:
	GAFactoryParams()										{ }
public:
	~GAFactoryParams()										{ }

	/** Serialize the current parameters.
	 *  Serialize the current parameters. This is an abstract function and needs
	 *  to be implemented in a specific algorithm.
	 */
	virtual bool write(serut::SerializationInterface &si) const = 0;

	/** De-serialize the current parameters.
	 *  De-serialize the current parameters. This is an abstract function and needs
	 *  to be implemented in a specific algorithm.
	 */
	virtual bool read(serut::SerializationInterface &si) = 0;
};

/** A genetic algorithm factory, which contains all problem-specific code. 
 *  A genetic algorithm factory, which contains all problem-specific code. This interface
 *  contains several purely virtual functions which have to be implemented. Some defaults
 *  for single-objective and multi-objective genetic algorithms are provided in the
 *  classes GAFactorySingleObjective and GAFactoryMultiObjective respectively.
 */
class GAFactory : public errut::ErrorBase
{
protected:
	GAFactory()											{ }
public:
	~GAFactory()											{ }

	/** Allocate an instace of the GAFactoryParams implementation which corresponds
	 *  to this factory (should be deleted afterwards).
	 */
	virtual GAFactoryParams *createParamsInstance() const = 0;
	
	/** Returns an instance of the GAFactoryParams containing the parameters with
	 *  which this factory was initialized.
	 */
	virtual const GAFactoryParams *getCurrentParameters() const = 0;

	/** Initialize the factory.
	 *  Initialize the factory.
	 *  \param pFactoryParams Parameters which should be used during the factory
	 *                        initialization. This information must be copied since
	 *                        this particular instance may be destroyed!
	 */
	virtual bool init(const GAFactoryParams *pFactoryParams) = 0;

	/** Returns the number of fitness components.
	 *  Returns the number of fitness components. For a standard genetic algorithm,
	 *  this will be one; in a multi-objective genetic algorithm, multiple fitness
	 *  measures are optimized at the same time.
	 */
	virtual size_t getNumberOfFitnessComponents() const = 0;
	
	/** Creates a new, randomly initialized genome that corresponds to this particular
	 *  genetic algorithm.
	 */
	virtual Genome *createNewGenome() const = 0;

	/** Returns the maximum number of bytes that are needed for serializing a genome
	 *  belonging to this factory.
	 */
	virtual size_t getMaximalGenomeSize() const = 0;

	/** Returns the maximum number of bytes that are needed for serializing the fitness
	 *  measures belonging to this factory.
	 */
	virtual size_t getMaximalFitnessSize() const = 0;

	/** Serializes the genome information in \c pGenome. */
	virtual bool writeGenome(serut::SerializationInterface &si, const Genome *pGenome) const = 0;

	/** Serializes the fitness information in the genome \c pGenome. */
	virtual bool writeGenomeFitness(serut::SerializationInterface &si, const Genome *pGenome) const = 0;

	/** Writes some additional information after the \c pGenerationInfoChanged flag
	 *  in GAFactory::onGeneticAlgorithmStep was set (you probably won't need this). 
	 */
	virtual bool writeCommonGenerationInfo(serut::SerializationInterface &si) const = 0;

	/** De-serialize a genome and store it in \c pGenome. */
	virtual bool readGenome(serut::SerializationInterface &si, Genome **pGenome) const = 0;

	/** De-serialize the fitness information and store it in genome \c pGenome. */
	virtual bool readGenomeFitness(serut::SerializationInterface &si, Genome *pGenome) const = 0;

	/** Read some additional information (see GAFactory::writeCommonGenerationInfo). */
	virtual bool readCommonGenerationInfo(serut::SerializationInterface &si) = 0;

	/** This function is called each time a generation has been processed. 
	 *  This function is called each time a generation has been processed.
	 *  \param generation The current generation number.
	 *  \param pGenerationInfoChanged Set this to \c true if some additional information needs to be
	 *                                transmitted in a distributed calculation. This can be necessary
	 *                                if the fitness calculation procedure needs to be modified while
	 *                                the algorithm is running. You probably won't need this though.
	 *  \param pStopAlgorithm Set this to \c true when the stopping criterion for your algorithm
	 *                        has been reached. You must do this at some point, otherwise the algorithm
	 *                        will run forever.
	 */
	virtual void onGeneticAlgorithmStep(int generation, bool *pGenerationInfoChanged, bool *pStopAlgorithm) = 0;

	/** This is called when the algorithm is started. */
	virtual void onGeneticAlgorithmStart()								{ }

	/** This is called when the algorithm is stopped. */
	virtual void onGeneticAlgorithmStop()								{ }

	/** This is called after the population has been sorted using the GAFactory::sort function. */
	virtual void onSortedPopulation(const std::vector<GenomeWrapper> &population)			{ }

	/** Sort the genomes in \c population according to their fitness. */
	virtual void sort(std::vector<GenomeWrapper> &population) = 0;

	/** Inspect the genomes in \c population and update the current set of best genomes. */
	virtual void updateBestGenomes(std::vector<GenomeWrapper> &population) = 0;

	/** Create a new population based on an existing one. */
	virtual void breed(const std::vector<GenomeWrapper> &population, 
			   std::vector<GenomeWrapper> &newPopulation) = 0;

	/** Introduce mutations in the genomes in \c population. */
	virtual void introduceMutations(std::vector<GenomeWrapper> &population) = 0;

	/** From the genomes in \c bestGenomes, select the preferred one (this is necessary in a 
	 *  multi-objective genetic algorithm.
	 */
	virtual Genome *selectPreferredGenome(const std::list<Genome *> &bestGenomes) const = 0;
protected:
	/** Returns a pointer to the current genetic algorithm instance. */
	GeneticAlgorithm *getCurrentAlgorithm()								{ return m_pCurrentAlgorithm; }

	/** Returns a pointer to the current genetic algorithm parameters. */
	GeneticAlgorithmParams getCurrentAlgorithmParameters() const					{ return m_gaParams; }
private:
	void setCurrentAlgorithm(GeneticAlgorithm *pAlgorithm, const GeneticAlgorithmParams &params)	{ m_pCurrentAlgorithm = pAlgorithm; m_gaParams = params; }
	
	GeneticAlgorithm *m_pCurrentAlgorithm;
	GeneticAlgorithmParams m_gaParams;

	friend class GeneticAlgorithm;
};

} // end namespace

#endif // MOGAL_GAFACTORY_H

