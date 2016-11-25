// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#ifndef DEMO_PAGE_HPP
#define DEMO_PAGE_HPP

#include <string>

std::string generate_demo_page(
	int64_t open_connections,
	int64_t page_requests,
	std::string client_agents
)
{
	return{
	"<!DOCTYPE html>"
	"<html>"
	"<head>"
	"<style>"
		".text_copy_a, .text_copy_b {"
			"fill: none;"
			"stroke: #EFEAC5;"
			"stroke-width: 0.3vw;"
		"}"
			".text_copy_a {"
			"stroke-dasharray: 12vw 24vw;"
			"animation: anim_a 4s ease-in-out 1s 1;"
		"}"
			".text_copy_b {"
			"stroke-dasharray: 10vw 10vw;"
			"animation: anim_b 14s ease-in-out -5s infinite;"
		"}"
			".copys_a {"
			"animation: anim_op_a 1s 1;"
		"}"
			".text_copy_a:nth-child(1) {"
			"stroke: #BF2D42;"
			"stroke-dashoffset: 12vw;"
		"}"
			".text_copy_a:nth-child(2) {"
			"stroke: #3DD4F2;"
			"stroke-dashoffset: 24vw;"
		"}"
			".text_copy_a:nth-child(3) {"
			"stroke: #EFEAC5;"
			"stroke-dashoffset: 36vw;"
		"}"
			".text_copy_b:nth-child(1) {"
			"stroke: #BF2D42;"
			"stroke-dashoffset: 0vw;"
		"}"
			".text_copy_b:nth-child(2) {"
			"stroke: #EFEAC5;"
			"stroke-dashoffset: 10vw;"
		"}"
			"@keyframes anim_op_a {"
			"from { opacity: 0; }"
			"to { opacity: 0; }"
		"}"
		"@keyframes anim_a {"
			"0% {"
			"stroke-dashoffset: 10vw;"
			"stroke-dasharray: 0vw 20vw;"
			"}"
		"}"
		"@keyframes anim_b {"
			"50% {"
			"stroke-dashoffset: 12vw;"
			"stroke-dasharray: 5vw 12vw;"
			"}"
		"}"
		"html, body {"
			"height:99%;"
		"}"
		"body {"
			"background: radial-gradient(#153646 30%, #082330);"
			"overflow-x: hidden;"
		"}"
		"footer {"
			"display : table-row;"
			"vertical-align : bottom;"
			"height : 1em;"
		"}"
		"hr{"
			"height: 1px;"
			"color: #EFEAC5;"
			"background-color: #EFEAC5;"
			"border: none;"
		"}"
		"#container {"
			"height:100%;"
			"width: 100%;"
			"border-collapse:collapse;"
			"display : table;"
		"}"
		".title {"
			"font: 16vw/1 Arial, Helvetica, sans-serif;"
		"}"
		".main {"
			"padding-top: 7vh;"
			"text-align: left;"
			"font: 1.4em Arial, Helvetica, sans-serif;"
			"color: #EFEAC5;"
		"}"
		".visitor_info {"
			"text-align: center;"
		"}"
		".info_text {"
			"font-size: 0.95em;"
			"color: #91E7F8;"
			"width: 90%;"
		"}"
		".server_info {"
			"width: 100%;"
			"font-size: 1.2em;"
			"padding-top: 2vh;"
			"text-align: center;"
		"}"
		"span {"
			"display: inline-block;"
		"}"
	"</style>"
	"</head>"

	"<body>"
	"<div id=\"container\">"
		"<div class=\"title\">"
			"<svg width=\"100\" height=\"20\""
				"style=\"width: 100%; height: 20%;\""
			">"
			"<symbol id=\"text_a\">"
			"<text x=\"8%\" y=\"90%\" class=\"text_line_a\">Include</text>"
			"</symbol>"
			"<symbol id=\"text_b\">"
			"<text x=\"65%\" y=\"90%\" class=\"text_line_b\">OS</text>"
			"</symbol>"

			"<g class=\"copys_a\">"
			"<use xlink:href=\"#text_a\" class=\"text_copy_a\"></use>"
			"<use xlink:href=\"#text_a\" class=\"text_copy_a\"></use>"
			"<use xlink:href=\"#text_a\" class=\"text_copy_a\"></use>"
			"</g>"
			"<g class=\"copys_b\">"
			"<use xlink:href=\"#text_b\" class=\"text_copy_b\"></use>"
			"<use xlink:href=\"#text_b\" class=\"text_copy_b\"></use>"
			"</g>"
			"</svg>"
		"</div>"
		"<div class=\"main\">"
			"<div class=\"visitor_info\">"
				"<span style=\"font-size: 1.6em\">"
				"--- Recent visitors ---"
				"</span><hr>"
				"<span class=\"info_text\">"
				+ std::move(client_agents) +
				"</span><hr>"
			"</div>"
		"</div>"
		"<footer class=\"main\">"
			"<span class=\"server_info\">"
			"Open connections: <span style=\"color: #BF2D42\">"
			+ std::to_string(open_connections) +
			"</span><br><hr>"
			"Page requests: <span style=\"color: #BF2D42\">"
			+ std::to_string(page_requests) +
			"</span>"
			"</span>"
			"&copy; 2016 Include<span style=\"color: #BF2D42\">OS</span>"
		"</footer>"
	"</div>"
	"</body>"
	"</html>"
	};
}

std::string generate_demo_header(size_t html_size, bool keep_open)
{
	return{
	"HTTP/1.1 200 OK\n"
	"Date: Mon, 01 Jan 1970 00:00:01 GMT\n"
	"Server: IncludeOS prototype 4.0\n"
	"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n"
	"Content-Type: text/html; charset=UTF-8\n"
	"Content-Length: " + std::to_string(html_size) + "\n"
	"Accept-Ranges: bytes\n"
	+ std::string{ keep_open
		? "Connection: Keep-Alive\n\n"
		: "Connection: close\n\n"
	}
	};
}
#endif // DEMO_PAGE_HPP