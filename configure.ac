AC_INIT([xmppbroadcast], [0.1])
AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AM_PROG_AR
PKG_INSTALLDIR

LT_INIT

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX([14], [noext])
AX_CHECK_COMPILE_FLAG([-Wall], [CXXFLAGS+=" -Wall"])
AX_CHECK_COMPILE_FLAG([-Werror], [CXXFLAGS+=" -Werror"])
AX_CHECK_COMPILE_FLAG([-pedantic], [CXXFLAGS+=" -pedantic"])

# Windows defines ERROR, which requires us to tell glog to not define
# it as abbreviated log severity (LOG(ERROR) still works, though, and
# that is all that we actually use in the code).
# See https://hpc.nih.gov/development/glog.html.
CXXFLAGS+=" -DGLOG_NO_ABBREVIATED_SEVERITIES"

# Public dependencies exposed in the headers.
AX_PKG_CHECK_MODULES([XAYAUTIL], [libxayautil], [])
AX_PKG_CHECK_MODULES([GAMECHANNEL], [channelcore gamechannel], [])

# Private dependencies for the library parts.
AX_PKG_CHECK_MODULES([JSON], [], [jsoncpp])
AX_PKG_CHECK_MODULES([JSONRPCCPPSERVER], [], [libjsonrpccpp-server])
AX_PKG_CHECK_MODULES([GLOG], [], [libglog])
AX_PKG_CHECK_MODULES([GFLAGS], [], [gflags])
AX_PKG_CHECK_MODULES([CHARON], [], [charon gloox])

# Private dependencies for tests and binaries only.
PKG_CHECK_MODULES([JSONRPCCPPCLIENT], [libjsonrpccpp-client])
PKG_CHECK_MODULES([GTEST], [gmock gtest_main])

# FIXME: We need the Charon installation prefix, since we want to
# access the testenv.pem certificate installed there.  For now, we
# just assume it is the default /usr/local, but ideally we should detect
# it for Charon itself.
AC_SUBST(CHARON_PREFIX, /usr/local)

AC_CONFIG_FILES([
  Makefile \
  src/Makefile
])
AC_OUTPUT

echo
echo "CXXFLAGS: ${CXXFLAGS}"
