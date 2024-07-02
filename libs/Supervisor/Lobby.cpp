#include <Lobby.h>

#include <GamesInfoExtractor.h>
#include <Games/CommObjects.h>

#include <nlohmann/json.hpp>
#include <easylogging++.h>
#include <regex>

namespace pla::supervisor {

using namespace games::json_entries;

Lobby::Lobby(size_t creatorClientId, std::string lobbyName, std::string gameKey)
  : m_creatorClientId(creatorClientId)
  , m_lobbyName(std::move(lobbyName))
  , m_gameKey(std::move(gameKey))
  , m_lastResponseTime(std::chrono::steady_clock::now())
{
  m_clients.insert({creatorClientId, std::chrono::steady_clock::now()});

  _extractGameMetadata();
}


void Lobby::_extractGameMetadata()
{
  static GamesInfoExtractor gamesInfoExtractor;
  const auto& plametas = gamesInfoExtractor.getPlametas();

  const std::regex plagameNameRegex {"([a-zA-Z0-9]+)(\\.plagame)"};

  std::smatch matches;
  for (const auto& [plametaGameFullKey, plametaParser]: plametas) {
    if (std::regex_search(plametaGameFullKey, matches, plagameNameRegex)) {
      // Extract game's name from file path - although spaces are permitted, they should not be used
      std::string gameShortKey = matches[1].str();

      if (gameShortKey == m_gameKey) {
        m_minPlayers = std::any_cast<int>(plametaParser["settings:min_players"]->getValue());
        m_maxPlayers = std::any_cast<int>(plametaParser["settings:max_players"]->getValue());
        LOG(DEBUG) << "We are dealing with " << gameShortKey << " game [min: " << m_minPlayers << ", max: " << m_maxPlayers << "]";
        return;
      }
    }
  }
}


bool Lobby::addClient(size_t clientId)
{
  if (m_clients.size() < m_maxPlayers) {
    // Check if ClientID is not already inserted
    if (m_clients.find(clientId) == m_clients.end()) {
      // If ClientID doesn't exist in the container, add it
      m_clients.insert({clientId, std::chrono::steady_clock::now()});
      return true;
    }
  }

  return false;
}


void Lobby::removeClient(size_t clientId)
{
  m_clients.erase(clientId);
}


void Lobby::sendUpdate(network::SupervisorPacketHandler& packetHandler) const
{
  sf::Packet packet;
  games::Reply reply {
    .type = games::PacketType::GetLobbyDetails,
  };

  nlohmann::json replyJson;
  replyJson[CREATOR_ID] = m_creatorClientId;
  replyJson[CLIENT_IDS] = getClients();
  replyJson[LOBBY_NAME] = m_lobbyName;
  replyJson[GAME_KEY] = m_gameKey;
  replyJson[MAX_PLAYERS] = m_maxPlayers;
  replyJson[MIN_PLAYERS] = m_minPlayers;
  replyJson[CURRENT_PLAYERS] = m_clients.size();
  replyJson[VALID] = true;
  reply.body = replyJson.dump();

  packet << reply;

  for (const auto& [client, _]: m_clients) {
    packetHandler.sendPacketToClient(client, packet);
  }
}


std::vector<size_t> Lobby::getClients() const {
  std::vector<size_t> returnVector;
  for (const auto& [clientId, _] : m_clients) {
    returnVector.push_back(clientId);
  }
  return returnVector;
}


void Lobby::updateClientLastResponseTime(size_t clientId)
{
  auto it = m_clients.find(clientId);
  if (it != m_clients.end()) {
    it->second = Clock::now();
  }
}


void Lobby::sendToAllClients(network::SupervisorPacketHandler& packetHandler, games::PacketType type, const std::string& body)
{
  games::Reply reply {
    .type = type,
    .body = body,
  };

  sf::Packet packet;
  packet << reply;

  for (const auto [clientId, _] : m_clients) {
    packetHandler.sendPacketToClient(clientId, packet);
  }
}

}