#include <Parser.h>

#include <ErrorHandler/ErrorLogger.h>
#include <easylogging++.h>

#include <fstream>
#include <regex>

// debug
#include <iostream>

namespace pla::utils::plameta {

Parser::Parser(std::ifstream& plametaFileStream)
  : m_plametaFileStream(plametaFileStream)
{
  _setValidEntries();

  std::string readLine;
  std::string currentSection {"global"};

  while (std::getline(m_plametaFileStream, readLine)) {
    std::regex sectionRegex {"^\\[(.+)\\]$"}; // Match for [section]
    std::smatch results;
    if (std::regex_match(readLine, results, sectionRegex)) {
      LOG(DEBUG) << "Current section: " << currentSection;
      currentSection = results.str(1); // Change current key if new section has been encountered
      continue;
    }

    // If we haven't found new section, it means we have to look for new entry
    // entry-name: entry-value
    std::regex entryRegex {R"(^([a-zA-Z0-9_\-]+):[\s]{1}([a-zA-Z0-9_\s\-\.]+)$)"};
    if (std::regex_match(readLine, results, entryRegex)) {
      auto key = results.str(1);
      auto value = results.str(2);

      LOG(DEBUG) << "Key " << key << " with value " << value;

      // We should check if read entry is valid
      bool valid = false;
      EntryType type = EntryType::Unknown;
      for (const auto& validEntry : m_validEntries) {
        if (std::get<0>(validEntry) == key) {
          valid = true;
          type = std::get<1>(validEntry);
          break;
        }
      }

      if (valid) {
        LOG(DEBUG) << "Found valid key " << key << " with value " << value << ". Adding new entry...";

        auto sectionIt = m_entries.find(currentSection);
        if (sectionIt == m_entries.end()) {
          LOG(DEBUG) << "We don't have that section...";
          // We don't have any entries with global key yet...
          auto [it, inserted] = m_entries.insert({currentSection, {}});
          if (inserted) {
            sectionIt = it;
          } else {
            LOG(ERROR) << "Parser: Cannot insert entry!";
            common::err_handler::ErrorLogger::throwError();
          }
        }

        auto& keys = sectionIt->second;
        auto entryPtr = std::make_shared<Entry>(key, value, type);

        keys.push_back(std::move(entryPtr));
      }
    }
  }


  for (const auto& globalKey : m_entries) {
    LOG(DEBUG) << "Global key " << globalKey.first;
    for (const auto& keyEntry : globalKey.second) {
      auto type = keyEntry->getType();
      switch (type) {
        case EntryType::String:
          LOG(DEBUG) << keyEntry->getKey() << ": " << keyEntry->getValue<std::string>();
          break;
        case EntryType::Int:
          LOG(DEBUG) << keyEntry->getKey() << ": " << keyEntry->getValue<int>();
          break;
        case EntryType::Float:
          LOG(DEBUG) << keyEntry->getKey() << ": " << keyEntry->getValue<float>();
          break;
        default:
          LOG(ERROR) << "Wrong parameter type!";
          common::err_handler::ErrorLogger::throwError();
          break;
      }
    }
  }


}


void Parser::_setValidEntries()
{
  m_validEntries.emplace_back("name", EntryType::String);
  m_validEntries.emplace_back("description", EntryType::String);
  m_validEntries.emplace_back("distributor", EntryType::String);
  m_validEntries.emplace_back("author", EntryType::String);
  m_validEntries.emplace_back("version", EntryType::String);
  m_validEntries.emplace_back("min_players", EntryType::Int);
  m_validEntries.emplace_back("max_players", EntryType::Int);
  m_validEntries.emplace_back("port", EntryType::Int);
}


std::shared_ptr<Entry> Parser::operator[] (const std::string& key)
{
  // Key is in given format:
  // section:entry
  auto pos = key.find(':');
  std::string section = key.substr(0, pos);
  std::string entryKey = key.substr(pos + 1, std::string::npos);

  LOG(DEBUG) << "Section: " << section;
  LOG(DEBUG) << "Entry key: " << entryKey;

  auto it = m_entries.find(section);
  if (it == m_entries.end()) {
    return nullptr;
  }

  auto& entries = it->second;
  for (const auto& entry : entries) {
    if (entry->getKey() == entryKey) {
      return entry;
    }
  }

  return nullptr;
}

} // Namespace
