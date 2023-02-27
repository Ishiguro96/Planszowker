#include <States/GameLobbyState.h>
#include <Callbacks/GameLobbyCallbacks.h>

#include <GamesClient/SharedObjects.h>

#include <easylogging++.h>
#include <nlohmann/json.hpp>

#include <imgui_stdlib.h>

#include <utility>
#include <chrono>

namespace pla::games {

using namespace games_client;

GameLobbyState::GameLobbyState(games_client::GraphicalView& graphicalView, GameLobbyStateArguments gameLobbyStateArguments)
  : m_graphicalView(graphicalView)
  , m_gameWindow(*graphicalView.getGameWindow())
  , m_controller(graphicalView.getController())
  , m_gameArguments(std::move(gameLobbyStateArguments))
  , m_callbacks(std::make_shared<GameLobbyCallbacks>(*this))
{
  // Connect GameChoosing callbacks to packet handler
  m_controller.getPacketHandler()->connectCallbacks(m_callbacks.get());

  // Create thread to send heartbeats to keep lobby alive
  m_lobbyHeartbeatThread = std::thread(&GameLobbyState::_lobbyHeartbeat, this);
}


GameLobbyState::~GameLobbyState()
{
  m_runLobbyHeartbeatThread = false;
  m_sendLobbyHeartbeat = false;
  m_lobbyHeartbeatThread.join();
}


void GameLobbyState::init()
{
  LOG(DEBUG) << "[GameLobbyState]::init";
  LOG(DEBUG) << "[GameLobbyState] args: " << m_gameArguments.gameName;
}


void GameLobbyState::eventHandling()
{
  // Process events
  sf::Event event{};
  while (m_gameWindow.pollEvent(event))
  {
    // Handle ImGui events
    ImGui::SFML::ProcessEvent(m_gameWindow, event);

    // Close window: exit
    if (event.type == sf::Event::Closed) {
      m_gameWindow.close();
      m_graphicalView.stop();
      break;
    }
  }
}


void GameLobbyState::display()
{
  ImGui::SFML::Update(m_gameWindow, m_deltaClock.restart());

  // TODO: Test-purpose button only
  if(ImGui::Button("Return to game selection"))
  {
    m_graphicalView.changeState(States::GameChoosing);
  }

  //ImGui::SetCursorPos(ImVec2(-FLT_MIN, 60.f));
  switch(m_lobbyState){
    case LobbyState::Main:
      m_sendLobbyHeartbeat = false;
      _guiDisplayMainGui();
      break;
    case LobbyState::JoinLobby:
      m_sendLobbyHeartbeat = false;
      _guiDisplayJoinLobby();
      break;
    case LobbyState::CreateLobby:
      m_sendLobbyHeartbeat = false;
      _guiDisplayCreateNewLobby();
      break;
    case LobbyState::LobbyDetails:
      m_sendLobbyHeartbeat = true;
      _guiDisplayLobby();
      break;
  }

  //m_gameWindow.clear(sf::Color(55, 55, 55));
  m_gameWindow.clear(sf::Color::Magenta);

  ImGui::SFML::Render(m_gameWindow);
  m_gameWindow.display();
}


void GameLobbyState::updateLobbyDetails(const nlohmann::json& updateJson)
{
  m_lobbyDetailsJson = updateJson;
}


void GameLobbyState::_guiDisplayMainGui()
{
  if(ImGui::Button("Create Lobby"))
  {
    m_lobbyState = LobbyState::CreateLobby;
  }

  if(ImGui::Button("Join Lobby"))
  {
    nlohmann::json requestJson;
    requestJson["GameKey"] = m_gameArguments.gameName;
    m_controller.sendRequest(PacketType::ListOpenLobbies, requestJson.dump());
    m_lobbyState = LobbyState::JoinLobby;
  }
}


void GameLobbyState::_guiDisplayLobby()
{
  ImGui::BeginTable("Players", 1);
  ImGui::TableSetupColumn("Player");
  ImGui::TableHeadersRow();
  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  try {
    auto clientsIDs = m_lobbyDetailsJson.at("ClientIDs").get<std::vector<size_t>>();
    auto creatorID = m_lobbyDetailsJson.at("CreatorID").get<size_t>();

    for (const auto clientID: clientsIDs) {
      if (clientID == creatorID) {
        ImGui::Text("[Owner] %lu", clientID);
      } else {
        ImGui::Text("%lu", clientID);
      }
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
    }
  } catch (std::exception& e) {
  }
  ImGui::EndTable();

  // Back button
  if(ImGui::Button("Back"))
  {
    m_lobbyState = LobbyState::Main;
  }
}


void GameLobbyState::_guiDisplayJoinLobby()
{
  ImGui::BeginTable("Lobbies", 3);
  ImGui::TableSetupColumn("Lobby Name");
  ImGui::TableSetupColumn("Players");
  ImGui::TableHeadersRow();
  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  try {
    auto lobbiesJson = m_lobbiesListJson.at("Lobbies");

    for (const auto& lobbyJson: lobbiesJson) {
      auto lobbyNameStr = lobbyJson.at("LobbyName").get<std::string>();
      auto lobbyMinPlayers = lobbyJson.at("MinPlayers").get<int>();
      auto lobbyCurrentPlayers = lobbyJson.at("CurrentPlayers").get<int>();
      auto lobbyMaxPlayers = lobbyJson.at("MaxPlayers").get<int>();
      ImGui::Text("%s", lobbyNameStr.c_str());
      ImGui::TableNextColumn();
      ImGui::Text("[%d / %d] (min. %d)", lobbyCurrentPlayers, lobbyMaxPlayers, lobbyMinPlayers);
      ImGui::TableNextColumn();
      if (ImGui::Button("Join -->")) {
        LOG(DEBUG) << "Join for " << shared::getClientInfo().getId() << " pushed!";
        LOG(DEBUG) << lobbyJson.dump(4);

        nlohmann::json requestJson;
        requestJson["CreatorID"] = lobbyJson.at("CreatorID").get<size_t>();
        m_controller.sendRequest(PacketType::JoinLobby, requestJson.dump());
      }
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
    }
  } catch (std::exception& e) {
  }
  ImGui::EndTable();

  // Back button
  if(ImGui::Button("Back"))
  {
    m_lobbyState = LobbyState::Main;
  }
}


void GameLobbyState::_guiDisplayCreateNewLobby()
{
  constexpr size_t MAX_LOBBY_NAME_SIZE = 32;
  static std::string lobbyName;

  ImGui::InputText("Lobby name", &lobbyName);
  if (ImGui::Button("Create lobby"))
  {
    LOG(DEBUG) << lobbyName;
    if (lobbyName.empty()) {
      m_createNewLobbyErrorString = "Lobby name cannot be empty!";
    } else if (lobbyName.size() > MAX_LOBBY_NAME_SIZE) {
      m_createNewLobbyErrorString = "Lobby name too long!";
    } else {
      nlohmann::json createLobbyRequestJson;
      createLobbyRequestJson["LobbyName"] = lobbyName;
      createLobbyRequestJson["GameKey"] = m_gameArguments.gameName;

      m_controller.sendRequest(PacketType::CreateLobby, createLobbyRequestJson.dump());
    }
  }

  ImGui::Text("%s", m_createNewLobbyErrorString.c_str());

  // Back button
  if(ImGui::Button("Back"))
  {
    m_lobbyState = LobbyState::Main;
  }
}


void GameLobbyState::updateLobbiesList(const nlohmann::json &updateJson)
{
  m_lobbiesListJson = updateJson;
}


void GameLobbyState::_lobbyHeartbeat()
{
  constexpr size_t HeartbeatSleepTime = 10;

  while (m_runLobbyHeartbeatThread) {
    if (m_sendLobbyHeartbeat) {
      m_controller.sendRequest(PacketType::LobbyHeartbeat);
      LOG(DEBUG) << "Sending lobby heartbeat";
    }

    std::this_thread::sleep_for(std::chrono::seconds(HeartbeatSleepTime));
  }
}

} // namespace