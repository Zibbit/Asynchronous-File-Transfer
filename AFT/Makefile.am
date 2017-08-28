MAINTAINERCLEANFILES = \
	Makefile.in

htmldatadir = $(datadir)/AFT

sbin_PROGRAMS = \
	AFTd

bin_PROGRAMS = \
	AFT

common_CFLAGS = \
	-Wall \
	$(EVD_CFLAGS) \
	$(JSON_CFLAGS) \
	-DENABLE_TESTS="\"$(enable_tests)\""

if ENABLE_DEBUG
common_CFLAGS += -Werror -g3 -O0 -ggdb
else
common_CFLAGS += -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS
endif

common_LDADD = \
	$(EVD_LIBS) \
	$(JSON_LIBS) \
	-lgcrypt

common_source_c = \
	AFT-protocol.c \
	AFT-source.c \
	AFT-transfer.c

common_source_h = \
	AFT-protocol.h \
	AFT-source.h \
	AFT-transfer.h

# AFT server daemon
AFTd_CFLAGS = $(common_CFLAGS) \
	-DHTML_DATA_DIR="\"$(htmldatadir)\""

AFTd_LDADD = $(common_LDADD)

AFTd_SOURCES = \
	AFTd-main.c \
	$(common_source_c) \
	AFT-web-service.c \
	AFT-node.c \
	$(common_source_h) \
	AFT-web-service.h \
	AFT-node.h

# AFT client
AFT_CFLAGS = $(common_CFLAGS)

AFT_LDADD = $(common_LDADD)

AFT_SOURCES = \
	AFT-main.c \
	$(common_source_c) \
	$(common_source_h)
