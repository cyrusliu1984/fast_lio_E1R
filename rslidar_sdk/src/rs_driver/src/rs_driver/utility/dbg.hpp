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
#include <stdio.h>
#include <cstring>  
namespace robosense
{
namespace lidar
{

inline void hexdump(const uint8_t* data, size_t size, const char* desc = NULL)
{
  printf("\n---------------%s(size:%d)------------------", (desc ? desc : ""), (int)size);

  for (size_t i = 0; i < size; i++)
  {
    if (i % 16 == 0)
      printf("\n");
    printf("%02x ", data[i]);
  }

  printf("\n---------------------------------\n");
}
inline bool isLittleEndian() {
    uint16_t num = 0x0102;  
    uint8_t *bytePtr = reinterpret_cast<uint8_t*>(&num);
    return (bytePtr[0] == 0x02);
}
inline int32_t u8ArrayToInt32(const uint8_t* data, uint8_t len,  bool is_little_endian) {
    int32_t s32Data = 0;
    if(len != 4)
    {
      return 0;
    }
    if(is_little_endian)
    {
      s32Data = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    }else{
      s32Data = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
    }
    return s32Data;
}
inline float convertUint32ToFloat(uint32_t byteArray) {
    float floatValue;
    std::memcpy(&floatValue, &byteArray, sizeof(float));
    return floatValue;
}


}  // namespace lidar
}  // namespace robosense
