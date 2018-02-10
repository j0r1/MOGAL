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

#include "gamodule.h"

namespace mogal
{

#define GAMODULE_ERRSTR_NOTOPENED		"No module has been opened yet"

GAModule::GAModule()
{
	registerFunctionName("CreateFactoryInstance");
}

GAModule::~GAModule()
{
}

GAFactory *GAModule::createFactoryInstance() const
{
	if (!isOpen())
	{
		setErrorString(GAMODULE_ERRSTR_NOTOPENED);
		return 0;
	}
	return m_createFactory();
}

void GAModule::onFoundFunctionName(const std::string &name, void *pFunction)
{ 
	if (name == std::string("CreateFactoryInstance"))
		m_createFactory = (CreateFactoryInstanceFunction)pFunction;
}

} // end namespace

