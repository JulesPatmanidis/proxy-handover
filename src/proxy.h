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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum softmodem_mode_t
{
    SOFTMODEM_LTE,
    SOFTMODEM_NR,
    SOFTMODEM_NSA,
    SOFTMODEM_LTE_HANDOVER,
    SOFTMODEM_LTE_HANDOVER_N_ENB
} softmodem_mode_t;

struct oai_task_args
{
 softmodem_mode_t softmodem_mode;
 int node_id;
 int num_enbs;
};

#ifdef __cplusplus
}
#endif
