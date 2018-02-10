macro(support_option DESCRIPTION OPTIONNAME DEFINENAME DEFAULTVALUE EMPTYVALUE)
	option(${OPTIONNAME} ${DESCRIPTION} ${DEFAULTVALUE})
	if (${OPTIONNAME})
		set(${DEFINENAME} "#define ${DEFINENAME}")
	else (${OPTIONNAME})
		set(${DEFINENAME} "${EMPTYVALUE}")
	endif (${OPTIONNAME})
endmacro(support_option)

macro(apply_include_paths VARNAME)
	set (BLA "${VARNAME}")
	foreach(i IN LISTS BLA)
		set (BLA2 "${i}")
		if (BLA2)
			include_directories("${i}")
		endif (BLA2)
	endforeach(i)
endmacro(apply_include_paths)

macro(remove_empty VARNAME)
	set (remove_empty_NEWLIST "")
	foreach(i IN LISTS ${VARNAME})
		set (BLA2 "${i}")
		if (BLA2)
			list(APPEND remove_empty_NEWLIST "${i}")
		endif (BLA2)
	endforeach(i)
	set(${VARNAME} "${remove_empty_NEWLIST}")
endmacro(remove_empty)

macro(get_install_directory DSTVAR)
	set (_DEFAULT_LIBRARY_INSTALL_DIR lib)
	if (EXISTS "${CMAKE_INSTALL_PREFIX}/lib32/" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
		set (_DEFAULT_LIBRARY_INSTALL_DIR lib32)
	elseif (EXISTS "${CMAKE_INSTALL_PREFIX}/lib64/" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		set (_DEFAULT_LIBRARY_INSTALL_DIR lib64)
	endif ()

	set(${DSTVAR} "${_DEFAULT_LIBRARY_INSTALL_DIR}" CACHE PATH "Library installation directory")
	if(NOT IS_ABSOLUTE "${${DSTVAR}}")
		set(${DSTVAR} "${CMAKE_INSTALL_PREFIX}/${${DSTVAR}}")
	endif()
endmacro(get_install_directory)

macro(add_additional_stuff INCVAR LIBVAR)
	set(ADDITIONAL_INCLUDE_DIRS "" CACHE STRING "Additional include directories")
	if (UNIX)
		set(ADDITIONAL_LIBRARIES "" CACHE STRING "Additional libraries to link against")
	else (UNIX)
		set(ADDITIONAL_GENERAL_LIBRARIES "" CACHE STRING "Additional libraries to link against in both debug and release modes")
		set(ADDITIONAL_RELEASE_LIBRARIES "" CACHE STRING "Additional libraries to link against in release mode")
		set(ADDITIONAL_DEBUG_LIBRARIES "" CACHE STRING "Additional libraries to link against in debug mode")

		set(ADDITIONAL_LIBRARIES "${ADDITIONAL_GENERAL_LIBRARIES}")

		foreach(l IN LISTS ADDITIONAL_RELEASE_LIBRARIES)
			list(APPEND ADDITIONAL_LIBRARIES optimized)
			list(APPEND ADDITIONAL_LIBRARIES "${l}")
		endforeach(l)
		foreach(l IN LISTS ADDITIONAL_DEBUG_LIBRARIES)
			list(APPEND ADDITIONAL_LIBRARIES debug)
			list(APPEND ADDITIONAL_LIBRARIES "${l}")
		endforeach(l)
	endif (UNIX)
	list(APPEND ${INCVAR} "${ADDITIONAL_INCLUDE_DIRS}")
	list(APPEND ${LIBVAR} "${ADDITIONAL_LIBRARIES}")
endmacro(add_additional_stuff)

