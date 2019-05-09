if(${ARCH} STREQUAL "x86_64")
  set(OPENSSL_MODULES
    openssl/init.cpp
    openssl/client.cpp
    openssl/server.cpp
    openssl/tls_stream.cpp
    https/openssl_server.cpp
    http/client.cpp
    https/s2n_server.cpp
  )
  #TODO get from conan
  set(OPENSSL_LIBS
    s2n_libs2n
    s2n_libssl
    s2n_crypto
  )
endif()

set(BOTAN_MODULES
  https/botan_server.cpp
)

SET(ETH_SRCS
     ethernet/ethernet.cpp
     ethernet/ethernet_8021q.cpp
     )


SET(IP4_SRCS
    ip4/arp.cpp
    ip4/ip4.cpp
    ip4/reassembly.cpp
    ip4/icmp4.cpp
    )



SET(IP6_SRCS
  ip6/addr.cpp
  ip6/addr_list.cpp
  ip6/ip6.cpp
  ip6/icmp6.cpp
  ip6/ndp.cpp
  ip6/mld.cpp
  ip6/extension_header.cpp
  #net/ip6/packet_ndp.cpp
  #net/ip6/packet_mld.cpp
  ip6/slaac.cpp
  )


SET(TCP_SRCS
    tcp/tcp.cpp
    tcp/connection.cpp
    tcp/connection_states.cpp
    tcp/write_queue.cpp
    tcp/rttm.cpp
    tcp/listener.cpp
    tcp/read_buffer.cpp
    tcp/read_request.cpp
    tcp/stream.cpp
    tcp/tcp_conntrack.cpp
    )
SET(UDP_SRCS
    udp/udp.cpp
    udp/socket.cpp
  )

set(HTTP_SRCS
    http/header.cpp
    http/header_fields.cpp
    http/message.cpp
    http/request.cpp
    http/response.cpp
    http/status_codes.cpp
    http/time.cpp
    http/version.cpp
    http/mime_types.cpp
    http/cookie.cpp
    http/client_connection.cpp
    http/basic_client.cpp
    http/server_connection.cpp
    http/server.cpp
    http/response_writer.cpp
    )


SET(DHCP_SRCS
    dhcp/dh4client.cpp
    dhcp/dhcpd.cpp
    )

set(DNS_SRCS
    dns/dns.cpp
    dns/client.cpp
    dns/record.cpp
    dns/response.cpp
    dns/query.cpp
    )


set(NAT_SRCS
    nat/nat.cpp
    nat/napt.cpp
    )

set(SRCS
    checksum.cpp
    buffer_store.cpp
    inet.cpp
    interfaces.cpp
    packet_debug.cpp
    conntrack.cpp
    vlan_manager.cpp
    addr.cpp
    ws/websocket.cpp
)

#TODO figure out if cmake can do multilevel objects somehow
#or maybe calling this will return a list of objects..
SET(OBJLIST
        ${NET_SRCS}
        ${ETH_SRCS}
        ${IP4_SRCS}
        ${IP6_SRCS}
        ${TCP_SRCS}
        ${UDP_SRCS}
        ${NAT_SRCS}
        ${DNS_SRCS}
        ${DHCP_SRCS}
    )
#TODO what else can we strip away from net?
if (NOT ${PLATFORM} STREQUAL "nano")
  list(APPEND OBJLIST ${HTTP_SRCS})
  if (NOT CMAKE_TESTING_ENABLED)
    list(APPEND OBJLIST
      ${BOTAN_MODULES}
      ${OPENSSL_MODULES}
    )
    list(APPEND SRCS
      configure.cpp
    )
  endif()
endif()


add_library(net OBJECT ${SRCS} ${OBJLIST})
