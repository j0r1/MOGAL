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

#ifndef MOGAL_DOUBLEVECTORGAFACTORYPARAMS_H

#define MOGAL_DOUBLEVECTORGAFACTORYPARAMS_H

#include "mogalconfig.h"
#include "gafactory.h"

namespace mogal
{

class MOGAL_IMPORTEXPORT DoubleVectorGAFactoryParams : public GAFactoryParams
{
public:
	DoubleVectorGAFactoryParams();
	DoubleVectorGAFactoryParams(int numGenomeParams);
	~DoubleVectorGAFactoryParams();

	int getNumberOfParameters() const								{ return m_numGenomeParams; }

	bool write(serut::SerializationInterface &si) const;
	bool read(serut::SerializationInterface &si);
private:
	int m_numGenomeParams;
};

} // end namespace

#endif // MOGAL_DOUBLEVECTORGAFACTORYPARAMS_H

