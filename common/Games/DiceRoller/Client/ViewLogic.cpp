#include "ViewLogic.h"

namespace pla::common::games::dice_roller {

void DiceRollerViewLogic::updateModel(const std::any& modelUpdateData)
{

}

const std::any& DiceRollerViewLogic::getUpdatedModel()
{
  return std::make_any<bool>(true);
}

} // namespaces
