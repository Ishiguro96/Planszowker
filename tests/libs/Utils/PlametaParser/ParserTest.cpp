#include <gtest/gtest.h>

#include <PlametaParser/Parser.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>

#include <easylogging++.h>

INITIALIZE_EASYLOGGINGPP

namespace {

using namespace pla;

class ParserTest : public testing::Test {
  void SetUp() override {
    // Create valid Plameta content
    m_validPlameta << "[" << OVERVIEW_SECTION << "]\n"
                   << NAME_ENTRY_KEY << ": " << NAME_ENTRY_VALUE << "\n"
                   << AUTHOR_ENTRY_KEY << ": " << OTHER_ENTRY_VALUE << "\n"
                   << "\n"
                   << "[" << OTHER_GLOBAL_SECTION << "]\n"
                   << SECRET_KEY_KEY << ": " << SECRET_KEY_VALUE << "\n"
                   << FLOAT_KEY_KEY << ": " << FLOAT_KEY_VALUE << "\n"
                   << INT_KEY_KEY << ": " << INT_KEY_VALUE << "\n";
  }

protected:
  //std::vector<std::tuple<std::string, std::string>>
  std::string OVERVIEW_SECTION = "overview";
  std::string NAME_ENTRY_KEY = "name";
  std::string NAME_ENTRY_VALUE = "Parser UT";
  std::string AUTHOR_ENTRY_KEY = "author";
  std::string OTHER_ENTRY_VALUE = "Author value";

  std::string OTHER_GLOBAL_SECTION = "other_global_section";
  std::string SECRET_KEY_KEY = "version";
  std::string SECRET_KEY_VALUE = "Version value";
  std::string FLOAT_KEY_KEY = "min_players";
  std::string FLOAT_KEY_VALUE = "123";
  std::string INT_KEY_KEY = "invalid_key";
  std::string INT_KEY_VALUE = "123";

  std::stringstream m_validPlameta;
};

TEST_F(ParserTest, ParseValidPlameta) {
  utils::plameta::Parser parser{std::move(m_validPlameta)};

  EXPECT_EQ(std::any_cast<std::string>(parser[OVERVIEW_SECTION + ":" + NAME_ENTRY_KEY]->getValue()), NAME_ENTRY_VALUE);
  // EXPECT_EQ(parser[OVERVIEW_SECTION + ":" + AUTHOR_ENTRY_KEY], OTHER_ENTRY_VALUE);
  // EXPECT_EQ(parser[OTHER_GLOBAL_SECTION + ":" + SECRET_KEY_KEY], SECRET_KEY_VALUE);
  // EXPECT_EQ(parser[OTHER_GLOBAL_SECTION + ":" + FLOAT_KEY_KEY], FLOAT_KEY_VALUE);
  // EXPECT_EQ(parser[OTHER_GLOBAL_SECTION + ":" + INT_KEY_KEY], INT_KEY_VALUE);
}

int main() {
  el::Configurations customConf;
  customConf.setToDefault();
  customConf.set(el::Level::Debug, el::ConfigurationType::Format, "[%level]: %msg");
  el::Loggers::reconfigureLogger("default", customConf);

  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

}
