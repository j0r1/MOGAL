add_definitions(-DMOGAL_COMPILING)

set(HEADERS
	gafactory.h
	gafactorydefaults.h
	gafactorymultiobjective.h
	gafactorysingleobjective.h
	gamodule.h
	gaserver.h
	geneticalgorithm.h
	genome.h 
	modulebase.h
	randomnumbergenerator.h
	doublevectorgenome.h
	doublevectorgafactory.h
	doublevectorgafactoryparams.h
	mpigeneticalgorithm.h
	"${PROJECT_BINARY_DIR}/src/mogalconfig.h" )

set(SOURCES
	gafactorydefaults.cpp
	gafactorymultiobjective.cpp
	gafactorysingleobjective.cpp
	gamodule.cpp
	geneticalgorithm.cpp
	modulebase.cpp 
	randomnumbergenerator.cpp
	doublevectorgafactory.cpp
	doublevectorgenome.cpp
	doublevectorgafactoryparams.cpp 
	mpigeneticalgorithm.cpp )


if (NOT UNIX)
	set(CMAKE_DEBUG_POSTFIX _d)
endif (NOT UNIX)

if (UNIX OR MOGAL_COMPILE_STATIC)
	add_library(mogal-static STATIC ${SOURCES} ${HEADERS})
	set_target_properties(mogal-static PROPERTIES OUTPUT_NAME mogal)
	set_target_properties(mogal-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
	set(MOGAL_INSTALLTARGETS mogal-static)
	target_link_libraries(mogal-static ${MOGAL_LINK_LIBS})
endif (UNIX OR MOGAL_COMPILE_STATIC)

if (UNIX OR NOT MOGAL_COMPILE_STATIC)
	add_library(mogal-shared SHARED ${SOURCES} ${HEADERS})
	set_target_properties(mogal-shared PROPERTIES OUTPUT_NAME mogal)
	set_target_properties(mogal-shared PROPERTIES VERSION ${VERSION})
	set_target_properties(mogal-shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
	set(MOGAL_INSTALLTARGETS ${MOGAL_INSTALLTARGETS} mogal-shared)
	target_link_libraries(mogal-shared ${MOGAL_LINK_LIBS})
endif (UNIX OR NOT MOGAL_COMPILE_STATIC)

apply_include_paths("${MOGAL_INTERNAL_INCLUDES}")
apply_include_paths("${MOGAL_EXTERNAL_INCLUDES}")

install(FILES ${HEADERS} DESTINATION include/mogal)
install(TARGETS ${MOGAL_INSTALLTARGETS} DESTINATION ${LIBRARY_INSTALL_DIR})

