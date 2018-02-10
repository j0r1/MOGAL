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
 * \file genome.h
 */

#ifndef MOGAL_GENOME_H

#define MOGAL_GENOME_H

#include <string>

namespace mogal
{

/** Interface for a genome in the genetic algorithm. */
class Genome
{
public:
	Genome()											{ }
	virtual ~Genome()										{ }

	/** Calculate the fitness of this genome. */
	virtual void calculateFitness() = 0;

	/** In a multi-objective genetic algorithm, the fitness component
	 *  specified by \c componentNumber will be used when comparing genomes
	 *  in the Genome::isFitterThan function. 
	 */
	virtual void setActiveFitnessComponent(int componentNumber)					{ }

	/** This function should return \c true is the current genome is considered
	 *  to be strictly more fit than the genome in \c pGenome.
	 */
	virtual bool isFitterThan(const Genome *pGenome) const = 0;

	/** Return a new genome by combining the current genome and the one in \c pGenome. */
	virtual Genome *reproduce(const Genome *pGenome) const = 0;
	
	// NOTE: Clone should also copy fitness info etc, since it is also used to store the
	//       current best genome

	/** Create an exact copy of the current genome (this should also copy the calculated
	 *  fitness information, since it is also used to store the current best genomes).
	 */
	virtual Genome *clone() const = 0;
	
	/** Introduce mutations in this genome. */
	virtual void mutate() = 0;

	/** Return a description of the fitness of this genome. */
	virtual std::string getFitnessDescription() const = 0;
};

/** Wrapper class to sort genomes and to track their evolution. */
class GenomeWrapper
{
public:
	/** Create a new genome wrapper.
	 *  Create a new genome wrapper.
	 *  \param pGenome The genome to wrap.
	 *  \param parent1 Index of the first parent (after sorting)
	 *  \param parent2 Index of the second parent (after sorting)
	 *  \param position Position of the genome in the new population
	 */
	GenomeWrapper(Genome *pGenome = 0, int parent1 = -1, int parent2 = -1, int position = -1)	{ m_pGenome = pGenome; m_parent1 = parent1; m_parent2 = parent2; m_position = position; }

	~GenomeWrapper()										{ }

	/** Return the stored genome. */
	Genome *getGenome() const									{ return m_pGenome; }

	/** Return the index (after sorting) of the first parent. */
	int getParent1() const										{ return m_parent1; }

	/** Return the index (after sorting) of the second parent. */
	int getParent2() const										{ return m_parent2; }

	/** Return the own position (after sorting) in the population. */
	int getPosition() const										{ return m_position; }
private:
	Genome *m_pGenome;
	int m_parent1;
	int m_parent2;
	int m_position;
};

inline bool operator< (const GenomeWrapper &g1, const GenomeWrapper &g2)
{
	return g1.getGenome()->isFitterThan(g2.getGenome());
}

} // end namespace

#endif // MOGAL_GENOME_H

