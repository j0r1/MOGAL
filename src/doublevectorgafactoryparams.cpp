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

#include "doublevectorgafactoryparams.h"
#include <stdio.h>

namespace mogal
{

DoubleVectorGAFactoryParams::DoubleVectorGAFactoryParams()
{
	m_numGenomeParams = -1;
}

DoubleVectorGAFactoryParams::DoubleVectorGAFactoryParams(int numParams)
{
	m_numGenomeParams = numParams;
}

DoubleVectorGAFactoryParams::~DoubleVectorGAFactoryParams()
{
}

bool DoubleVectorGAFactoryParams::write(serut::SerializationInterface &si) const
{
	if (!si.writeInt32(m_numGenomeParams))
	{
		setErrorString("Can't write the number of genome parameters: " + si.getErrorString());
		return false;
	}
	return true;
}

bool DoubleVectorGAFactoryParams::read(serut::SerializationInterface &si)
{
	int32_t v;

	if (!si.readInt32(&v))
	{
		setErrorString("Can't read the number of genome parameters: " + si.getErrorString());
		return false;
	}

	if (v < 1)
	{
		char str[1024];

		sprintf(str, "The number of genome parameters read was less than one (%d)", (int)v);
		setErrorString(str);
		return false;
	}

	m_numGenomeParams = v;

	return true;
}

} // end namespace
