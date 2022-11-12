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

#include <sys/stat.h>
#include <sstream>
#include "lte_proxy.h"
#include "nfapi_pnf.h"

namespace
{
    Multi_UE_Proxy *instance;
}

void print_socket_info(struct sockaddr_in socket) {
        char s[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(socket.sin_addr), s, INET_ADDRSTRLEN);
        //unsigned int p = ntohs(&(((struct sockaddr_in *)socket)->sin_port));
        printf("printing socket info...\n");
        printf("address: %s, port: %d\n", s, ntohs(socket.sin_port));
}

Multi_UE_PNF::Multi_UE_PNF(int pnf_id, int num_of_ues, std::string enb_ip, std::string proxy_ip)
{
    num_ues = num_of_ues ;
    id = pnf_id;

    configure(enb_ip, proxy_ip);

    oai_subframe_init(pnf_id);
}

void Multi_UE_PNF::configure(std::string enb_ip, std::string proxy_ip)
{
    vnf_ipaddr = enb_ip;
    pnf_ipaddr = proxy_ip;
    std::cout<<"VNF is on IP Address "<<vnf_ipaddr<<std::endl;
    std::cout<<"PNF is on IP Address "<<pnf_ipaddr<<std::endl;
}

void Multi_UE_PNF::start(softmodem_mode_t softmodem_mode)
{
    pthread_t thread;
    vnf_p5port = 50001 + id * enb_port_delta;
    vnf_p7port = 50011 + id * enb_port_delta;
    pnf_p7port = 50010 + id * enb_port_delta;

    struct oai_task_args args {softmodem_mode, id};

    configure_nfapi_pnf(id, vnf_ipaddr.c_str(), vnf_p5port, pnf_ipaddr.c_str(), pnf_p7port, vnf_p7port);

    if (pthread_create(&thread, NULL, &oai_subframe_task, (void *)&args) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_create failed for calling oai_subframe_task");
    }
}

Multi_UE_Proxy::Multi_UE_Proxy(int num_of_ues, std::vector<std::string> enb_ips, std::string proxy_ip, std::string ue_ip)
{
    assert(instance == NULL);
    instance = this;
    for (int ue_idx = 0; ue_idx < num_of_ues; ue_idx++)
    {
        eNB_id[ue_idx] = 0;
    }

    num_ues = num_of_ues;
    int num_of_enbs = enb_ips.size();

    for (int i = 0; i < num_of_enbs; i++)
    {
        lte_pnfs.push_back(Multi_UE_PNF(i, num_of_ues, enb_ips[i], proxy_ip));
    }
    configure(ue_ip);
}

void Multi_UE_Proxy::configure(std::string ue_ip)
{
    oai_ue_ipaddr = ue_ip;
    std::cout << "OAI-UE is on IP Address " << oai_ue_ipaddr << std::endl;

    for (int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        int oai_rx_ue_port = 3211 + ue_idx * port_delta;
        int oai_tx_ue_port = 3212 + ue_idx * port_delta;
        init_oai_socket(oai_ue_ipaddr.c_str(), oai_tx_ue_port, oai_rx_ue_port, ue_idx);
    }
}

void Multi_UE_Proxy::start(softmodem_mode_t softmodem_mode)
{
    int num_lte_pnfs = lte_pnfs.size();

    for (int i = 0; i < num_lte_pnfs; i++)
    {
        lte_pnfs[i].start(softmodem_mode);
        sleep(1);
    }

    for (int i = 0; i < num_ues; i++)
    {
        threads.push_back(std::thread(&Multi_UE_Proxy::receive_message_from_ue, this, i));
    }
    for (auto &th : threads)
    {
        if(th.joinable())
        {
            th.join();
        }
    }
}

/**
 * @brief Setup Rx/Tx sockets for communication with the given UE
 * 
 * @param addr The ip of the oai UE
 * @param tx_port The transmit port
 * @param rx_port The receive port
 * @param ue_idx The index of the UE
 * @return 0 for successful setup, -1 if errors occured
 */
