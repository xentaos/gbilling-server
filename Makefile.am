## Process this file with automake to produce Makefile.in

SUBDIRS = src data ui pixmaps sounds

gbilling_serverdocdir = ${prefix}/doc/gbilling-server
gbilling_serverdoc_DATA = \
        README \
        COPYING \
        AUTHORS \
        ChangeLog \
        INSTALL \
        NEWS \
        TODO

gbilling_servermenudir = ${prefix}/share/applications
gbilling_servermenu_DATA = gbilling-server.desktop

EXTRA_DIST = $(gbilling_serverdoc_DATA) $(gbilling_servermenu_DATA)
