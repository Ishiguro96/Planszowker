#include <Parser.h>

#include <ErrorHandler/ErrorLogger.h>
#include <easylogging++.h>

#include <regex>

// debug
#include <iostream>

namespace pla::utils::plameta {

Parser::Parser(std::stringstream plametaContent)
  : m_plametaContent(std::move(plametaContent))
{
  _setValidEntries();

  std::string readLine;
  std::string currentSection {"global"};

  while (std::getline(m_plametaContent, readLine)) {
    // TODO: Matching this regex on Windows with `$` character at the end doesn't work.
    // I'm not sure why, my guess is that `\r\n` is messing things up. To be investigated.
    std::regex sectionRegex {"^\\[(.+)\\]"}; // Match for [section]
    std::smatch results;
    if (std::regex_search(readLine, results, sectionRegex)) {
      LOG(DEBUG) << "Previous section: " << currentSection;
      currentSection = results.str(1); // Change current key if new section has been encountered
      LOG(DEBUG) << "New section: " << currentSection;
      continue;
    }

    // If we haven't found new section, it means we have to look for new entry
    // entry-name: entry-value
    std::regex entryRegex {R"(^([a-zA-Z0-9_\-]+):[\s]{1}([a-zA-Z0-9_\s\-\.,]+)$)"};
    if (std::regex_search(readLine, results, entryRegex)) {
      auto key = results.str(1);
      auto value = results.str(2);

      LOG(DEBUG) << "Key <" << key << "> with value <" << value << ">";

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
        LOG(DEBUG) << "Found valid key <" << key << "> with value <" << value << ">. Adding new entry...";

        auto sectionIt = m_entries.find(currentSection);
        if (sectionIt == m_entries.end()) {
          LOG(DEBUG) << "We don't have <" << currentSection << "> section in container. Creating one...";
          // If we don't have specific global section in container, we need to add it there
          auto [it, inserted] = m_entries.insert({currentSection, {}});
          if (inserted) {
            sectionIt = it;
          } else {
            LOG(ERROR) << "Parser: Cannot insert entry!";
            err_handler::ErrorLogger::throwError();
          }
        }

        auto& keys = sectionIt->second;
        LOG(DEBUG) << "\t > Pushing new entry [" << currentSection << "]:" << key << " with value: " << value;
        auto entryPtr = std::make_shared<Entry>(key, value, type);

        keys.push_back(std::move(entryPtr));
      }
    }
  }
}

Parser::Parser(const Parser& other)
{
  this->m_validEntries = other.m_validEntries;
  this->m_entries = other.m_entries;
  this->m_plametaContent << other.m_plametaContent.str();
}

void Parser::_setValidEntries()
{
  m_validEntries.emplace_back("name", EntryType::String, "N/A");
  m_validEntries.emplace_back("description", EntryType::String, "N/A");
  m_validEntries.emplace_back("distributor", EntryType::String, "N/A");
  m_validEntries.emplace_back("author", EntryType::String, "N/A");
  m_validEntries.emplace_back("version", EntryType::String, "N/A");
  m_validEntries.emplace_back("min_players", EntryType::Int, "0");
  m_validEntries.emplace_back("max_players", EntryType::Int, "0");
  m_validEntries.emplace_back("port", EntryType::Int, "0");
}


std::shared_ptr<Entry> Parser::operator[] (const std::string& key) const
{
  // Key is in given format:
  // section:entry
  auto pos = key.find(':');
  std::string section = key.substr(0, pos);
  std::string entryKey = key.substr(pos + 1, std::string::npos);

  auto it = m_entries.find(section);
  if (it != m_entries.end()) {
    auto& entries = it->second;

    for (const auto& entry : entries) {
      if (entry->getKey() == entryKey) {
        return entry;
      }
    }
  }

  // If we haven't found given entry
  for (const auto& validEntry : m_validEntries) {
    if (std::get<0>(validEntry) == entryKey) {

      // We get default value
      std::shared_ptr<Entry> returnPtr = std::make_shared<Entry>(std::get<0>(validEntry),
                                                                 std::get<2>(validEntry),
                                                                 std::get<1>(validEntry));
      return std::move(returnPtr);
    }
  }

  // Throw exception if nullptr is returned
  err_handler::ErrorLogger::throwError();
  return nullptr;
}

} // Namespace