int Multi_UE_Proxy::init_oai_socket(const char *addr, int tx_port, int rx_port, int ue_idx)
{
     {   //Setup Rx Socket
        printf("Setting up rx socket\n");
        memset(&address_rx_, 0, sizeof(address_rx_));
        address_rx_.sin_family = AF_INET;
        address_rx_.sin_addr.s_addr = INADDR_ANY;
        address_rx_.sin_port = htons(rx_port);
        ue_rx_socket_ = socket(address_rx_.sin_family, SOCK_DGRAM, 0);
        ue_rx_socket[ue_idx] = ue_rx_socket_;

        if (ue_rx_socket_ < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
            return -1;
        }
        if (bind(ue_rx_socket_, (struct sockaddr *)&address_rx_, sizeof(address_rx_)) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "bind failed in init_oai_socket: %s\n", strerror(errno));
            close(ue_rx_socket_);
            ue_rx_socket_ = -1;
            return -1;
        }
        printf("ignore this print %s, %d\n", addr, tx_port);
    }
    return 0;
}

/**
 * @brief Listens to the port of the given UE for messages and calls the handler funtion
 * 
 * @param ue_idx The index of the UE
 */
void Multi_UE_Proxy::receive_message_from_ue(int ue_idx)
{
    // Setup tx socket first
    {
        int tmp_sock;
        int len;
        struct sockaddr_in ue_discovered_addr;
        struct sockaddr_in send_bind_addr;
        socklen_t addr_len = sizeof(ue_discovered_addr);
        uint8_t buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];

        printf("Setting up downlink socket\n");

        /* Create tx socket */
        if ((tmp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            printf("Error creating downlink socket for UE %d: %s", ue_idx, strerror(errno));
            return;
        }

        send_bind_addr.sin_family = AF_INET;
        send_bind_addr.sin_addr.s_addr = INADDR_ANY;

        /* Tx port formula: 3212 + ue_idx * port_delta; */
        send_bind_addr.sin_port = htons(3212 + ue_idx * port_delta);
        
        /* Bind */
        if (bind(tmp_sock, (struct sockaddr *)&send_bind_addr, sizeof(struct sockaddr)) == -1)
        {
            printf("Error binding downlink socket for UE %d: %s", ue_idx, strerror(errno));
            return ;
        }
        /* Receive the discovery packet on the tx socket and store the UE address */
        printf("Waiting for discovery message\n");
        len = recvfrom(tmp_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&ue_discovered_addr, &addr_len);
        if (len == -1)
        {
            printf("Recv failure (%d): %s", errno, strerror(errno));
            return;
        }
        printf("Got discovery message!\n");
        /* Connect socket to the UE address of the packet received (ue_discovered_addr). */
        if (connect(tmp_sock, (struct sockaddr *)&ue_discovered_addr, addr_len) < 0)
        {
            printf("Error connecting downlink socket for UE %d: %s", ue_idx, strerror(errno));
            return;
        }

        /* Save the tx socket for the ue in the proxy */
        ue_tx_socket[ue_idx] = tmp_sock;
    }

    // Receive messages from ues
    printf("Receive messages from ue: %d\n", ue_idx);
    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
    socklen_t addr_len = sizeof(address_rx_);
    int print_count = 0;
    while(true)
    {
        //NFAPI_TRACE(NFAPI_TRACE_INFO, "(Proxy) Receive from: ue_idx: %d, address: %s", ue_idx, (sockaddr *)&address_rx_); // Error string to sockaddr
        int buflen = recvfrom(ue_rx_socket[ue_idx], buffer, sizeof(buffer), 0, (sockaddr *)&address_rx_, &addr_len);
        if (buflen == -1)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Recvfrom failed %s", strerror(errno));
            return ;
        }
        if (print_count % 1000 == 0) {
            printf("Got message %d from ue %d\n", print_count, ue_idx);
        }
        print_count++;
        if (buflen == 4)
        {
            //NFAPI_TRACE(NFAPI_TRACE_INFO , "Dummy frame");
            continue;

        }
        else
        {
            nfapi_p7_message_header_t header;
            if (nfapi_p7_message_header_unpack(buffer, buflen, &header, sizeof(header), NULL) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Header unpack failed for standalone pnf");
                return ;
            }
            uint16_t sfn_sf = nfapi_get_sfnsf(buffer, buflen);
            eNB_id[ue_idx] = header.phy_id;
            NFAPI_TRACE(NFAPI_TRACE_INFO , "(Proxy) Proxy has received %d uplink message from OAI UE at socket. Frame: %d, Subframe: %d",
                    header.message_id, NFAPI_SFNSF2SFN(sfn_sf), NFAPI_SFNSF2SF(sfn_sf));
        }
        oai_subframe_handle_msg_from_ue(eNB_id[ue_idx], buffer, buflen, ue_idx + 2);
    }
}

