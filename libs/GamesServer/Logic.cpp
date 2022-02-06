#include "Logic.h"

#include <fstream>

#include "Rng/RandomGenerator.h"

namespace pla::common::games::server {

Logic::Logic(std::vector<size_t>& clientIds, const std::string& gameName)
  : m_gameName(gameName)
  , m_clientsIDs(clientIds)
{
  for (auto const& clientId : clientIds) {
    m_clientsIDsAndPoints[clientId] = 0;
  }

  m_currentClientsIDAndPointsIt = m_clientsIDsAndPoints.begin();

  m_luaVM.open_libraries(sol::lib::base,
                         sol::lib::package,
                         sol::lib::table,
                         sol::lib::string,
                         sol::lib::math);

  try {
    // DEBUG CODE
    std::ifstream jsonFile{"lua-scripts/games/DiceRoller/BoardDescription.json"};
    std::string readString;
    static std::string jsonString;
    while (jsonFile >> readString) {
      jsonString += readString;
    }

    std::cout << jsonString << "\n";

    m_luaVM["JsonString"] = jsonString;
    // END DEBUG CODE

    // Load core LUA modules
    m_luaVM.script("Machine = require('lua-scripts.core.lua-state-machine')"); // State machine
    m_luaVM.script("Json = require('lua-scripts.core.lua-json')"); // JSON encode-decode
    m_luaVM.script("ReplyModule = require('lua-scripts.core.lua-reply')"); // Reply Module
    m_luaVM.script("ActionRequest = require('lua-scripts.core.lua-action-request')"); // ActionRequest Module
    m_luaVM.script("Helper = require('lua-scripts.core.lua-helper')"); // Helper Module

    // Load Game Objects
    m_luaVM.script("Entity = require('lua-scripts.core.objects.entity')");
    m_luaVM.script("DestinationPoint = require('lua-scripts.core.objects.destination-point')");
    m_luaVM.script("GameObjects = require('lua-scripts.core.lua-game-objects')"); // Game Objects

    // Make necessary utils visible in LUA
    auto rng = m_luaVM.new_usertype<rng::RandomGenerator>("Rng",
            sol::constructors<rng::RandomGenerator(int, int)>());
    rng["GenerateRandomNumber"] = &rng::RandomGenerator::generateRandomNumber;

    // Bind functions to be available in LUA
    m_luaVM.set_function("AdvanceRound", &Logic::_advanceRound, this);
    m_luaVM.set_function("FinishGame", &Logic::_finishGame, this);
    m_luaVM.set_function("AddPointsToCurrentPlayer", &Logic::_addPointsToCurrentClient, this);
    m_luaVM.set_function("GetCurrentPlayerPoints", &Logic::_getCurrentPlayerPoints, this);
    m_luaVM.set_function("GetRoundsCounter", &Logic::_getRoundsCount, this);
    m_luaVM.set_function("GetCurrentPlayer", &Logic::_getCurrentClientID, this);
    m_luaVM.set_function("GetPlayers", &Logic::_getClients, this);
    m_luaVM.set_function("GetPlayerPoints", &Logic::_getClientPoints, this);
    m_luaVM.set_function("SendReply", &Logic::_updateClients, this);

    // Load Game Initialization script - loading this should create states, but it's game dependent
    sol::protected_function_result result = m_luaVM.script_file(LUA_SCRIPT_GAMES_PREFIX + m_gameName + "/" + m_gameName
            + LUA_SCRIPT_GAMES_INIT_SUFFIX);
  } catch(sol::error& e) {
    std::cerr << "Exception has been raised! " << e.what() << "\n";
  }
}

bool Logic::_checkIfTurnAvailable(size_t clientId) const
{
  return m_currentClientsIDAndPointsIt->first == clientId;
}

void Logic::_advanceRound()
{
  std::cout << "Advancing round...\n";
  m_currentClientsIDAndPointsIt = std::next(m_currentClientsIDAndPointsIt);
  if (m_currentClientsIDAndPointsIt == m_clientsIDsAndPoints.end()) {
    // Roll-over -> increase round counter by 1
    m_currentClientsIDAndPointsIt = m_clientsIDsAndPoints.begin();
    ++m_roundCounter;
  }
}

void Logic::_finishGame()
{
  // Send reply to clients with players' points
  std::cout << "[CORE] Game finished.\n";
  m_finished = true;
}


void Logic::_updateClients(std::string req) const
{
  Reply reply {
    .type = PacketType::GameSpecificData,
    .status = ReplyType::Success,
    .body = std::move(req)
  };

  std::cout << "[CORE] DEBUG: Reply string is: " << reply.body << "\n";

  sf::Packet replyPacket;
  replyPacket << reply;

  if (m_networkHandler) {
    m_networkHandler->sendPacketToEveryClients(replyPacket);
  } else {
    std::cerr << "[CORE] DEBUG: NetworkHandler is nullptr!\n";
  }
}


int Logic::_getClientPoints(size_t clientID) const
{
  int retValue {0};
  auto foundIt = m_clientsIDsAndPoints.find(clientID);
  if (foundIt != m_clientsIDsAndPoints.end()) {
    // We found a client's entry
    retValue = foundIt->second;
  }

  return retValue;
}


void Logic::_addPointsToCurrentClient(int points)
{
  m_currentClientsIDAndPointsIt->second += points;
}


void Logic::handleGameLogic(size_t clientId, Request requestType,
                                      network::ServerPacketHandler &packetHandler)
{
  for (const auto& client: m_clientsIDsAndPoints) {
    std::cout << "Available clientID: " << client.first << "\n";
  }
  std::cout << "Current clientID turn: " << m_currentClientsIDAndPointsIt->first << "\n";

  sf::Packet replyToClient;
  Reply replyStruct {};

  // TODO: bug when previous client was deleted before game - BRD-15
  if (!_checkIfTurnAvailable(clientId)) {
    packetHandler.sendPacketToClient(clientId, replyToClient);
    return;
  }

  m_networkHandler = &packetHandler;

  replyStruct.status = ReplyType::Success;

  // Every request handling is divided into 3 sections:
  //   - request decoding using JSON decoder and pre-processing,
  //   - invoking <GameName>.lua script,
  //   - sending reply to clients and post-processing.

  // Request is in JSON format. It is decoded and passed as a `request` table into LUA VM.
  try {
    m_luaVM.set("Request", requestType.body);
    m_luaVM.script("Request = Json.decode(Request)");
  } catch (sol::error& e) {
    std::cerr << "[LUA] Error: Exception has been raised!\n" << e.what() << "\n";
  }

  // Create `Reply` table
  m_luaVM["Reply"] = m_luaVM.create_table();

  // Invoking <GameName>.lua script
  try {
    m_luaVM.script_file(LUA_SCRIPT_GAMES_PREFIX + m_gameName + "/" + m_gameName + ".lua");
  } catch(sol::error& e) {
    std::cerr << "[LUA] Error: Exception has been raised!\n" << e.what() << "\n";
  }

  // Sending Reply to Clients
  try {
    m_luaVM["Reply"]["GameFinished"] = m_finished;
    m_luaVM.script("ReplyModule:SendReply()");
  } catch (sol::error& e) {
    std::cerr << "[LUA] Error: Exception has been raised!\n" << e.what() << "\n";
  }
}


} // namespaces
