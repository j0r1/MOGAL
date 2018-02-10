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
 * \file modulebase.h
 */

#ifndef MOGAL_MODULEBASE_H

#define MOGAL_MODULEBASE_H

#include <errut/errorbase.h>
#include <string>
#include <list>

namespace mogal
{

/** Helper class for loading modules. */
class ModuleBase : public errut::ErrorBase
{
protected:
	ModuleBase();
	~ModuleBase();
public:
	/** Opens a module.
	 *  This function opens a module and verifies that all function names
	 *  which have been registered by ModuleBase::registerFunctionName are present
	 *  in the module. For each of these function names, the ModuleBase::onFoundFunctionName
	 *  member function will be called.
	 *  \param modulePath Complete path to the module.
	 */
	bool open(const std::string &modulePath);

	/** Opens a module.
	 *  Opens a module. This function constructs the complete path from the
	 *  arguments and calls the function below.
	 *  \param baseDirectory Directory in which the module is located.
	 *  \param moduleName File name of the module
	 */
	bool open(const std::string &baseDirectory, const std::string &moduleName);

	/** Close a previously opened module. */
	bool close();

	/** Returns \c true if a module has been successfully opened, \c false otherwise. */
	bool isOpen() const									{ return (m_pModule == 0)?false:true; }
protected:
	/** This function is called for each function name which has been registered
	 *  by a call to ModuleBase::registerFunctionName and which is present in the
	 *  module.
	 *  This function is called for each function name which has been registered
	 *  by a call to ModuleBase::registerFunctionName and which is present in the
	 *  module.
	 *  \param name Name of the function.
	 *  \param pFunction Entry point to the function.
	 */
	virtual void onFoundFunctionName(const std::string &name, void *pFunction) = 0;

	/** Register a function name which should be present in a module when it is opened. */
	void registerFunctionName(const std::string &name)					{ m_functionNames.push_back(name); }
private:
	void *m_pModule;
	std::list<std::string> m_functionNames;	
};
	
} // end namespace

#endif // MOGAL_MODULEBASE_H

