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
 * \file randomnumbergenerator.h
 */

#ifndef MOGAL_RANDOMNUMBERGENERATOR_H

#define MOGAL_RANDOMNUMBERGENERATOR_H

#include "mogalconfig.h"
#include <stdint.h>

namespace mogal
{

/** Abstract interface for a random number generator. */
class MOGAL_IMPORTEXPORT RandomNumberGenerator
{
public:
	RandomNumberGenerator()										{ }
	virtual ~RandomNumberGenerator()								{ }

	/** A random number generator must implement this function, which should generate a random double between 0 and 1. */
	virtual double pickRandomNumber() const = 0;
protected:
	/** Can be used by derived classes to pick a more or less random seed. */
	uint32_t pickSeed() const;
};

} // end namespace

#endif // MOGAL_RANDOMNUMBERGENERATOR_H
