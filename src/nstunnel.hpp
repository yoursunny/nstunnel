#ifndef NSTUNNEL_NSTUNNEL_HPP
#define NSTUNNEL_NSTUNNEL_HPP

#include <Udp.h>

class NsTunnel
{
public:
  explicit
  NsTunnel(UDP& udp);

  bool begin(const String& domain, IPAddress serverIp = INADDR_NONE,
             uint16_t serverPort = 53, uint16_t clientPort = 5300);

  typedef void (*RxCallback)(uint16_t id, const String& msg);

  void onRx(const RxCallback& cb);

  void loop();

  bool send(uint16_t id, const String& msg);

private:
  UDP& m_udp;
  String m_domain;
  IPAddress m_serverIp;
  uint16_t m_serverPort;
  RxCallback m_rxCb;
};

#endif // NSTUNNEL_NSTUNNEL_HPP