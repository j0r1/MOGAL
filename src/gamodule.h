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
 * \file gamodule.h
 */

#ifndef MOGAL_GAMODULE_H

#define MOGAL_GAMODULE_H

#include "modulebase.h"

namespace mogal
{

class GAFactory;

/** Class for loading a genetic algorithm factory.
 *  Class for loading a genetic algorithm factory from a module. Such a module
 *  should have a function named \b CreateFactoryInstance which returns a
 *  a GAFactory object when called.
 *
 *  When creating a module, some code similar to the following can be used:
 *  \code
 * extern "C"
 * {
 *	mogal::GAFactory *CreateFactoryInstance()
 *	{
 *		return new MyGAFactory();
 *	}
 * }
 *  \endcode
 */
class GAModule : public ModuleBase
{
public:
	GAModule();
	~GAModule();

	/** Calls the \c CreateFactoryInstance function which is stored in the
	 *  module and returns the new GAFactory instance.
	 */
	GAFactory *createFactoryInstance() const;
protected:
	void onFoundFunctionName(const std::string &name, void *pFunction);
private:
	typedef GAFactory *(*CreateFactoryInstanceFunction)();
	
	CreateFactoryInstanceFunction m_createFactory;
};
	
} // end namespace

#endif // MOGAL_GAMODULE_H

