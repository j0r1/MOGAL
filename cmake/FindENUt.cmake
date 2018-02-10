
message(STATUS "Checking if ENUt cmake config file can be found")

find_package(ENUt QUIET NO_MODULE)

if (NOT ENUT_FOUND) # Config file could not be found
	find_path(ENUT_INCLUDE_DIR enut/nutconfig.h)
	
	set(ENUT_INCLUDE_DIRS ${ENUT_INCLUDE_DIR})

	if (UNIX)
		find_library(ENUT_LIBRARY enut)
		if (ENUT_LIBRARY)
			set(ENUT_LIBRARIES ${ENUT_LIBRARY})
		endif (ENUT_LIBRARY)
	else (UNIX)
		find_library(ENUT_LIB_RELEASE enut)
		find_library(ENUT_LIB_DEBUG enut_d)

		if (ENUT_LIB_RELEASE OR ENUT_LIB_DEBUG)
			set(ENUT_LIBRARIES "")
			if (ENUT_LIB_RELEASE)
				set(ENUT_LIBRARIES ${ENUT_LIBRARIES} optimized ${ENUT_LIB_RELEASE})
			endif (ENUT_LIB_RELEASE)
			if (ENUT_LIB_DEBUG)
				set(ENUT_LIBRARIES ${ENUT_LIBRARIES} debug ${ENUT_LIB_DEBUG})
			endif (ENUT_LIB_DEBUG)
		endif (ENUT_LIB_RELEASE OR ENUT_LIB_DEBUG)
	endif(UNIX)
endif (NOT ENUT_FOUND)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(ENUt DEFAULT_MSG ENUT_INCLUDE_DIRS ENUT_LIBRARIES)


