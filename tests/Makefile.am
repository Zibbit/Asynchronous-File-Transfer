MAINTAINERCLEANFILES = \
	Makefile.in

tests_dir = `pwd`/
src_dir = $(top_srcdir)/AFT
htmldatadir = $(datadir)/AFT

AM_CFLAGS = \
	-Wall \
	$(EVD_CFLAGS) \
	$(JSON_CFLAGS) \
	-DENABLE_TESTS="\"$(enable_tests)\"" \
	-DTESTS_DIR="\"$(tests_dir)\"" \
	-DHTML_DATA_DIR="\"$(htmldatadir)\"" \
	-I$(top_srcdir)/AFT

if ENABLE_DEBUG
AM_CFLAGS += -Werror -g3 -O0 -ggdb
else
AM_CFLAGS += -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS
endif

AM_LIBS = \
	$(EVD_LIBS) \
	$(JSON_LIBS)

if ENABLE_TESTS

noinst_PROGRAMS = \
	test-protocol \
	test-node-sources

TESTS = \
	test-protocol \
	test-node-sources

# test-protocol
test_protocol_CFLAGS = $(AM_CFLAGS)
test_protocol_LDADD = $(AM_LIBS)
test_protocol_SOURCES = \
	../AFT/AFT-source.c \
	../AFT/AFT-transfer.c \
	../AFT/AFT-protocol.c \
	test-protocol.c

# test-node-sources
test_node_sources_CFLAGS = $(AM_CFLAGS)
test_node_sources_LDADD = $(AM_LIBS)
test_node_sources_SOURCES = \
	$(src_dir)/AFT-source.c \
	$(src_dir)/AFT-protocol.c \
	$(src_dir)/AFT-web-service.c \
	$(src_dir)/AFT-transfer.c \
	$(src_dir)/AFT-node.c \
	test-node-sources.c

endif # ENABLE_TESTS

EXTRA_DIST =
