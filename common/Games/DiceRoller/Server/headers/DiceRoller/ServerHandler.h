#pragma once

/* Generic */
#include "Games/ServerHandler.h"
#include "NetworkHandler/NetworkHandler.h"

/* STD */
#include <memory>
#include <vector>

namespace pla::common::games::dice_roller {

class DiceRollerServerHandler : public ServerHandler
{
public:
  DiceRollerServerHandler() = default;
  ~DiceRollerServerHandler() = default;

  void run() final;
  void stop() final { };

  void networkCall(std::shared_ptr<sf::TcpSocket>& client, sf::Packet& packet, size_t playerId) final;
  void setPlayersIds(std::vector<size_t> playerIds) final;
private:
  static constexpr size_t MaxPlayers = 2;
};

} // namespaces
