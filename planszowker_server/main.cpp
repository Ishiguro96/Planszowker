#include <string>
#include <fstream>
#include <sstream>

#include <ErrorHandler/ExceptionThrower.h>
#include <Supervisor/Supervisor.h>

#include <easylogging++.h>

using namespace pla;
using namespace pla::supervisor;
using namespace pla::err_handler;

INITIALIZE_EASYLOGGINGPP

static constexpr auto CONFIG_FILENAME = "config.plameta";

////////////
// SERVER //
////////////
int main() {

  std::ifstream configFile {CONFIG_FILENAME};

  el::Configurations customConf;
  customConf.setToDefault();
  customConf.set(el::Level::Debug, el::ConfigurationType::Format, "[%levshort, %loc]: %msg");
  el::Loggers::reconfigureLogger("default", customConf);

  if (!configFile.is_open()) {
    LOG(ERROR) << "Cannot open " << CONFIG_FILENAME << " file!";
    return EXIT_FAILURE;
  }

  try {
    std::stringstream configStream;
    configStream << configFile.rdbuf();

    Supervisor serverSupervisor {std::move(configStream)};
    serverSupervisor.run();
  } catch (ExceptionThrower& e) {
    LOG(INFO) << "Terminating Planszówker Server due to errors...";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
