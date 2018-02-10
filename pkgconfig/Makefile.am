### all of the standard pc files we need to generate
pcfiles = 							\
	mogal.pc

pcfiles_uninstalled =						\
	mogal-uninstalled.pc

all-local: $(pcfiles) $(pcfiles_uninstalled)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)

EXTRA_DIST =							\
	mogal.pc.in						\
	mogal-uninstalled.pc.in

CLEANFILES = $(pcfiles) $(pcfiles_uninstalled)
