#include <States/GameChoosingState.h>

#include <Games/Callbacks/GameChoosingCallbacks.h>
#include <GamesClient/SharedObjects.h>
#include <Games/States/GameLobbyState.h>

#include <easylogging++.h>

#include <cmath>

namespace pla::games {

using namespace pla::games_client;

GameChoosingState::GameChoosingState(games_client::GraphicalView& graphicalView)
  : m_graphicalView(graphicalView)
  , m_gameWindow(*graphicalView.getGameWindow())
  , m_controller(graphicalView.getController())
  , m_callbacks(std::make_shared<GameChoosingCallbacks>(*this))
{
  // Connect GameChoosing callbacks to packet handler
  m_controller.getPacketHandler()->connectCallbacks(m_callbacks.get());
}


void GameChoosingState::init()
{
  m_controller.sendRequest(games::PacketType::ID);
  m_controller.sendRequest(games::PacketType::ListAvailableGames);
}


void GameChoosingState::eventHandling()
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

    // TODO: Remove it later
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::G) {
      shared::getGamesMetaInfo().clear();

      m_controller.sendRequest(games::PacketType::ListAvailableGames);
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::H) {
      LOG(DEBUG) << "Content of m_gamesMetaInfo:";
      for (const auto& ziu : shared::getGamesMetaInfo().getPlametaParsers()) {
        LOG(DEBUG) << ziu.first;
      }
    }
  }
}


void GameChoosingState::display()
{
  ImGui::SFML::Update(m_gameWindow, m_deltaClock.restart());

  displayGameTile();

  m_gameWindow.clear(sf::Color(55, 55, 55));

  ImGui::SFML::Render(m_gameWindow);
  m_gameWindow.display();
}


void GameChoosingState::updateAvailableGames(const std::string& combinedString)
{
  shared::getGamesMetaInfo().addMetaData(combinedString);
}


void GameChoosingState::displayGameTile()
{
  constexpr float PADDING = 30.f; // Padding used to display main game choosing window

  // Main game choosing window is split into smaller children
  // Amount of children depends on the main window size
  //
  // =========================================
  // |   ======   ======   ======   ======   |
  // |   | GA |   | GA |   | GA |   | GA |   |
  // |   | ME |   | ME |   | ME |   | ME |   |
  // |   ======   ======   ======   ======   |
  // |                                       |
  // |   ======   ======   ======   ======   |
  // |   | GA |   | GA |   | GA |   | GA |   |
  // |   | ME |   | ME |   | ME |   | ME |   |
  // |   ======   ======   ======   ======   |
  // =========================================


  // Set main game choosing window position and size
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
  ImGui::Begin("Main Game Choosing Window", nullptr, ImGuiWindowFlags_NoTitleBar |
                                                                        ImGuiWindowFlags_NoCollapse |
                                                                        ImGuiWindowFlags_NoResize |
                                                                        ImGuiWindowFlags_NoMove);

  constexpr float GAME_ENTRY_WINDOW_WIDTH = 300.f;
  constexpr float GAME_ENTRY_WINDOW_HEIGHT = 360.f;
  auto mainGameEntryWindowSize = ImGui::GetWindowSize();
  auto mainGameEntryWindowPos = ImGui::GetWindowPos();

  float numberOfEntriesInRow = std::floor(mainGameEntryWindowSize.x / (GAME_ENTRY_WINDOW_WIDTH + 2.f * PADDING));
  float interEntryPadding = 0.f;
  if (numberOfEntriesInRow >= 2.f) {
    interEntryPadding = (mainGameEntryWindowSize.x - GAME_ENTRY_WINDOW_WIDTH * numberOfEntriesInRow - 2.f * PADDING) / (numberOfEntriesInRow - 1.f);
  }

  ImGui::Indent(PADDING);

  size_t childrenCounter = 0;
  for (const auto &plameta: shared::getGamesMetaInfo().getPlametaParsers()) {
    const auto &key = plameta.first;
    const auto &parser = plameta.second;

    std::string gameName = std::any_cast<std::string>(parser["overview:name"]->getValue());
    std::cout << gameName.c_str() << "\n";
    /// Set child's window name to name received in `.plameta` file
    ImGui::BeginChild(gameName.c_str(),
                      ImVec2(GAME_ENTRY_WINDOW_WIDTH, GAME_ENTRY_WINDOW_HEIGHT),
                      ImGuiChildFlags_Border);

    ImGui::PushFont(m_graphicalView.getFontManager().getFont("Roboto-Light-24px"));
    float gameNameWidth = ImGui::CalcTextSize(gameName.c_str()).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - gameNameWidth) * 0.5f);
    ImGui::Text("%s", gameName.c_str());
    ImGui::Separator();
    ImGui::PopFont();

    ImGui::PushFont(m_graphicalView.getFontManager().getFont("Roboto-Light-18px"));

    const auto &thumbnails = shared::getGamesMetaInfo().getThumbnails();
    auto thumbnailIter = thumbnails.find(key);
    const sf::Vector2f thumbnailSize{220.f, 220.f};
    if (thumbnailIter == std::end(thumbnails)) {
      // We have NOT found special thumbnail file
      try {
        ImGui::SetCursorPosX((ImGui::GetContentRegionMax().x - thumbnailSize.x ) / 2.f);
        ImGui::Image(*(thumbnails.at("DefaultThumbnail")), thumbnailSize);
      } catch (const std::out_of_range &e) {}
    } else {
      ImGui::SetCursorPosX((ImGui::GetContentRegionMax().x - thumbnailSize.x ) / 2.f);
      ImGui::Image(*(thumbnailIter->second), thumbnailSize);
    }

    if(ImGui::Button("Play", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.f)))
    {
      GameLobbyStateArguments arg;
      arg.gameName = key;
      m_graphicalView.changeState(States::GameLobby, std::any(arg));
    }
    ImGui::SameLine();
    ImGui::Button("Info", ImVec2(-FLT_MIN, 0.f));

    ImGui::PopFont();
    ImGui::EndChild();

    auto numberOfEntriesInRowI = static_cast<size_t>(numberOfEntriesInRow);
    if ((childrenCounter % numberOfEntriesInRowI) < (numberOfEntriesInRowI - 1)) {
      auto childN = static_cast<float>((childrenCounter % numberOfEntriesInRowI) + 1);
      ImGui::SameLine(mainGameEntryWindowPos.x + PADDING + childN * GAME_ENTRY_WINDOW_WIDTH + childN * interEntryPadding);
    } else {
      // We have reached row limit - create a separator
      ImGui::NewLine();
    }

    ++childrenCounter;
  }
  ImGui::End();
}

} // namespace
