//   __  __                     ___                            _  _
//  |  \/  |  __ _     __      | __|  _  _      ___     ___   | || |
//  | |\/| | / _` |   / _|     | _|  | +| |    |_ /    |_ /    \_, |
//  |_|__|_| \__,_|   \__|_   _|_|_   \_,_|   _/__|   _/__|   _|__/
//  _|"""""|_|"""""|_|"""""|_| """ |_|"""""|_|"""""|_|"""""|_| """"|
//  "`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'
#pragma once
#include "fuzzy_helpers.hpp"
#include "fuzzy_stack.hpp"
#include "fuzzy_packet.hpp"
//#include "fuzzy_stream.hpp"
extern void fuzzy_http(const uint8_t*, size_t);
extern void fuzzy_websocket(const uint8_t*, size_t);
