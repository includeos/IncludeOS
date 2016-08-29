// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// https://github.com/pillarjs/path-to-regexp/blob/master/index.js

#ifndef PATH_TO_REGEX_HPP
#define PATH_TO_REGEX_HPP

#include <regex>
#include <string>
#include <vector>
#include <map>

namespace route {

struct Token {
	std::string name;	// can be a string or an int (index)
	std::string prefix;
	std::string delimiter;
	std::string pattern;
  bool        optional;
  bool        repeat;
  bool        partial;
  bool        asterisk;
	bool        is_string;	// If it is a string we only put/have a string in the name-attribute (path in parse-method)
	               	// So if this is true, we can ignore all attributes except name

  void set_string_token(const std::string& name_) {
    name = name_;
    prefix = "";
    delimiter = "";
    optional = false;
    repeat = false;
    partial = false;
    asterisk = false;
    pattern = "";
    is_string = true;
  }
};	// < struct Token

class PathToRegex {

public:

  /**
   *  Creates a path-regex from string input (path)
   *  Updates keys-vector (empty input parameter)
   *
   *  Calls parse-method and then tokens_to_regex-method based on the tokens returned from parse-method
   *  Puts the tokens that are keys (not string-tokens) into keys-vector
   *
   *  std::vector<Token> keys (empty)
   *    Is created outside the class and sent in as parameter
   *    One Token-object in the keys-vector per parameter found in the path
   *
   *  std::map<std::string, bool> options (optional)
   *    Can contain bool-values for the keys "sensitive", "strict" and/or "end"
   *    Default:
   *      strict = false
   *      sensitive = false
   *      end = true
   */
	static std::regex path_to_regex(const std::string& path, std::vector<Token>& keys,
		const std::map<std::string, bool>& options = std::map<std::string, bool>());

  /**
   *  Creates a path-regex from string input (path)
   *
   *  Calls parse-method and then tokens_to_regex-method based on the tokens returned from parse-method
   *
   *  std::map<std::string, bool> options (optional)
   *    Can contain bool-values for the keys "sensitive", "strict" and/or "end"
   *    Default:
   *      strict = false
   *      sensitive = false
   *      end = true
   */
  static std::regex path_to_regex(const std::string& path,
    const std::map<std::string, bool>& options = std::map<std::string, bool>());

  /**
   *  Creates vector of tokens based on the given string (this vector of tokens can be sent as
   *  input to tokens_to_regex-method and includes tokens that are strings, not only tokens
   *  that are parameters in str)
   */
  static std::vector<Token> parse(const std::string& str);

  /**
   *  Creates a regex based on the tokens and options (optional) given
   */
  static std::regex tokens_to_regex(const std::vector<Token>& tokens,
    const std::map<std::string, bool>& options = std::map<std::string, bool>());

  /**
   *  Goes through the tokens-vector and push all tokens that are not string-tokens
   *  onto keys-vector
   */
  static void tokens_to_keys(const std::vector<Token>& tokens, std::vector<Token>& keys);

private:
	static const std::regex PATH_REGEXP;

};  // < class PathToRegex

};	// < namespace route

#endif	// < PATH_TO_REGEX_HPP
