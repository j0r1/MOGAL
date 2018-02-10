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

#include "modulebase.h"
#include <dlfcn.h>

namespace mogal
{

#define MODULEBASE_ERRSTR_ALREADYOPEN			"A module has already been opened"
#define MODULEBASE_ERRSTR_NOTOPEN			"No module has been opened yet"
#define MODULEBASE_ERRSTR_COULDNTFINDFUNCTION		"Couldn't find the function name: "
#define MODULEBASE_ERRSTR_COULDNTOPEN			"Couldn't open the module: "

ModuleBase::ModuleBase()
{
	m_pModule = 0;
}

ModuleBase::~ModuleBase()
{
	close();
}

bool ModuleBase::open(const std::string &modulePath)
{
	if (m_pModule != 0)
	{
		setErrorString(MODULEBASE_ERRSTR_ALREADYOPEN);
		return false;
	}
	
	void *pModule = dlopen(modulePath.c_str(), RTLD_NOW);

	if (pModule == 0)
	{
		setErrorString(std::string(MODULEBASE_ERRSTR_COULDNTOPEN) + std::string(dlerror()));
		return false;
	}

	// look for requested symbols
	std::list<std::string>::const_iterator it;

	for (it = m_functionNames.begin() ; it != m_functionNames.end() ; it++)
	{
		std::string name = (*it);
		void *pFunction = dlsym(pModule, name.c_str());
		
		if (pFunction == 0)
		{
			setErrorString(std::string(MODULEBASE_ERRSTR_COULDNTFINDFUNCTION) + name);
			dlclose(pModule);
			return false;
		}

		onFoundFunctionName(name, pFunction);
	}

	m_pModule = pModule;
	return true;
}

bool ModuleBase::open(const std::string &baseDirectory, const std::string &moduleName)
{
	std::string fullPath = baseDirectory + std::string("/") + moduleName;
	return open(fullPath);
}

bool ModuleBase::close()
{
	if (m_pModule == 0)
	{
		setErrorString(MODULEBASE_ERRSTR_NOTOPEN);
		return false;
	}

	dlclose(m_pModule);
	m_pModule = 0;

	return true;
}

} // end namespace

