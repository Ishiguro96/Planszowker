#include <Callbacks/GameCallbacks.h>

#include <Games/CommObjects.h>
#include <Games/BoardParser.h>
#include <AssetsManager/AssetsReceiver.h>
#include <ErrorHandler/ErrorLogger.h>

#include <easylogging++.h>
#include <nlohmann/json.hpp>

namespace pla::games {

using namespace json_entries;

void GameCallbacks::endTransactionCallback(const std::any& arg)
{
  try {
    auto assetInfoJson = nlohmann::json::parse(std::any_cast<std::string>(arg));
    if (assetInfoJson.at(ASSET_TYPE).get<std::string>() == "BoardDescription") {
      m_state.m_boardParser = std::make_shared<BoardParser>(nlohmann::json::parse(assets::AssetsReceiver::getBoardDescription()));
    }
  } catch (std::exception& e) {
    err_handler::ErrorLogger::printError("[GameCallbacks] Bad any cast!");
  }
}

}