#pragma once

#include <string>
#include <vector>

namespace Video2Card::Utils
{

  class Base64Utils
  {
public:

    // Encode binary data to base64 string
    static std::string Encode(const std::vector<unsigned char>& data);

    // Decode base64 string to binary data
    static std::vector<unsigned char> Decode(const std::string& encoded);

private:

    static const std::string base64_chars;
    static inline bool IsBase64(unsigned char c);
  };

} // namespace Video2Card::Utils
