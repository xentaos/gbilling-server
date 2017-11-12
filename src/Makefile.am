## Process this file with automake to produce Makefile.in

INCLUDES = $(PACKAGE_CFLAGS)

AM_CFLAGS = -Wall -Wpointer-arith

# -DGTK_DISABLE_DEPRECATED=1 -DG_DISABLE_DEPRECATED=1

bin_PROGRAMS = gbilling-server

gbilling_server_SOURCES = \
	callback.c \
	gbilling.c \
	gui.c \
	main.c \
	server.c \
	sockc.c \
	setting.c \
	sound.c \
	sqlcb.c \
	sqldb.c \
	sqlite3.c

gbilling_server_LDFLAGS = -export-dynamic -ldl

gbilling_server_LDADD = $(PACKAGE_LIBS)
