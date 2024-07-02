#include <Supervisor/Supervisor.h>
#include <Supervisor/GamesInfoExtractor.h>
#include <Supervisor/Lobbies.h>

#include <PlametaParser/Entry.h>
#include <Games/CommObjects.h>

#include <easylogging++.h>
#include <nlohmann/json.hpp>

#include <string>
#include <iostream>
#include <thread>

namespace pla::supervisor {

using namespace games;
using namespace games::json_entries;

Supervisor::Supervisor(std::stringstream configStream)
  : m_configParser(std::move(configStream))
  , m_gameInstancesCheckingThread(std::jthread(&Supervisor::_gameInstancesCheckingThread, this))
{
  auto helpCmd = std::make_shared<Command>(
          "help",
          "Lists all available commands",
          [this]()
          {
            std::cout << "Available commands:\n";
            for (const auto& command : this->m_commands) {
              std::cout << "\t" << command->getCommand() << " - " << command->getDescription() << "\n";
            }
          }
  );

  auto quitCmd = std::make_shared<Command>(
          "quit",
          "Stops server and all game instances",
          [this]()
          {
            this->m_run = false;
          }
  );

  _registerCommand(std::move(helpCmd));
  _registerCommand(std::move(quitCmd));
}


Supervisor::~Supervisor()
{
  for (auto& [clientId, gameInstance] : m_gameInstances) {
    auto& [serverHandler, syncParams] = gameInstance;
    serverHandler->stop();
  }

  m_run = false;
}


void Supervisor::run()
{
  auto entryPtr = m_configParser["config:port"];
  std::cout << "[Config]:port = " << std::any_cast<int>(entryPtr->getValue()) << "\n";

  std::size_t port = static_cast<size_t>(std::any_cast<int>(entryPtr->getValue()));
  network::SupervisorPacketHandler supervisorPacketHandler {m_run, port};
  m_packetHandler = &supervisorPacketHandler;

  supervisorPacketHandler.runInBackground();
  std::thread inputThread {&Supervisor::_getUserInput, this};

  Lobbies::startWatchdogThread(supervisorPacketHandler);

  while(m_run) {
    _processPackets(supervisorPacketHandler);
  }

  Lobbies::stopWatchdogThread();

  inputThread.join();
}


void Supervisor::_registerCommand(std::shared_ptr<Command>&& command)
{
  m_commands.push_back(std::move(command));
}


void Supervisor::_getUserInput()
{
  std::string inputCommand;

  std::cout << "Planszówker Server v" << PROJECT_VER << "\n"
                                                        "(Type \'help\' for list of all available commands)\n"
                                                        "================================================\n";

  while (m_run) {
    std::cout << "Command: ";

    std::cin >> inputCommand;

    bool cmdFound {false};
    for (const auto& command : m_commands) {
      if (command->getCommand().compare(inputCommand) == 0) {
        command->invoke();
        cmdFound = true;
        break;
      }
    }

    if (!cmdFound) {
      std::cout << "Command not found!\n";
    }
  }
}


void Supervisor::_processPackets(network::SupervisorPacketHandler& packetHandler)
{
  // TODO: Refactor this bulk...
  std::vector<size_t> clientIdKeys;
  auto packetsMap = packetHandler.getPackets(clientIdKeys);

  for (const auto& clientIdKey: clientIdKeys) {
    // Find map entry with client ID -> (size_t, deque) is received
    const auto mapIt = packetsMap.find(clientIdKey);
    if (mapIt == packetsMap.end()) {
      continue;
    }

    // Iterate over all packets in deque
    for (auto& packet: mapIt->second) {
      Request request{};
      packet >> request;

      try {
        if (request.type == PacketType::Heartbeat) {
          // We don't care about Heartbeat packets
          continue;
        } else if (request.type == PacketType::ID) {
          // If we get ID request, we need to send client's ID
          nlohmann::json replyJson;
          replyJson[CLIENT_ID] = clientIdKey;

          Reply idReply{
            .type = PacketType::ID,
            .body = replyJson.dump()
          };

          sf::Packet replyPacket;
          replyPacket << idReply;

          packetHandler.sendPacketToClient(clientIdKey, replyPacket);
        } else if (request.type == PacketType::ListAvailableGames) {
          _listAvailableGamesHandler(clientIdKey, packetHandler);
        } else if (request.type == PacketType::CreateLobby) {
          _createLobbyHandler(clientIdKey, packetHandler, nlohmann::json::parse(request.body));
        } else if (request.type == PacketType::GetLobbyDetails) {
          _getLobbyDetailsHandler(clientIdKey, packetHandler, nlohmann::json::parse(request.body));
        } else if (request.type == PacketType::ListOpenLobbies) {
          _listOpenLobbiesHandler(clientIdKey, packetHandler, nlohmann::json::parse(request.body));
        } else if (request.type == PacketType::JoinLobby) {
          _joinLobbyHandler(clientIdKey, packetHandler, nlohmann::json::parse(request.body));
        } else if (request.type == PacketType::LobbyHeartbeat) {
          _lobbyHeartbeatHandler(clientIdKey, packetHandler, nlohmann::json::parse(request.body));
        } else if (request.type == PacketType::StartGame) {
          _startGameHandler(clientIdKey, packetHandler);
        } else if (request.type == PacketType::GameSpecificData) {
          _gameSpecificDataHandler(clientIdKey, packetHandler, request);
        } else if (request.type == PacketType::DownloadAssets) {
          _downloadAssetsHandler(clientIdKey, packetHandler);
        }
      } catch (std::exception& e) {
        LOG(DEBUG) << "[Supervisor] Exception in _processPackets";
      }

      LOG(DEBUG) << "Type " << static_cast<int>(request.type) << " for clientId " << clientIdKey;
//      std::cout << "Type: " << static_cast<int>(request.type) << "\n";
//      try {
//        std::cout << "Request JSON:\n" << nlohmann::json::parse(request.body).dump(4) << "\n";
//      } catch (std::exception& e) {
//        std::cout << "Request:" << request.body << "\n";
//      }
    }
  }
}


void Supervisor::_listAvailableGamesHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler)
{
  // Remove a lobby from a list if exists
  Lobbies::removeLobby(clientIdKey);

  static GamesInfoExtractor gamesInfoExtractor;

  auto& metaAssets = gamesInfoExtractor.getMetaAssets();

  for(const auto& metaAsset: metaAssets) {
    Reply reply {.type = games::PacketType::ListAvailableGames};
    reply.body = metaAsset.first + "::" + metaAsset.second;

    LOG(DEBUG) << "Reply length: " << reply.body.length();

    sf::Packet packet;
    packet << reply;

    packetHandler.sendPacketToClient(clientIdKey, packet);
  }
}


