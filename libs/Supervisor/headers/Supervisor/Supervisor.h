#pragma once

#include <Supervisor/GamesInfoExtractor.h>
#include <Supervisor/Command.h>

#include <AssetsManager/AssetsTransmitter.h>
#include <NetworkHandler/SupervisorPacketHandler.h>
#include <PlametaParser/Parser.h>
#include <ThreadSafeQueue/ThreadSafeQueue.h>
#include <Games/CommObjects.h>
#include <Games/GameInstance.h>
#include <GamesServer/ServerHandler.h>
#include <Supervisor/Lobby.h>
#include <TickThread/TickThread.h>

#include <nlohmann/json.hpp>

#include <atomic>
#include <memory>
#include <sstream>
#include <tuple>

namespace pla::games_server {
class ServerHandler;
}

namespace pla::supervisor {

class Supervisor
{
public:
  Supervisor() = delete;
  explicit Supervisor(std::stringstream&& configStream);

  ~Supervisor();

  void run();

private:
  using GameInstancesTuple = std::tuple<std::shared_ptr<games_server::ServerHandler>, std::unique_ptr<games::GameInstanceSyncParameters>>;

  void _getUserInput();
  void _registerCommand(std::shared_ptr<Command>&& command);
  void _processPackets(network::SupervisorPacketHandler& packetHandler);

  void _listAvailableGamesHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler);
  void _createLobbyHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson);
  void _getLobbyDetailsHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson);
  void _listOpenLobbiesHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson);
  void _joinLobbyHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson);
  void _lobbyHeartbeatHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson);
  void _startGameHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler);
  void _gameSpecificDataHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const games::Request& request);
  void _downloadAssetsHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler);

  void _createNewGameInstance(network::SupervisorPacketHandler& packetHandler, const Lobby& lobby);

  void _gameInstancesCheckingThread();

  utils::plameta::Parser m_configParser;

  std::atomic_bool m_run {true};

  std::vector<std::shared_ptr<Command>> m_commands;

  std::mutex m_gameInstancesMutex;
  std::jthread m_gameInstancesCheckingThread;

  utils::TickThread<std::chrono::milliseconds, 500> m_tickThread;

  network::SupervisorPacketHandler* m_packetHandler;

  std::unordered_map<size_t, GameInstancesTuple> m_gameInstances;
  std::unordered_map<size_t, size_t> m_clientCreatorMapper;
};

}