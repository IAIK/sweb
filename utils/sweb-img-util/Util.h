/**
 * Filename: Util.h
 * Description:
 *
 * Created on: 05.09.2012
 * Author: chris
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <sstream>

class Util
{
public:

  //----------------------------------------------------------------------------
  /// converts a std::string into a given integer value
  ///
  /// @param[in] str std::string to convert
  /// @param[out] value the converted value
  ///
  /// @return true if conversion was successful
  //
  template<class T>
  static bool strToType(const std::string& str, T& value);

private:
  // no instances
  Util();
  Util(const Util& cpy);
  virtual ~Util();
};

//------------------------------------------------------------------------------
template<class T>
bool Util::strToType(const std::string& str, T& value)
{
  std::istringstream stream(str);
  stream >> value;

  if(stream.fail() || !stream.eof())
  {
    return false;
  }
  return true;
}

#endif /* UTIL_H_ */