void Supervisor::_createLobbyHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson)
{
  Reply reply {
    .type = games::PacketType::CreateLobby,
  };

  LOG(DEBUG) << "[Create Lobby Handler]";
  try {
    auto lobby = Lobbies::createNewLobby(clientIdKey, requestJson.at(LOBBY_NAME), requestJson.at(GAME_KEY));

    sf::Packet packet;

    nlohmann::json replyJson;
    replyJson[VALID] = true;
    reply.body = replyJson.dump();

    packet << reply;

    packetHandler.sendPacketToClient(clientIdKey, packet);

    lobby->sendUpdate(packetHandler);
  } catch (std::exception& e) { }
}


void Supervisor::_getLobbyDetailsHandler(size_t clientIdKey, network::SupervisorPacketHandler &packetHandler, const nlohmann::json &requestJson)
{
  Reply reply {
    .type = games::PacketType::GetLobbyDetails,
  };

  LOG(DEBUG) << "[Get Lobby Details Handler]";

  auto lobby = Lobbies::getLobby(requestJson[CREATOR_ID]);
  if (lobby) {
    LOG(DEBUG) << "[LobbyDetailsHandler] CreatorID: " << lobby->getCreatorClientId();
    LOG(DEBUG) << "[LobbyDetailsHandler] Lobby Name: " << lobby->getLobbyName();
    LOG(DEBUG) << "[LobbyDetailsHandler] Game Key: " << lobby->getGameKey();

    nlohmann::json replyJson;
    replyJson[CREATOR_ID] = lobby->getCreatorClientId();
    replyJson[CLIENT_IDS] = lobby->getClients();
    replyJson[LOBBY_NAME] = lobby->getLobbyName();
    replyJson[GAME_KEY] = lobby->getGameKey();
    replyJson[MAX_PLAYERS] = lobby->getMaxPlayers();
    replyJson[MIN_PLAYERS] = lobby->getMinPlayers();
    replyJson[CURRENT_PLAYERS] = lobby->getCurrentPlayers();
    replyJson[VALID] = true;
    reply.body = replyJson.dump();

    sf::Packet packet;
    packet << reply;

    packetHandler.sendPacketToClient(clientIdKey, packet);
  }
}


