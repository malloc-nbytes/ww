AC_INIT([ww], [1.0], [zdhdev@yahoo.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([src/include/config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
])

AC_DEFINE_UNQUOTED([COMPILER_NAME], ["$CC"], [Name of the C compiler])
AC_DEFINE_UNQUOTED([COMPILER_VERSION], ["`$CC --version | head -n1`"], [Version of the C compiler])
AC_DEFINE_UNQUOTED([COMPILER_PATH], ["`which $CC`"], [Full path to the C compiler])
AC_DEFINE_UNQUOTED([PREFIX], ["$prefix"], [Installation prefix directory])
AC_DEFINE_UNQUOTED([LIBDIR], ["$libdir"], [Library installation directory])

AC_OUTPUT
