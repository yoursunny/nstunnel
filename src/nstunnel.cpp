#include "nstunnel.hpp"
#include "internal/dns-message.hpp"
#include <esp32-hal.h>

NsTunnel::NsTunnel(UDP& udp)
  : m_udp(udp)
  , m_rxCb(nullptr)
{
}

bool
NsTunnel::begin(const String& domain, IPAddress serverIp, uint16_t serverPort, uint16_t clientPort)
{
  m_domain = domain;
  m_serverIp = serverIp;
  m_serverPort = serverPort;
  return static_cast<bool>(m_udp.begin(clientPort));
}

void
NsTunnel::onRx(const RxCallback& cb)
{
  m_rxCb = cb;
}

void
NsTunnel::loop()
{
  int len;
  while ((len = m_udp.parsePacket()) > 0) {
    dns_message::TxtResponse parsed;
    uint8_t pkt[dns_message::MAXLEN];
    if (m_rxCb != nullptr && m_udp.read(pkt, sizeof(pkt)) == len &&
        parsed.parse(pkt, len)) {
      m_rxCb(parsed.id, parsed.rdata);
    }
    m_udp.flush();
    yield();
  }
}

bool
NsTunnel::send(uint16_t id, const String& msg)
{
  uint8_t pkt[dns_message::MAXLEN];
  size_t size = dns_message::writeTxtQuery(pkt, id, {msg, m_domain});
  if (size == 0 || m_udp.beginPacket(m_serverIp, m_serverPort) != 1) {
    return false;
  }
  m_udp.write(pkt, size);
  return m_udp.endPacket() == 1;
}
