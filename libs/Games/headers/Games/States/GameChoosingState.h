#pragma once

#include "IState.h"

#include <Games/Callbacks/ICallbacks.h>
#include <Games/GameWindow.h>
#include <GamesClient/GraphicalView.h>
#include <NetworkHandler/ClientPacketHandler.h>
#include <Games/GamesMetaInfo.h>

#include <vector>

namespace pla::games {

class GameChoosingState final : public IState
{
public:
  GameChoosingState() = delete;
  explicit GameChoosingState(games_client::GraphicalView& graphicalView);

  void eventHandling() final;
  void display() final;
  void init() final;

  void updateAvailableGames(const std::string& combinedString);
private:
  const sf::Vector2f ThumbnailSize = {20.f, 180.f};

  games_client::GraphicalView& m_graphicalView;
  GameWindow& m_gameWindow;
  games_client::Controller& m_controller;

  std::shared_ptr<ICallbacks> m_callbacks;

  GamesMetaInfo m_gamesMetaInfo;
  std::atomic_bool m_updateMetaInfo {false}; ///< Used to indicate whether all games info should be updated (re-rendered)
};

} // namespace