void Multi_UE_Proxy::oai_enb_downlink_nfapi_task(int id, void *msg_org)
{
    lock_guard_t lock(mutex);

    nfapi_p7_message_header_t *pHeader = (nfapi_p7_message_header_t *)msg_org;

    if (msg_org == NULL) {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack supplied pointers are null\n");
        return;
    }
    pHeader->phy_id = id;

    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
    int encoded_size = nfapi_p7_message_pack(msg_org, buffer, sizeof(buffer), nullptr);
    if (encoded_size <= 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message pack failed");
        return;
    }

    union
    {
        nfapi_p7_message_header_t header;
        nfapi_dl_config_request_t dl_config_req;
        nfapi_tx_request_t tx_req;
        nfapi_hi_dci0_request_t hi_dci0_req;
        nfapi_ul_config_request_t ul_config_req;
    } msg;

    if (nfapi_p7_message_unpack((void *)buffer, encoded_size, &msg, sizeof(msg), NULL) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p7_message_unpack failed NEM ID: %d", 1);
        return;
    }

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        if (id != eNB_id[ue_idx]) {
            continue;
        }
        address_tx_.sin_port = htons(3212 + ue_idx * port_delta); // Make that a function argument?

	    if (ue_tx_socket[ue_idx] < 2)
	    {
	        printf("Ue tx socket not initialized yet");
	        continue;
	    }
	    //printf("Sending to ue %d\n", ue_idx);
	    if (send(ue_tx_socket[ue_idx], buffer, encoded_size, 0) < 0)
	    {
	        printf("error sending message to ue");
	    }
    }
}

void Multi_UE_Proxy::pack_and_send_downlink_sfn_sf_msg(uint16_t id, uint16_t sfn_sf)
{
    lock_guard_t lock(mutex);

    // sfn_sf_info_t sfn_sf_info;
    // sfn_sf_info.phy_id = id;
    // sfn_sf_info.sfn_sf = sfn_sf;

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        if (ue_tx_socket[ue_idx] < 2)
        {
            //printf("Ue tx socket not initialized yet (sfn_sf)");
            continue;
        }
        //printf("Sending sfn_sf to ue %d\n", ue_idx);
        if (send(ue_tx_socket[ue_idx], &sfn_sf, sizeof(sfn_sf), 0) < 0)
        {
            printf("(Proxy) Send sfn_sf_tx to OAI UE FAIL Frame: %d,Subframe: %d, ENB: %d\n", NFAPI_SFNSF2SFN(sfn_sf), NFAPI_SFNSF2SF(sfn_sf), id);
        }
    }
}

void transfer_downstream_nfapi_msg_to_proxy(uint16_t id, void *msg)
{
    instance->oai_enb_downlink_nfapi_task(id, msg);
}

/**
 * @brief Sends a downling sfn_sf msg to the UEs
 * 
 * @param id EnB ID
 * @param sfn_sf Current sfn_sf
 */
void transfer_downstream_sfn_sf_to_proxy(uint16_t id, uint16_t sfn_sf)
{
    instance->pack_and_send_downlink_sfn_sf_msg(id, sfn_sf);
}
