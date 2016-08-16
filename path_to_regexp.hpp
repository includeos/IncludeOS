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

#ifndef PATH_TO_REGEXP_HPP
#define PATH_TO_REGEXP_HPP

#include <regex>
#include <string>
#include <vector>
#include <map>

namespace route {

struct Token {
	std::string name;	// can be a string or an int (index)
	std::string prefix;
	std::string delimiter;
	bool optional;
	bool repeat;
	bool partial;
	bool asterisk;
	std::string pattern;
	bool is_string;	// If it is a string we only put/have a string in the name-attribute (path in parse-method)
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

class PathToRegexp {

public:
	PathToRegexp(const std::string& path, std::vector<Token>& keys,
		const std::map<std::string, bool>& options = std::map<std::string, bool>());

	const std::vector<Token> parse(const std::string& str) const noexcept;

	const std::string escape_group(const std::string& group) const;

	const std::string escape_string(const std::string& str) const;

	const std::regex tokens_to_regexp(const std::vector<Token>& tokens,
		const std::map<std::string, bool>& options) const;

	// std::regex attach_keys(const std::regex& re, const std::vector<Token>& keys);

	// std::string flags(const std::map<std::string, bool>& options);

	inline const std::regex& get_regex() const { return regex_result; }

private:
	static const std::regex PATH_REGEXP;
	std::regex regex_result;

};  // < class PathToRegexp

};	// < namespace route

#endif	// < PATH_TO_REGEXP_HPP
