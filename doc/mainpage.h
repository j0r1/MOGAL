/** \mainpage MOGAL
 *
 * \section intro Introduction
 *
 * 	MOGAL is a Multi-Objective Genetic Algorithm Library. It provides a 
 * 	framework for writing genetic algorithms which can be run in a
 * 	distributed way. Furthermore, these genetic algorithms can be
 * 	multi-objective, meaning that several fitness measures can be
 * 	optimized at the same time.
 *
 * \section license License
 *
 * 	The license which applies to this library is the LGPL. You can find the
 * 	full version in the file \c LICENSE.LGPL which is included in the library
 * 	archive. The short version is the following:
 *
 *	\code
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  
 * USA.
 *	\endcode
 *
 * \section usage Usage
 *
 * 	Starting a genetic algorithm, both locally and in a distributed way, is
 * 	done using an instance of the mogal::GeneticAlgorithm instance. One of
 * 	the overloaded versions of mogal::GeneticAlgorithm::run will start the
 * 	optimization, and when it is finished, the same instance will store the
 * 	best solution(s).
 * 	This class implements the core structure of a genetic algorithm: start
 * 	with a random population and keep producing new populations until a
 * 	stopping criterion is met. The inner workings of a specific genetic
 * 	algorithm must be implemented by deriving your own class from
 * 	mogal::GAFactory. The classes mogal::GAFactorySingleObjective and
 * 	mogal::GAFactoryMultiObjective are derived from the mogal::GAFactory
 * 	class, and will make it easier to start your own genetic algorithm.
 *
 * 	The script <tt><b>createGATemplate.sh</b></tt> which is included in this package can
 * 	be used to create a framework for your genetic algorithm. When this is
 * 	run, it will produce an overview of the functions which will need to
 * 	be implemented. Note that if you're only going to run the genetic algorithm
 * 	locally, many of these functions will not need a decent implementation.
 * 	On the other hand, if distributed calculation is desired, some additional
 * 	code will need to be written to serialize the information in a genome.
 * 
 * \section examples Examples
 *
 *	The <tt><b>examples</b></tt> subdirectory contained in the source code
 *	package contains some examples which can help you to get started.
 * 
 * 	\subsection single Single-objective example
 *
 *		The file <tt><b>single.cpp</b></tt> illustrates how a simple,
 *		single-objective genetic algorithm can be implemented using
 *		the library. Since no code is provided to serialize the genome
 *		information, this example can't be run in a distributed way.
 *
 *		To compile the example:
 *		\code
 * g++ -o single single.cpp `pkg-config --cflags --libs mogal`
 *		\endcode
 * 
 * 	\subsection multi Multi-objective example
 *
 * 		The file <tt><b>multi.cpp</b></tt> shows how a multi-objective
 * 		genetic algorithm can be implemented with the help of the library.
 * 		Again, no code is provided for distributed calculation of the
 * 		genome fitness values.
 *
 * 		To compile this example:
 * 		\code
 * g++ -o multi multi.cpp `pkg-config --cflags --libs mogal`
 * 		\endcode
 *
 * 	\subsection dist Distributed calculation example
 *
 * 		The files <tt><b>distmain.cpp</b></tt>, <tt><b>distmodule.cpp</tt></b>,
 * 		<tt><b>distparams.cpp</tt></b> and <tt><b>distparams.h</tt></b> are
 * 		part of an example which illustrates how the genome fitness calculation
 * 		can be distributed.
 *
 * 		To compile the main program:
 * 		\code
 * g++ -o distmain distmain.cpp distparams.cpp `pkg-config --cflags --libs mogal`
 * 		\endcode
 * 		To compile the module:
 * 		\code
 * g++ -shared -Wl,-soname,distmodule.so -o distmodule.so distmodule.cpp distparams.cpp `pkg-config --cflags --libs mogal`
 * 		\endcode
 *
 * 		To make the program work, you'll have to do two additional things:
 * 		- start a 'gaserver' instance on port 22000 and use the path to distmodule.so
 * 		  as its module directory.
 * 		- start at least one 'gahelper' instance that connects to the
 * 		  'gaserver' instance, and make sure that distmodule.so can be found
 * 		  in its module path as well.
 *
 * 		Note that because this example is very simple, the communication
 * 		overhead that is introduced in distributing the fitness calculation
 * 		actually makes it slower than when the calculation would not be
 * 		distributed.
 * 
 * \section contact Contact
 *
 * 	If you have any questions, remarks or requests, you can contact me at
 * 	\c jori(\c dot)\c liesenborgs(\c at)\c gmail(\c dot)\c com
 *
 */

