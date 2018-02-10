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

#ifndef GASERVER_H

#define GASERVER_H

#define GASERVER_MAXPACKSIZE						(128*1024*1024)
#define GASERVER_PACKID							0x5041434B

#define GASERVER_COMMAND_HELPER						1
#define GASERVER_COMMAND_CLIENT						2
#define GASERVER_COMMAND_BUSY						3
#define GASERVER_COMMAND_ACCEPT						4
#define GASERVER_COMMAND_KEEPALIVE					5
#define GASERVER_COMMAND_FACTORY					6
#define GASERVER_COMMAND_RESULT						7
#define GASERVER_COMMAND_NOHELPERS					8
#define GASERVER_COMMAND_CALCULATE					9
#define GASERVER_COMMAND_FITNESS					10
#define GASERVER_COMMAND_FACTORYRESULT					11
#define GASERVER_COMMAND_CURRENTBEST					12
#define GASERVER_COMMAND_GENERATIONINFO					13
#define GASERVER_COMMAND_ISLANDHELPER					14
#define GASERVER_COMMAND_ISLANDFINISHED					15
#define GASERVER_COMMAND_ISLANDSTOP					16
#define GASERVER_COMMAND_MIGRATEDGENOME					17
#define GASERVER_COMMAND_BESTLOCALGENOME				18
#define GASERVER_COMMAND_GENERATIONTIME					19

#endif // GASERVER_H
