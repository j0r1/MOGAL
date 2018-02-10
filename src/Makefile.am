lib_LTLIBRARIES = libmogal.la
bin_PROGRAMS = gaserver gahelper
dist_bin_SCRIPTS = createGATemplate.sh

libmogal_la_SOURCES = modulebase.cpp gamodule.cpp geneticalgorithm.cpp gafactorydefaults.cpp \
		      gafactorysingleobjective.cpp gafactorymultiobjective.cpp
libmogal_la_LDFLAGS = -release @VERSION@

libmogalincludedir = ${includedir}/mogal
libmogalinclude_HEADERS = modulebase.h gamodule.h genome.h geneticalgorithm.h gafactory.h \
			  gafactorydefaults.h gafactorysingleobjective.h gafactorymultiobjective.h

gaserver_SOURCES = gaserver.cpp gaserver.h log.cpp log.h
gaserver_DEPENDENCIES = libmogal.la
gaserver_LDADD = -lenut -lenet -lserut -ldl .libs/libmogal.a

gahelper_SOURCES = gahelper.cpp log.cpp log.h
gahelper_DEPENDENCIES = libmogal.la
gahelper_LDADD = -lenut -lenet -lserut -ldl .libs/libmogal.a
