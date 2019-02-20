// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <net/ip6/addr.hpp>

#include <cstdint>
#include <sstream>

inline uint8_t nibbleFromByte(uint8_t character) noexcept
{
  if(character >= '0' && character <= '9')
    return character - '0';
  if(character >= 'A' && character <= 'F')
    return character - 'A' + 10;
  if(character >= 'a' && character <= 'f')
    return character - 'a' + 10;
  return 0xFF;
}


inline uint16_t shortfromHexString(const std::string &str) noexcept
{
  uint16_t val=0;
  int shift=(str.size()-1)*4;
  int size = str.size();
  //this is why big endian is used for networking
  //could have used strtol
  for (auto i=0;i < size; i++)
  {
      val|=(nibbleFromByte(str[i])&0xF)<<shift;
      shift-=4;
  }
  using namespace net;
  //handle the endianess
  return ntohs(val);
}

inline bool isLegalCharacter(char character)
{
  if(character >= '0' && character <= '9')
    return true;
  if(character >= 'A' && character <= 'F')
    return true;
  if(character >= 'a' && character <= 'f')
    return true;
  if(character == ':')
    return true;
  return false;
}

constexpr int ip6_delimiters=7;
constexpr int ip6_max_entry_length=4;
constexpr int ip6_min_string_length=2;
constexpr int ip6_max_stringlength=ip6_delimiters+(ip6_max_entry_length*8);


namespace net::ip6 {

  Addr::Addr(const std::string &addr)
  {
    //This implementation does not take into account if there is a % or /
    int delimiters=ip6_delimiters;
    int fillat=0;
    char prev='N';

    if (addr.size() > ip6_max_stringlength) {
      throw Invalid_Address("To many characters in "+addr);
    }
    if (addr.size() < ip6_min_string_length) {
      throw Invalid_Address("To few characters in "+addr);
    }

    for (auto &character:addr)
    {
      if (!isLegalCharacter(character))
      {
        throw Invalid_Address("Illegal character "+std::string(1,character)+" in "+addr);
      }

      if (character == ':')
      {
        if (prev==':') {
          if (fillat != 0) {
            throw Invalid_Address("Multiple :: :: in "+addr);
          }
          fillat=delimiters;
        }
        delimiters--;
      }
      prev=character;
    }

    if (delimiters < 0){
      throw Invalid_Address("To many : in "+addr);
    }

    //since we are counting down fillat can at best be 1 as its the previous :
    if (delimiters > 0 && fillat == 0 )
    {
      throw Invalid_Address("To few : in"+addr);
    }

    std::string token;
    std::istringstream tokenStream(addr);

    int countdown=ip6_delimiters;
    int idx=0;

    while (std::getline(tokenStream, token, ':'))
    {
      if (countdown==fillat)
      {
        while(delimiters--)
        {
          i16[idx++]=0;
          //tokens.push_back(std::string("0"));
        }
      }
      if (token.size() > 4)
      {
        throw Invalid_Address("To many entries in hexadectet/quibble/quad-nibble "+token+" in "+addr);
      }
      i16[idx++]=shortfromHexString(token);
    //  tokens.push_back(token);
      countdown--;
    }

    //last character is : hence there is no token for it
    if (prev == ':'){
      i16[idx]=0;
    }
  }
}
