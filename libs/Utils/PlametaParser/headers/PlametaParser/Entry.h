#pragma once

#include <string>
#include <utility>
#include <any>

namespace pla::utils::plameta {

enum class EntryType : uint8_t {
  Unknown,
  String,
  Int,
  Float
};

/*!
 * @brief PlaMeta Entry class
 * This class is responsible for holding a single PlaMeta's entry.
 *
 * @ingroup plameta
 */
class Entry
{
public:
  using EntryValue = std::any;

  /*!
   * @brief Entry class constructor
   *
   * @param[in] key Entry's key. Entries are indexes by keys.
   * @param[in] rawValue Value read directly from `.plameta` file.
   * @param[in] type Entry's type. It will be converted according to this type.
   */
  Entry(std::string key, std::string rawValue, EntryType type);

  /*!
   * @brief Get Entry's type
   *
   * @return Entry's type enum.
   */
  [[nodiscard]] [[maybe_unused]]
  EntryType getType() { return m_type; }

  /*!
   * @brief Get Entry's key
   *
   * @return Entry's key.
   */
  [[nodiscard]] [[maybe_unused]]
  std::string getKey() { return m_key; }

  /*!
   * @brief Get Entry's raw value
   *
   * @return Entry's raw value (read directly from `.plameta` file).
   */
  [[nodiscard]] [[maybe_unused]]
  std::string getRaw() { return m_rawValue; }

  /*!
   * @brief Get Entry's value
   *
   * @return Entry's value as `std::any`.
   */
  [[nodiscard]]
  EntryValue getValue();

private:
  std::string m_key;
  std::string m_rawValue;
  EntryType m_type;
};

} // Namespace
