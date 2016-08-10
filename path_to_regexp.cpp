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

#include "path_to_regexp.hpp"

namespace route {

const std::regex PathToRegexp::PATH_REGEXP =
	std::regex{"((\\\\.)|(([\\/.])?(?:(?:\\:(\\w+)(?:\\(((?:\\\\.|[^\\\\()])+)\\))?|\\(((?:\\\\.|[^\\\\()])+)\\))([+*?])?|(\\*))))"};

/*
 *	Create a path regexp from string input
 *
 *	Set regex_result (the resulting regex)
 *	Update keys-vector
 *
 *  vector<Token> keys
 *    Mandatory
 *    Is created outside the class and sent to the constructor
 *    In the constructor (or other method) this keys-vector is filled
 *    One Token-object in the vector per parameter found in the path
 *
 *  map<string, bool> options
 *    Optional
 *    Can contain bool-values for the keys "sensitive", "strict" and/or "end"
 *    Default:
 *      strict = false
 *      sensitive = false
 *      end = true
*/
PathToRegexp::PathToRegexp(const std::string& path, std::vector<Token>& keys,
	const std::map<std::string, bool>& options) {

	std::vector<Token> tokens = parse(path);

	// Set private attribute regex_result:
	regex_result = tokens_to_regexp(tokens, options);

	// Update keys-vector:
	for(size_t i = 0; i < tokens.size(); i++) {
		Token t = tokens[i];

		if(not t.is_string)
			keys.push_back(t);
	}

	// JavaScript needs to use attachKeys method to change keys object (??) but not necessary in C++ (reference)
	// JS: attachKeys(re, keys);
}

// Used by the constructor
// Parse a string for the raw tokens
const std::vector<Token> PathToRegexp::parse(const std::string& str) const {
  if(str.empty()) {
    std::vector<Token> v;
    return v;
  }

	std::vector<Token> tokens;
	int key = 0;
	int index = 0;
	std::string path = "";
	std::smatch res;

	for(std::sregex_iterator i = std::sregex_iterator{str.begin(), str.end(), PATH_REGEXP};
		i != std::sregex_iterator{}; ++i) {

		res = *i;

		std::string m = res[0];			  // the parameter, f.ex. /:test
		std::string escaped = res[2];
		int offset = res.position();

		// JS: path += str.slice(index, offset); from and included index to and included offset-1
		path += str.substr(index, (offset - index));	// from index, number of chars: offset - index

		index = offset + m.size();

		/*printf("Escaped: %s\n", escaped.c_str());
		printf("OFFSET: %d\n", offset);
		printf("INDEX: %d\n", index);
		printf("PATH: %s\n", path.c_str());
		printf("Res[0]/m inside loop (whole element/parameter): %s\n", m.c_str());
    printf("index (offset + m.size()): %d\n", index);
    printf("Res.index/offset: %d\n", offset);*/

		if(not escaped.empty()) {
			path += escaped[1];		// if escaped == \a, escaped[1] == a (if str is "/\\a" f.ex.)
			continue;
		}

		std::string next = ((size_t) index < str.size()) ? std::string{str.at(index)} : "";

		/*printf("STR: %s\n", str.c_str());
		printf("NEXT: %s\n", next.c_str());*/

		std::string prefix = res[4];	// f.ex. /
		std::string name = res[5];		// f.ex. test
		std::string capture = res[6];	// f.ex. \d+
		std::string group = res[7];		// f.ex. (users|admins)
		std::string modifier = res[8];	// f.ex. ?
		std::string asterisk = res[9];	// * if path is /*

		// Push the current path onto the tokens
		if(not path.empty()) {
			Token stringToken;
			stringToken.name = path;
			stringToken.is_string = true;
			tokens.push_back(stringToken);
			path = "";
		}

		bool partial = (not prefix.empty()) and (not next.empty()) and (next not_eq prefix);
		bool repeat = (modifier == "+") or (modifier == "*");
		bool optional = (modifier == "?") or (modifier == "*");
		std::string delimiter = (not prefix.empty()) ? prefix : "/";
		std::string pattern;

		if(not capture.empty())
			pattern = capture;
		else if(not group.empty())
			pattern = group;
		else
			pattern = (not asterisk.empty()) ? ".*" : ("[^" + delimiter + "]+?");

		Token t;
		t.name = (not name.empty()) ? name : std::to_string(key++);
		t.prefix = prefix;
		t.delimiter = delimiter;
		t.optional = optional;
		t.repeat = repeat;
		t.partial = partial;
		t.asterisk = (asterisk == "*");
		t.pattern = escape_group(pattern);
		t.is_string = false;
		tokens.push_back(t);
	}

	// Match any characters still remaining
	if((size_t) index < str.size())
		path += str.substr(index);

	// If the path exists, push it onto the end
	if(not path.empty()) {
		Token stringToken;
		stringToken.name = path;
		stringToken.is_string = true;
		tokens.push_back(stringToken);
	}

	return tokens;
}

// Used by parse-method
/**
 * Escape the capturing group by escaping special characters and meaning.
 *
 * @param  {string} group
 * @return {string}
 */
const std::string PathToRegexp::escape_group(const std::string& group) const {

	// printf("INSIDE ESCAPEGROUP. STR: %s\n", group.c_str());

	// JS: return group.replace(/([=!:$\/()])/g, '\\$1');
	std::regex r{"/([=!:$\\/()])/g"};
	return std::regex_replace(group, r, "\\$1");
}

// Used by tokens_to_regexp-method
/**
 * Escape a regular expression string.
 *
 * @param  {string} str
 * @return {string}
 */
const std::string PathToRegexp::escape_string(const std::string& str) const {

	// TODO
	// escapeGroup(...) is working, but this is not escaping strings? (/route -> \/route f.ex.)

	// printf("INSIDE ESCAPESTRING. STR: %s\n", str.c_str());

	// JS: return str.replace(/([.+*?=^!:${}()[\]|\/\\])/g, '\\$1')
	std::regex r{"/([.+*?=^!:${}()[\\]|\\/\\\\])/g"};
	return std::regex_replace(str, r, "\\$1");
}

// Used by the constructor
// Taking tokens and returning a regex
const std::regex PathToRegexp::tokens_to_regexp(const std::vector<Token>& tokens,
	const std::map<std::string, bool>& options) const {

	// Options:
	// 	strict (bool)      default false
	// 	sensitive (bool)   default false
	// 	end (bool)         default true

  if(tokens.empty())
    return std::regex{""};

	bool strict = options.find("strict")->second;	// Case sensitive..
	bool end = options.find("end")->second;
	std::string route = "";
	Token lastToken = tokens[tokens.size() - 1];
	std::regex re{"(\\/$)"};
	bool endsWithSlash = lastToken.is_string and std::regex_match(lastToken.name, re); // JS: /\/$/.test(lastToken);
	                                               // if the last char in lastToken's name is a slash

	// Iterate over the tokens and create our regexp string
	for(size_t i = 0; i < tokens.size(); i++) {
		Token token = tokens[i];

		if(token.is_string) {

			printf("TOKEN IS STRING\n");
			printf("ROUTE BEFORE ESCAPESTRING: %s\n", route.c_str());
			printf("TOKEN: %s\n", (token.name).c_str());

			route += escape_string(token.name);

			printf("ROUTE AFTER ESCAPESTRING: %s\n", route.c_str());

		} else {

			printf("TOKEN IS NOT STRING\n");

			std::string prefix = escape_string(token.prefix);
			std::string capture = "(?:" + token.pattern + ")";

			if(token.repeat) {
				capture += "(?:" + prefix + capture + ")*";

				printf("TOKEN REPEAT\n");
			}

			if(token.optional) {

				printf("TOKEN OPTIONAL\n");

				if(not token.partial) {
					capture = "(?:" + prefix + "(" + capture + "))?";

					printf("TOKEN PARTIAL\n");
				}
				else {
					capture = prefix + "(" + capture + ")?";

					printf("TOKEN NOT PARTIAL\n");
				}
			} else {
				capture = prefix + "(" + capture + ")";

				printf("TOKEN NOT OPTIONAL\n");
			}

			route += capture;
		}

		printf("END FOR-LOOP. ROUTE: %s\n", route.c_str());
	}

	printf("ROUTE-STRING AFTER FOR-LOOP: %s\n", route.c_str());

	// In non-strict mode we allow a slash at the end of match. If the path to
	// match already ends with a slash, we remove it for consistency. The slash
	// is valid at the end of a path match, not in the middle. This is important
	// in non-ending mode, where "/test/" shouldn't match "/test//route".

	// TODO: TEST: slice -> substr: (endswithslash)

	if(not strict) {
		printf("STRICT IS FALSE\n");
		route = (endsWithSlash ? route.substr(0, (route.size() - 2)) : route) + "(?:\\/(?=$))?";
		printf("ROUTE: %s\n", route.c_str());
	}

	printf("AFTER IF NOT STRICT: ROUTE: %s\n", route.c_str());

/*if (!strict)
	route = (endsWithSlash ? route.slice(0, -2) : route) + '(?:\\/(?=$))?'

// JS:	str.slice(index, offset); from and included index to and included offset-1
   C++:	str.substr(index, (offset - index));	// from index, number of chars: offset - index
*/

	if(end) {
		route += "$";

		printf("END IS TRUE. ROUTE: %s\n", route.c_str());
	} else {
		// In non-ending mode, we need the capturing groups to match as much as
		// possible by using a positive lookahead to the end or next path segment
		route += (strict and endsWithSlash) ? "" : "(?=\\/|$)";

		printf("END IS FALSE. ROUTE: %s\n", route.c_str());
	}

	std::string regex_string = "^" + route;

	printf("REGEX_STRING: %s\n", regex_string.c_str());

	// return std::regex{"^" + route, flags(options)};

	auto it = options.find("sensitive");

	if(it != options.end() and it->second) {
		printf("SENSITIVE FOUND AND IS TRUE\n");
		return std::regex{"^" + route};
	}

	printf("SENSITIVE NOT FOUND OR IS FALSE: CASE INSENSITIVE\n");

	return std::regex{"^" + route, std::regex_constants::icase};
	// case insensitive if sensitive is false
	// default is that sensitive is false

	// TODO: Check if std::regex_constants::icase works (case insensitive)
}

/* If we are to return re that is passed in, this method can be void (pass a reference so we can change it)
std::regex PathToRegexp::attachKeys(const std::regex& re, const std::vector<Token>& keys) {
	// js: re.keys = keys; return re;
}*/

/* Used by tokens_to_regexp-method
// Get the flags for a regex from the options
std::string PathToRegexp::flags(const std::map<std::string, bool>& options) {
	// sensitive: default false

	if(options.find("sensitive")->second)
		return "";

	return "i";
}*/

};	// < namespace route
