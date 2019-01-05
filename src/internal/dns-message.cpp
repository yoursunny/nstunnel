#include "dns-message.hpp"
#include <lwip/def.h>

namespace dns_message {

// https://0x00sec.org/t/dns-header-for-c/618
struct DnsHeader
{
  uint16_t id;
  uint16_t rd:1;
  uint16_t tc:1;
  uint16_t aa:1;
  uint16_t opcode:4;
  uint16_t qr:1;
  uint16_t rcode:4;
  uint16_t zero:3;
  uint16_t ra:1;
  uint16_t qcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t adcount;
};

struct DnsQuestionTail
{
  uint16_t qtype;
  uint16_t qclass;
};

size_t
writeTxtQuery(uint8_t pkt[MAXLEN], uint16_t id, std::initializer_list<String> name)
{
  static_assert(MAXLEN > sizeof(DnsHeader) + sizeof(DnsQuestionTail), "");

  DnsHeader hdr = {0};
  hdr.id = id;
  hdr.rd = 1;
  hdr.qcount = htons(1);
  memcpy(pkt, &hdr, sizeof(hdr));
  size_t len = sizeof(hdr);

  for (const String& part : name) {
    for (int start = 0, end; start < part.length(); start = end + 1) {
      end = part.indexOf('.', start);
      if (end < 0) {
        end = part.length();
      }
      int labelLen = end - start;
      if (labelLen > 63 || labelLen > (MAXLEN - len) - 1 - 1 - sizeof(DnsQuestionTail)) {
        return 0;
      }
      pkt[len++] = labelLen;
      memcpy(&pkt[len], part.c_str() + start, labelLen);
      len += labelLen;
    }
  }

  pkt[len++] = 0; // empty label for root zone

  DnsQuestionTail tail = {0};
  tail.qtype = htons(16); // TXT
  tail.qclass = htons(1); // IN
  memcpy(&pkt[len], &tail, sizeof(tail));
  len += sizeof(tail);

  return len;
}

class TxtResponseParser
{
public:
  bool
  hasInput(size_t count) const
  {
    return rem >= count;
  }

  const void*
  read(size_t size)
  {
    assert(hasInput(size));
    const uint8_t* result = input;
    input += size;
    rem -= size;
    return result;
  }

  template<typename T>
  bool
  copyTo(T* result)
  {
    if (!hasInput(sizeof(T))) {
      return false;
    }
    memcpy(result, read(sizeof(T)), sizeof(T));
    return true;
  }

  /** \brief Read several uint16 values.
   *  \return last value, or -1 to indicate error.
   */
  int
  readMultiUint16(int count)
  {
    uint16_t v;
    while (--count >= 0) {
      if (!copyTo<uint16_t>(&v)) {
        return -1;
      }
    }
    return ntohs(v);
  }

  /** \brief Read and discard a name.
   */
  bool
  skipName()
  {
    bool hasCompression = false;
    while (!hasCompression) {
      uint8_t sizeofLabel;
      if (!copyTo<uint8_t>(&sizeofLabel)) {
        return false;
      }

      if (sizeofLabel == 0) {
        return true;
      }
      if ((sizeofLabel & 0xC0) == 0xC0) { // compressed label
        sizeofLabel = 1;
        hasCompression = true;
      }

      // skip label
      if (!hasInput(sizeofLabel)) {
        return false;
      }
      read(sizeofLabel);
    }
    return true;
  }

public:
  const uint8_t* input;
  size_t rem;
};

TxtResponse::TxtResponse()
  : id(0)
{
}

bool
TxtResponse::parse(const uint8_t* pkt, size_t len)
{
  TxtResponseParser parser{pkt, len};

  DnsHeader hdr;
  if (!parser.copyTo<DnsHeader>(&hdr) ||
      ntohs(hdr.qcount) != 1 || ntohs(hdr.ancount) != 1) {
    return false;
  }
  id = hdr.id;

  // question
  if (!parser.skipName() || parser.readMultiUint16(2) < 0) {
    return false;
  }

  // answer
  int sizeofRdata;
  if (!parser.skipName() || (sizeofRdata = parser.readMultiUint16(5)) < 0 ||
      !parser.hasInput(sizeofRdata)) {
    return false;
  }
  uint8_t sizeofTxt;
  if (!parser.copyTo<uint8_t>(&sizeofTxt) || !parser.hasInput(sizeofTxt)) {
    return false;
  }
  rdata.reserve(sizeofTxt);
  const uint8_t* txt = static_cast<const uint8_t*>(parser.read(sizeofTxt));
  for (int i = 0; i < sizeofTxt; ++i) {
    rdata.concat(static_cast<char>(txt[i]));
  }

  return true;
}

} // namespace dns_message
