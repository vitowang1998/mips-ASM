#ifndef PARSE_ERROR_H
#define PARSE_ERROR_H
#include <string>

class ParseError {
  private:
    std::string content;
  public:
    ParseError(std::string err);
    inline const std::string &getMessage() const { return content; }
};
#endif // PARSE_ERROR_H
