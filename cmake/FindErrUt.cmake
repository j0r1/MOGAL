

find_package(ErrUt QUIET NO_MODULE)

if (NOT ERRUT_FOUND)

	find_path(ERRUT_INCLUDE_DIR NAMES errut/errorbase.h PATH_SUFFIXES include)

	include(FindPackageHandleStandardArgs)

	find_package_handle_standard_args(ErrUt DEFAULT_MSG ERRUT_INCLUDE_DIR)

	set(ERRUT_INCLUDE_DIRS ${ERRUT_INCLUDE_DIR})
	set(ERRUT_LIBRARIES "")

endif ()
