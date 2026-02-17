bin_PROGRAMS = ww ww-debug-build

ww_SOURCES = argument.c buffer.c flags.c glconf.c io.c line.c main.c str.c term.c window.c utils.c trie.c
ww_CFLAGS = -O2 -I$(top_srcdir)/src/include
ww_LDADD = 

ww_debug_build_SOURCES = $(ww_SOURCES)
ww_debug_build_CFLAGS = -g -O0 -I$(top_srcdir)/src/include
ww_debug_build_LDADD = 

debug: ww-debug-build
