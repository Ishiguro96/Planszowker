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

int main() {
  el::Configurations customConf;
  customConf.setToDefault();
  customConf.set(el::Level::Debug, el::ConfigurationType::Format, "[%levshort, %loc]: %msg");
  el::Loggers::reconfigureLogger("default", customConf);

  try {
    // Designer port is defaulted to 27017
    std::stringstream configStream;
    configStream << "[config]\nport:27017";

    Supervisor serverSupervisor {std::move(configStream)};
    serverSupervisor.run();
  } catch (ExceptionThrower& e) {
    LOG(WARNING) << "Terminating Planszówker Designer due to errors...";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