void Supervisor::_listOpenLobbiesHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson)
{
  Reply reply {
    .type = games::PacketType::ListOpenLobbies,
  };

  LOG(DEBUG) << "[List Open Lobbies Handler]";

  // Remove a lobby from a list if exists
  Lobbies::removeLobby(clientIdKey);

  try {
    auto gameKey = requestJson[GAME_KEY].get<std::string>();

    nlohmann::json replyJson;
    replyJson[GAME_KEY] = gameKey;
    replyJson[LOBBIES] = nlohmann::json::array();
    for (const auto& [creatorID, lobby]: Lobbies::getLobbies()) {
      // Only look for lobbies with given game key
      if (lobby.getGameKey() == gameKey) {
        replyJson[LOBBIES].push_back({
          {CREATOR_ID, lobby.getCreatorClientId()},
          {LOBBY_NAME, lobby.getLobbyName()},
          {MIN_PLAYERS, lobby.getMinPlayers()},
          {CURRENT_PLAYERS, lobby.getCurrentPlayers()},
          {MAX_PLAYERS, lobby.getMaxPlayers()}
        });
      }
    }
    replyJson[VALID] = true;
    reply.body = replyJson.dump();

    LOG(DEBUG) << "Reply:\n" << replyJson.dump(4);

    sf::Packet packet;
    packet << reply;

    packetHandler.sendPacketToClient(clientIdKey, packet);
  } catch (const std::exception& e) {
    LOG(DEBUG) << "[ListOpenLobbiesHandler] Exception!";
  }
}


void Supervisor::_joinLobbyHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson)
{
  Reply reply {
    .type = games::PacketType::JoinLobby,
  };

  LOG(DEBUG) << "[Join Lobby Handler]";

  auto lobby = Lobbies::getLobby(requestJson[CREATOR_ID].get<size_t>());
  if (lobby and requestJson[CREATOR_ID].get<size_t>() != clientIdKey) {
    nlohmann::json replyJson;
    replyJson[VALID] = false;

    if (lobby->addClient(clientIdKey)) {
      replyJson[VALID] = true;
    }

    reply.body = replyJson.dump();

    sf::Packet packet;
    packet << reply;

    packetHandler.sendPacketToClient(clientIdKey, packet);

    lobby->sendUpdate(packetHandler);
  }
}


void Supervisor::_lobbyHeartbeatHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const nlohmann::json& requestJson)
{
  try {
    auto type = requestJson[LOBBY_HEARTBEAT_TYPE].get<std::string>();
    if (type == "Creator") {
      // Update last response time for lobby
      Lobbies::updateLobbyLastResponseTime(clientIdKey);
    } else if (type == "Client") {
      auto creatorId = requestJson[CREATOR_ID].get<size_t>();
      Lobbies::updateClientLastResponseTime(creatorId, clientIdKey);
    }
  } catch (std::exception& e) {
  }
}


void Supervisor::_startGameHandler(size_t clientIdKey, network::SupervisorPacketHandler &packetHandler)
{
  nlohmann::json replyJson;
  replyJson[VALID] = false;

  // We should check whether a lobby has enough clients connected
  auto lobby = Lobbies::getLobby(clientIdKey);
  if (lobby->getCreatorClientId() == clientIdKey && lobby->hasEnoughClients()) {
    _createNewGameInstance(packetHandler, *lobby);

    replyJson[VALID] = true;
  }

  lobby->sendToAllClients(packetHandler, games::PacketType::StartGame, replyJson.dump());
}


