bin_PROGRAMS = xmpp-broadcast-rpc-server
lib_LTLIBRARIES = libxmppbroadcast.la

libxmppbroadcast_la_CXXFLAGS = \
  $(XAYAUTIL_CFLAGS) $(GAMECHANNEL_CFLAGS) $(CHARON_CFLAGS) \
  $(JSON_CFLAGS) $(JSONRPCCPPSERVER_CFLAGS) \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS)
libxmppbroadcast_la_LIBADD = \
  $(XAYAUTIL_LIBS) $(GAMECHANNEL_LIBS) $(CHARON_LIBS) \
  $(JSON_LIBS) $(JSONRPCCPPSERVER_LIBS) \
  $(GLOG_LIBS) $(GFLAGS_LIBS)
libxmppbroadcast_la_SOURCES = \
  mucclient.cpp \
  stanzas.cpp
noinst_HEADERS = \
  private/mucclient.hpp private/mucclient.tpp \
  private/stanzas.hpp

xmpp_broadcast_rpc_server_CXXFLAGS = \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS)
xmpp_broadcast_rpc_server_LDADD = \
  $(builddir)/libxmppbroadcast.la \
  $(GLOG_LIBS) $(GFLAGS_LIBS)
xmpp_broadcast_rpc_server_SOURCES = main.cpp

check_PROGRAMS = tests
TESTS = tests

tests_CXXFLAGS = \
  $(XAYAUTIL_CFLAGS) $(CHARON_CFLAGS) \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS) $(GTEST_CFLAGS)
tests_LDADD = \
  $(builddir)/libxmppbroadcast.la \
  $(XAYAUTIL_LIBS) $(CHARON_LIBS) \
  $(GLOG_LIBS) $(GFLAGS_LIBS) $(GTEST_LIBS)
tests_SOURCES = \
  testutils.cpp \
  \
  mucclient_tests.cpp \
  stanzas_tests.cpp
check_HEADERS = \
  testutils.hpp
