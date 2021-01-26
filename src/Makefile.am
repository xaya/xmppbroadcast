bin_PROGRAMS = xmpp-broadcast-rpc-server
lib_LTLIBRARIES = libxmppbroadcast.la

libxmppbroadcast_la_CXXFLAGS = \
  $(GAMECHANNEL_CFLAGS) $(CHARON_CFLAGS) \
  $(JSON_CFLAGS) $(JSONRPCCPPSERVER_CFLAGS) \
  $(GLOG_CFLAGS)
libxmppbroadcast_la_LIBADD = \
  $(GAMECHANNEL_LIBS) $(CHARON_LIBS) \
  $(JSON_LIBS) $(JSONRPCCPPSERVER_LIBS) \
  $(GLOG_LIBS)
libxmppbroadcast_la_SOURCES = \
  stanzas.cpp
noinst_HEADERS = \
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
  $(CHARON_CFLAGS) \
  $(GLOG_CFLAGS) $(GTEST_CFLAGS)
tests_LDADD = \
  $(builddir)/libxmppbroadcast.la \
  $(CHARON_LIBS) \
  $(GLOG_LIBS) $(GTEST_LIBS)
tests_SOURCES = \
  stanzas_tests.cpp
