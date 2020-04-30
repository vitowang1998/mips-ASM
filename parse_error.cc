#include "parse_error.h"

ParseError::ParseError(std::string err): content{std::move(err)} {}
