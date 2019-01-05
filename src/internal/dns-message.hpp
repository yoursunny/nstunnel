#ifndef NSTUNNEL_DNS_MESSAGE_HPP
#define NSTUNNEL_DNS_MESSAGE_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <WString.h>

namespace dns_message {

constexpr size_t MAXLEN = 512;

/** \brief Write a DNS query containing a TXT question.
 */
size_t writeTxtQuery(uint8_t pkt[MAXLEN], uint16_t id, std::initializer_list<String> name);

class TxtResponse
{
public:
  TxtResponse();

  bool parse(const uint8_t* pkt, size_t len);

public:
  uint16_t id;
  String rdata;
};

} // namespace dns_message

#endif // NSTUNNEL_DNS_MESSAGE_HPP
