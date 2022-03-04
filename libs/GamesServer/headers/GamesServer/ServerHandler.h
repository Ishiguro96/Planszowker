#pragma once

/* Generic */
#include <NetworkHandler/ServerPacketHandler.h>
#include "GamesHandler.h"
#include <AssetsManager/AssetsTransmitter.h>

/* SFML */
#include <SFML/Network.hpp>

/* STD */
#include <vector>
#include <memory>
#include <atomic>

namespace pla::games_server {

class ServerHandler
{
public:
  explicit ServerHandler(const std::string& gameName, network::ServerPacketHandler& packetHandler)
    : m_packetHandler(packetHandler)
    , m_run(true)
    , m_gameName(gameName)
    , m_gamesHandler(gameName)
  {
  }

  virtual ~ServerHandler()
  {
    m_packetHandler.stop();
  }

  virtual void run();

protected:
  virtual bool _internalHandling();

  std::string m_gameName;

  bool m_run;
  network::ServerPacketHandler& m_packetHandler;

  GamesHandler m_gamesHandler;

  std::map<size_t, std::shared_ptr<assets::AssetsTransmitter>> m_assetsTransmitterMap;
};

} // namespaces
