/*********************************************************************************************************************
Copyright (c) 2020 RoboSense
All rights reserved

By downloading, copying, installing or using the software you agree to this license. If you do not agree to this
license, do not download, install, copy or use the software.

License Agreement
For RoboSense LiDAR SDK Library
(3-clause BSD License)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the names of the RoboSense, nor Suteng Innovation Technology, nor the names of other contributors may be used
to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************************************************/

#pragma once
#include <rs_driver/driver/input/input.hpp>

#include <sstream>

#ifdef _WIN32
#define WIN32
#else //__linux__
#endif

#include <pcap.h>

namespace robosense
{
namespace lidar
{
class InputPcap : public Input
{
public:
  InputPcap(const RSInputParam& input_param, double sec_to_delay)
    : Input(input_param), pcap_(NULL), pcap_offset_(ETH_HDR_LEN), pcap_tail_(0), difop_filter_valid_(false), imu_filter_valid_(false)
  {
    if (input_param.use_vlan)
    {
      pcap_offset_ += VLAN_HDR_LEN;
    }

    pcap_offset_ += input_param.user_layer_bytes;
    pcap_tail_   += input_param.tail_layer_bytes;

    std::stringstream msop_stream, difop_stream, imu_stream;
    if (input_param_.use_vlan)
    {
      msop_stream << "vlan && ";
      difop_stream << "vlan && ";
    }

    msop_stream << "udp dst port " << input_param_.msop_port;
    difop_stream << "udp dst port " << input_param_.difop_port;
    imu_stream << "udp dst port " << input_param_.imu_port;

    msop_filter_str_ = msop_stream.str();
    difop_filter_str_ = difop_stream.str();
    imu_filter_str_ = imu_stream.str();
    
    float play_rate_ = input_param.pcap_rate;
    if(play_rate_ <= 0.0f)
    {
      play_rate_ = 1.0f;
    }
    if(play_rate_ > 10.0f)
    {
      play_rate_ = 10.0f;
    }
    millisec_to_delay_ = (uint64_t)(100.0f / play_rate_);
  }

  virtual bool init();
  virtual bool start();
  virtual ~InputPcap();

private:
  void recvPacket();
private:
  pcap_t* pcap_;
  size_t pcap_offset_;
  size_t pcap_tail_;
  std::string msop_filter_str_;
  std::string difop_filter_str_;
  std::string imu_filter_str_;
  bpf_program msop_filter_;
  bpf_program difop_filter_;
  bpf_program imu_filter_;
  bool difop_filter_valid_;
  bool imu_filter_valid_;
  uint64_t millisec_to_delay_;
 
};

inline bool InputPcap::init()
{
  if (init_flag_)
    return true;

  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_ = pcap_open_offline(input_param_.pcap_path.c_str(), errbuf);
  if (pcap_ == NULL)
  {
    cb_excep_(Error(ERRCODE_PCAPWRONGPATH));
    return false;
  }

  pcap_compile(pcap_, &msop_filter_, msop_filter_str_.c_str(), 1, 0xFFFFFFFF);

  if ((input_param_.difop_port != 0) && (input_param_.difop_port != input_param_.msop_port))
  {
    pcap_compile(pcap_, &difop_filter_, difop_filter_str_.c_str(), 1, 0xFFFFFFFF);
    difop_filter_valid_ = true;
  }
  if ((input_param_.imu_port != 0) && (input_param_.imu_port != input_param_.msop_port) &&  (input_param_.imu_port != input_param_.difop_port))
  {
    pcap_compile(pcap_, &imu_filter_, imu_filter_str_.c_str(), 1, 0xFFFFFFFF);
    imu_filter_valid_ = true;
  }

  init_flag_ = true;
  return true;
}

inline bool InputPcap::start()
{
  if (start_flag_)
    return true;

  if (!init_flag_)
  {
    cb_excep_(Error(ERRCODE_STARTBEFOREINIT));
    return false;
  }

  to_exit_recv_ = false;
  recv_thread_ = std::thread(std::bind(&InputPcap::recvPacket, this));

  start_flag_ = true;
  return true;
}

inline InputPcap::~InputPcap()
{
  stop();

  if (pcap_ != NULL)
  {
    pcap_close(pcap_);
    pcap_ = NULL;
  }
}
inline void InputPcap::recvPacket()
{
  auto start_time = std::chrono::steady_clock::now();
  while (!to_exit_recv_)
  {
    struct pcap_pkthdr* header;
    const u_char* pkt_data;
    int ret = pcap_next_ex(pcap_, &header, &pkt_data);
    if (ret < 0)  // reach file end.
    {
      pcap_close(pcap_);
      pcap_ = NULL;

      if (input_param_.pcap_repeat)
      {
        cb_excep_(Error(ERRCODE_PCAPREPEAT));

        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_ = pcap_open_offline(input_param_.pcap_path.c_str(), errbuf);
        continue;
      }
      else
      {
        cb_excep_(Error(ERRCODE_PCAPEXIT));
        break;
      }
    }

    if (pcap_offline_filter(&msop_filter_, header, pkt_data) != 0)
    {
      int dataLen = (int)(header->len - pcap_offset_ - pcap_tail_);
      if (dataLen < 0) {
        cb_excep_(Error(ERRCODE_WRONGMSOPPCAPPARSE));
        continue;
      }
      std::shared_ptr<Buffer> pkt = cb_get_pkt_(ETH_LEN);
      if(static_cast<size_t>(dataLen) > pkt->bufSize())
      {
        cb_excep_(Error(ERRCODE_WRONGMSOPPCAPPARSE));
        continue;
      }
      memcpy(pkt->data(), pkt_data + pcap_offset_, dataLen);
      pkt->setData(0, dataLen);

      if (cb_split_frame_(pkt->data()))
      {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time);
        auto sleepTime = std::chrono::milliseconds(millisec_to_delay_) - duration;
        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);   
        }
        start_time = std::chrono::steady_clock::now();
      }
      pushPacket(pkt);
    }
    else if (difop_filter_valid_ && (pcap_offline_filter(&difop_filter_, header, pkt_data) != 0))
    {
      int dataLen = (int)(header->len - pcap_offset_ - pcap_tail_);
      if (dataLen < 0) {
        cb_excep_(Error(ERRCODE_WRONGDIFOPPCAPPARSE));
        continue;
      }

      std::shared_ptr<Buffer> pkt = cb_get_pkt_(ETH_LEN);
      if(static_cast<size_t>(dataLen) > pkt->bufSize())
      {
        cb_excep_(Error(ERRCODE_WRONGDIFOPPCAPPARSE));
        continue;
      }

      memcpy(pkt->data(), pkt_data + pcap_offset_, dataLen);
      pkt->setData(0, dataLen);
      pushPacket(pkt);
    }
    else if (imu_filter_valid_ && (pcap_offline_filter(&imu_filter_, header, pkt_data) != 0))
    {
      int dataLen = (int)(header->len - pcap_offset_ - pcap_tail_);
      if (dataLen < 0) {
        cb_excep_(Error(ERRCODE_WRONGIMUPCAPPARSE));
        continue;
      }
      std::shared_ptr<Buffer> pkt = cb_get_pkt_(ETH_LEN);
      if(static_cast<size_t>(dataLen) > pkt->bufSize())
      {
        cb_excep_(Error(ERRCODE_WRONGIMUPCAPPARSE));
        continue;
      }
      memcpy(pkt->data(), pkt_data + pcap_offset_, dataLen);
      pkt->setData(0, dataLen);
      pushPacket(pkt);
    }
    else
    {
      continue;
    }
  }
}

}  // namespace lidar
}  // namespace robosense
