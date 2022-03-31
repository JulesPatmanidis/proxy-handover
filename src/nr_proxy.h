#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string>
#include <cstring>
#include <mutex>
#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include "proxy.h"

class Multi_UE_NR_Proxy
{
public:
    Multi_UE_NR_Proxy(int num_of_ues, std::string gnb_ip, std::string proxy_ip, std::string ue_ip);
    ~Multi_UE_NR_Proxy() = default;
    void configure(std::string gnb_ip, std::string proxy_ip, std::string ue_ip);
    int init_oai_socket(const char *addr, int tx_port, int rx_port, int ue_idx);
    void oai_gnb_downlink_nfapi_task(void *msg);
    void testcode_tx_packet_to_UE( int ue_tx_socket_);
    void pack_and_send_downlink_sfn_slot_msg(uint16_t sfn_slot);
    void receive_message_from_nr_ue(int ue_id);
    void send_nr_ue_to_gnb_msg(void *buffer, size_t buflen);
    void send_received_msg_to_proxy_queue(void *buffer, size_t buflen);
    void send_uplink_oai_msg_to_proxy_queue(void *buffer, size_t buflen);
    void start(softmodem_mode_t softmodem_mode);
private:
    std::string oai_ue_ipaddr;
    std::string vnf_ipaddr;
    std::string pnf_ipaddr;
    int vnf_p5port = -1;
    int vnf_p7port = -1;
    int pnf_p7port = -1;

    struct sockaddr_in address_tx_;
    struct sockaddr_in address_rx_;
    int ue_tx_socket_ = -1;
    int ue_rx_socket_ = -1;
    int ue_rx_socket[100];
    int ue_tx_socket[100];
    std::uint16_t id ;
    std::recursive_mutex mutex;
    using lock_guard_t = std::lock_guard<std::recursive_mutex>;
    std::vector<std::thread> threads;
    bool stop_thread = false;
    int port_delta = 2;
};
