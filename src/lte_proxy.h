#/*
# * Licensed to the EPYSYS SCIENCE (EpiSci) under one or more
# * contributor license agreements.
# * The EPYSYS SCIENCE (EpiSci) licenses this file to You under
# * the Episys Science (EpiSci) Public License (Version 1.1) (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy/blob/master/LICENSE
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about EPYSYS SCIENCE (EpiSci):
# *      bo.ryu@episci.com
# */

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

#define BASE_RX_UE_PORT 3211
#define BASE_TX_UE_PORT 3212


class Multi_UE_PNF
{
public:
    Multi_UE_PNF(int id, int num_of_ues, int num_of_enbs, std::string enb_ip, std::string proxy_ip);
    ~Multi_UE_PNF() = default;
    void configure(std::string enb_ip, std::string proxy_ip);
    void start(softmodem_mode_t softmodem_mode);
private:
    std::string vnf_ipaddr;
    std::string pnf_ipaddr;
    int vnf_p5port = -1;
    int vnf_p7port = -1;
    int pnf_p7port = -1;
    int id;
    int num_enbs;
    const int enb_port_delta = 200;
};

class Multi_UE_Proxy
{
public:
    Multi_UE_Proxy(int num_of_ues, std::vector<std::string> enb_ips, std::string proxy_ip);
    ~Multi_UE_Proxy() = default;

    void distribute_ues(int num_of_ues);
    void configure();
    int init_oai_socket(int rx_port, int ue_idx);
    void oai_enb_downlink_nfapi_task(int id, void *msg);
    void testcode_tx_packet_to_UE( int ue_tx_socket_);
    void pack_and_send_downlink_sfn_sf_msg(uint16_t id, uint16_t sfn_sf);
    void receive_message_from_ue(int ue_id);
    void send_ue_to_enb_msg(void *buffer, size_t buflen);
    void send_received_msg_to_proxy_queue(void *buffer, size_t buflen);
    void send_uplink_oai_msg_to_proxy_queue(void *buffer, size_t buflen);
    void start(softmodem_mode_t softmodem_mode);
    std::vector<Multi_UE_PNF> lte_pnfs;
private:
    uint16_t eNB_id[100]; // To identify the destination in uplink
    std::string vnf_ipaddr;
    std::string pnf_ipaddr;
    int vnf_p5port = -1;
    int vnf_p7port = -1;
    int pnf_p7port = -1;

    struct sockaddr_in address_tx_; // temporary struct 
    struct sockaddr_in address_rx_; // temporary struct
    int ue_tx_socket_ = -1;
    int ue_rx_socket_ = -1;
    int ue_rx_socket[100];
    int ue_tx_socket[100];

    typedef struct sfn_sf_info_s
    {
        uint16_t phy_id;
        uint16_t sfn_sf;
    } sfn_sf_info_t;

    std::recursive_mutex mutex;
    using lock_guard_t = std::lock_guard<std::recursive_mutex>;
    std::vector<std::thread> threads;
    bool stop_thread = false;
    const int port_delta = 2;
};
