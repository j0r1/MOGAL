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

#include "mogalconfig.h"
#ifdef MOGALCONFIG_MPISUPPORT

#include <mpi.h>
#include "loglevels.h"
#include "gafactory.h"
#include <serut/memoryserializer.h>
#include <serut/dummyserializer.h>

#endif // MOGALCONFIG_MPISUPPORT

#include "mpigeneticalgorithm.h"



namespace mogal
{

MPIGeneticAlgorithm::MPIGeneticAlgorithm()
{
}

MPIGeneticAlgorithm::~MPIGeneticAlgorithm()
{
}

#ifndef MOGALCONFIG_MPISUPPORT

bool MPIGeneticAlgorithm::runMPIHelper(GAFactory &factory)
{
	setErrorString("No MPI support was available at compile time");
	return false;
}

bool MPIGeneticAlgorithm::runMPI(GAFactory &factory, size_t populationSize, const GeneticAlgorithmParams *pParams)
{
	setErrorString("No MPI support was available at compile time");
	return false;
}

bool MPIGeneticAlgorithm::calculateFitness(std::vector<GenomeWrapper> &population)
{
	setErrorString("No MPI support was available at compile time");
	return false;
}

#else // MPI support available

bool MPIGeneticAlgorithm::runMPIHelper(GAFactory &factory)
{
	bool done = false;

	while (!done)
	{
		int numGenomes = 0;
		MPI_Status status;

		MPI_Recv(&numGenomes, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		// std::cout << "Code = " << numGenomes << std::endl;

		if (numGenomes == -1)
			done = true;
		else if (numGenomes == -2) // signals new generation info
		{
			int genInfoSize = 0;

			MPI_Recv(&genInfoSize, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			// std::cout << "runMPIHelper::genInfoSize = " << genInfoSize << std::endl;

			if (genInfoSize >= 0)
			{
				std::vector<uint8_t> genInfoBuffer(genInfoSize);
				serut::MemorySerializer mSer(&(genInfoBuffer[0]), genInfoSize, 0, 0);

				MPI_Recv(&(genInfoBuffer[0]), genInfoSize, MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				if (!factory.readCommonGenerationInfo(mSer))
				{
					setErrorString("Error reading common generation info: " + factory.getErrorString());
					return false;
				}
			}
		}
		else if (numGenomes >= 0)
		{
			// Read genomes and calculate fitness

			int genomeBufferSize = numGenomes * factory.getMaximalGenomeSize();
			int fitnessBufferSize = numGenomes * factory.getMaximalFitnessSize();
			std::vector<uint8_t> genomeBuffer(genomeBufferSize);
			std::vector<uint8_t> fitnessBuffer(fitnessBufferSize);

			MPI_Recv(&(genomeBuffer[0]), genomeBufferSize, MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			serut::MemorySerializer mSer(&(genomeBuffer[0]), genomeBufferSize, &(fitnessBuffer[0]), fitnessBufferSize);

			for (int i = 0 ; i < numGenomes ; i++)
			{
				Genome *pGenome = 0;

				if (!factory.readGenome(mSer, &pGenome))
				{
					setErrorString("Error reading genome from received data: " + factory.getErrorString());
					return false;
				}

				if (!pGenome->calculateFitness())
				{
					setErrorString("Error calculating genome fitness remotely");
					delete pGenome;
					return false;
				}

				if (!factory.writeGenomeFitness(mSer, pGenome))
				{
					setErrorString("Error writing genome fitness: " + factory.getErrorString());
					delete pGenome;
					return false;
				}

				delete pGenome;
			}

			// Send back fitness results

			MPI_Send(&(fitnessBuffer[0]), mSer.getBytesWritten(), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
		}
		else
		{
			writeLog(LOG_ERROR, "Got invalid number of genomes %d", numGenomes);
			return false;
		}
	}

	// std::cout << "runMPIHelper: end" << std::endl;

	return true;
}

bool MPIGeneticAlgorithm::runMPI(GAFactory &factory, size_t populationSize, const GeneticAlgorithmParams *pParams)
{
	MPI_Comm_size( MPI_COMM_WORLD, &m_numProcesses);

	m_generationInfoCount = 0;
	m_lastSentGenInfo = 0;
	m_firstPopulation = true;
	m_pFactory = &factory;

	bool retValue = run(factory, populationSize, pParams);

	// Make sure everyone stops

	int stopCode = -1;
	
	for (int i = 1 ; i < m_numProcesses ; i++)
		MPI_Send(&stopCode, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

	return retValue;
}

bool MPIGeneticAlgorithm::calculateFitness(std::vector<GenomeWrapper> &population)
{
	writeLog(LOG_DEBUG, "Entering MPIGeneticAlgorithm::calculateFitness");

	if (m_firstPopulation)
	{
		/*	
		std::cout << "m_numProcesses " << m_numProcesses << std::endl;
		std::cout << "popSize " << population.size() << std::endl;
		*/
		m_firstPopulation = false;
		m_genomesPerNode.resize(m_numProcesses);
		m_genomeOffset.resize(m_numProcesses);

		int genomesDone = 0;

		for (int i = 0 ; i < m_numProcesses ; i++)
		{
			int totalCount = ((double)(i+1)/(double)(m_numProcesses)*(double)population.size() + 0.5);

			if (totalCount > population.size())
				totalCount = population.size();

			int count = totalCount - genomesDone;

			m_genomesPerNode[i] = count;
			genomesDone += count;
		}

		/*
		std::cout << "new size 1: " << (population.size() * m_pFactory->getMaximalGenomeSize()) << std::endl;
		std::cout << "new size 2: " << (population.size() * m_pFactory->getMaximalFitnessSize()) << std::endl;
		std::cout << "m_pFactory " << (void *)m_pFactory << std::endl;
		std::cout << "getMaximalGenomeSize " << m_pFactory->getMaximalGenomeSize() << std::endl;
		std::cout << "getMaximalFitnessSize " << m_pFactory->getMaximalFitnessSize() << std::endl;
		*/
		m_genomeBuffer.resize(population.size() * m_pFactory->getMaximalGenomeSize());
		m_fitnessBuffer.resize(population.size() * m_pFactory->getMaximalFitnessSize());
	}

	// Distribute generation info if necessary

	/*
	std::cout << "m_lastSentGenInfo = " << m_lastSentGenInfo << std::endl;
	std::cout << "m_generationInfoCount = " << m_generationInfoCount << std::endl;
	*/

	if (m_lastSentGenInfo != m_generationInfoCount)
	{
		writeLog(LOG_DEBUG, "Sending generation info");

		m_lastSentGenInfo = m_generationInfoCount;

		serut::DummySerializer dSer;
		m_pFactory->writeCommonGenerationInfo(dSer);

		int code = -2;
		int genInfoSize = dSer.getBytesWritten();

		std::vector<uint8_t> genInfo(genInfoSize);
		serut::MemorySerializer mSer(0, 0, &(genInfo[0]), genInfoSize);

		if (!m_pFactory->writeCommonGenerationInfo(mSer))
		{
			writeLog(LOG_ERROR, "Error writing generation info: %s", m_pFactory->getErrorString().c_str());
			setErrorString("Couldn't write generation info to memory");
			return false;
		}

		for (int i = 1 ; i < m_numProcesses ; i++)
		{
			MPI_Send(&code, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&genInfoSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

			// std::cout << "runMPI::genInfoSize = " << genInfoSize << std::endl;

			if (genInfoSize >= 0)
				MPI_Send(&(genInfo[0]), genInfoSize, MPI_BYTE, i, 0, MPI_COMM_WORLD);

		}
	}

	// Distribute genomes

	int offset = 0;

	for (int i = 1 ; i < m_numProcesses ; i++)
	{
		m_genomeOffset[i] = offset;

		serut::MemorySerializer mSer(0, 0, &(m_genomeBuffer[0]), m_genomeBuffer.size());
		int num = m_genomesPerNode[i];
		int numGenomes = num;

		for (int j = 0 ; j < num ; j++)
		{
			Genome *pGenome = population[offset+j].getGenome();

			if (!m_pFactory->writeGenome(mSer, pGenome))
			{
				writeLog(LOG_ERROR, "Error writing genome: %s", m_pFactory->getErrorString().c_str());
				setErrorString("Coulnd't write genome info to memory");
				return false;
			}
		}

		writeLog(LOG_DEBUG, "Sending %d genomes to node %d (%d bytes)", num, i, mSer.getBytesWritten());
		MPI_Send(&numGenomes, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		MPI_Send(&(m_genomeBuffer[0]), mSer.getBytesWritten(), MPI_BYTE, i, 0, MPI_COMM_WORLD);
		offset += num;
	}

	int remainingGenomes = population.size() - offset;

	if (remainingGenomes != m_genomesPerNode[0]) // Should not happen, just to be safe
	{
		writeLog(LOG_ERROR, "Remaining genomes = %d, genomesPerNode[0] = %d", remainingGenomes, m_genomesPerNode[0]);
		setErrorString("Remaining genomes does not match number of genomes for node 0");
		return false;
	}

	// Calculate some genomes locally
	
	writeLog(LOG_DEBUG, "Calculating %d genomes locally", remainingGenomes);

	for (int j = 0 ; j < remainingGenomes ; j++)
	{
		Genome *pGenome = population[offset+j].getGenome();

		if (!pGenome->calculateFitness())
		{
			writeLog(LOG_ERROR, "Couldn't calculate genome fitness locally");
			setErrorString("Coudln't calculate genome fitness locally");
			return false;
		}
	}

	// Receive genomes fitness info calculated by other nodes
	
	int nodeCount = 1; // don't count ourselves

	while (nodeCount < m_numProcesses)
	{
		MPI_Status status;

		MPI_Recv(&(m_fitnessBuffer[0]), m_fitnessBuffer.size(), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		int src = status.MPI_SOURCE;
		int num = m_genomesPerNode[src];
		int offset = m_genomeOffset[src];
		serut::MemorySerializer mSer(&(m_fitnessBuffer[0]), m_fitnessBuffer.size(), 0, 0);

		writeLog(LOG_DEBUG, "Received result from node %d", src);

		for (int j = 0 ; j < num ; j++)
		{
			Genome *pGenome = population[offset+j].getGenome();

			if (!m_pFactory->readGenomeFitness(mSer, pGenome))
			{
				writeLog(LOG_ERROR, "Couldn't retrieve genome fitness: %s", m_pFactory->getErrorString().c_str());
				setErrorString("Coudln't read genome fitness");
				return false;
			}
		}

		nodeCount++;
	}

	writeLog(LOG_DEBUG, "Leaving MPIGeneticAlgorithm::calculateFitness");

	return true;
}

#endif // MOGALCONFIG_MPISUPPORT

bool MPIGeneticAlgorithm::onAlgorithmLoop(GAFactory &factory, bool generationInfoChanged)
{
	if (generationInfoChanged)
		m_generationInfoCount++;

	return true;
}

} // end namespace

