#include "ConsoleView.h"

#include "DiceRoller/Version.h"
#include "DiceRoller/ConsoleViewCallbackObject.h"
#include "DiceRoller/Objects.h"

#include <iostream>
#include <chrono>
#include <string>
#include <atomic>

using namespace pla::common::games;

namespace pla::common::games::dice_roller {

void DiceRollerConsoleView::update(const std::any& object) {
  auto receivedObject = std::any_cast<std::string>(object);

  std::cout << receivedObject << std::endl;

  // TODO: Remove below printout...
  //std::cout << "Received additional info: " << receivedObject.additionalInfo << "\n";
  //std::cout << "Reply: " << static_cast<size_t>(receivedObject.reply) << "\n";
}


void DiceRollerConsoleView::init() {
  // No mutex is needed here, since it is invoked before any multithreading

  // Some basic info about a game
  std::cout << "DiceRoller v" << DiceRollerVersionMajor << "." << DiceRollerVersionMinor << "." << DiceRollerVersionPatch << "\n";
  std::cout << "========================================\n";
  std::cout << "Choose an action. You can reroll up to 1 time. Points are summed. Player with a greater score wins.\n";
  std::cout << "========================================\n";
}


void DiceRollerConsoleView::notifyController(std::function<void(std::any&)> callback) {
  // TODO: It is just for testing
  DiceRollerRequest requestToSend;

  requestToSend.type = static_cast<DiceRollerRequestType>(m_inputType);

  auto request = std::make_any<DiceRollerRequest>(requestToSend);

  //std::cout << "Sending request type of value: " << static_cast<int>(std::any_cast<DiceRollerRequest>(requestToSend).type) << "\n";
  callback(request);
}


void DiceRollerConsoleView::runLoop(Controller* controller, std::atomic_bool& runLoop)
{
  while (runLoop)
  {
    // Wait for input
    std::cout << "Packet to send:\n (1) Roll dice\n (2) Re-roll dice\n (3) Confirm\n Your choice: ";
    std::cin >> m_inputType;

    if (m_inputType < 1 || m_inputType > 3) {
      std::cout << "Wrong packet type!\n";
      continue;
    }

    std::function<void(std::any&)> callback = std::bind(&Controller::viewCallback, controller, std::placeholders::_1);
    notifyController(callback);
  }
}

} // namespaces