void Supervisor::_createNewGameInstance(network::SupervisorPacketHandler& packetHandler, const Lobby& lobby)
{
  auto gameInstanceSyncParametersPtr = std::make_unique<GameInstanceSyncParameters>();

  GameInstance gameInstance {packetHandler, gameInstanceSyncParametersPtr->queue,
                             std::string(lobby.getGameKey()), lobby.getClients(), lobby.getCreatorClientId()};

  auto serverHandlerPtr = std::make_shared<games_server::ServerHandler>(gameInstance);

  gameInstanceSyncParametersPtr->gameServerThread = std::jthread(&games_server::ServerHandler::run, serverHandlerPtr);

  // If we don't have a game instance already created for given Creator ID, we create one
  if (m_gameInstances.find(gameInstance.creatorId) == m_gameInstances.end()) {
    LOG(DEBUG) << "[Supervisor::_createNewGameInstance] Creating new game instance...";
    m_gameInstances.emplace(gameInstance.creatorId, std::make_tuple(std::move(serverHandlerPtr), std::move(gameInstanceSyncParametersPtr)));

    for (auto clientId : lobby.getClients()) {
      // Insert or overwrite
      m_clientCreatorMapper[clientId] = lobby.getCreatorClientId();
    }
  }
}


void Supervisor::_gameSpecificDataHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler, const games::Request& request)
{
  try {
    auto requestJson = nlohmann::json::parse(request.body);

    std::unique_lock lock{m_gameInstancesMutex};

    auto it = m_clientCreatorMapper.find(clientIdKey);
    if (it == m_clientCreatorMapper.end()) {
      return;
    }

    auto gameInstance = m_gameInstances.find(it->second);
    if (gameInstance != m_gameInstances.end()) {
      auto& [serverHandler, gameSyncParams] = gameInstance->second;
      GameInstanceQueueParameters queueParams {
        .clientId = clientIdKey,
        .request = request
      };
      gameSyncParams->queue.push(queueParams);
    }
  } catch (std::exception& e) { }
}


void Supervisor::_downloadAssetsHandler(size_t clientIdKey, network::SupervisorPacketHandler& packetHandler)
{
  try {
    std::scoped_lock lock{m_gameInstancesMutex};

    auto it = m_clientCreatorMapper.find(clientIdKey);
    if (it == m_clientCreatorMapper.end()) {
      return;
    }

    LOG(DEBUG) << "[Supervisor] Download Assets Handler for client " << clientIdKey;

    auto gameInstance = m_gameInstances.find(it->second);
    if (gameInstance != m_gameInstances.end()) {
      auto& [serverHandler, gameSyncParams] = gameInstance->second;
      serverHandler->transmitAssetsToClient(clientIdKey);
    }
  } catch (std::exception& e) { }
}


void Supervisor::_gameInstancesCheckingThread()
{
  while (m_run) {
    m_tickThread.waitForTick();

    std::scoped_lock lock{m_gameInstancesMutex};

    if (not m_packetHandler) {
      return;
    }

    auto clientIds = m_packetHandler->getClients();

    for (auto it = m_gameInstances.begin(); it != m_gameInstances.end();) {
      auto& [creatorId, gameInstance] = *it;
      auto& [serverHandler, _] = gameInstance;

      if (not serverHandler) {
        continue;
      }

      bool terminate = false;

      // If creatorId has disconnected
      if (std::find(clientIds.begin(), clientIds.end(), creatorId) == clientIds.end()) {
        LOG(DEBUG) << "[Supervisor::_gameInstancesCheckingThread] Stopping game instance. Creator has disconnected";
        serverHandler->stop();
        terminate = true;
      }

      // If game has finished
      if (serverHandler->getLogic() && serverHandler->getLogic()->isGameFinished()) {
        LOG(DEBUG) << "[Supervisor::_gameInstancesCheckingThread] Stopping game instance. Game has finished";
        serverHandler->stop();
        terminate = true;
      }

      if (terminate) {
        LOG(DEBUG) << "[Supervisor::_gameInstancesCheckingThread] Terminating game instance...";
        it = m_gameInstances.erase(it);
      } else {
        ++it;
      }
    }
  }
}

} // namespace
