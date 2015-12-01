// Part of the IncludeOS unikernel - www.includeos.org
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

#include <os>
#include <string>
#include <iostream>

#include <memstream>
#include <jansson/jansson.h>

void Service::start()
{
  std::string text = R"(
[
    {
      "title": "this string is from array 1"
    },
    {
      "title": "this string is from array 2"
    }
]  
  )";
  
  std::cout << "*** parsing JSON string:" << text << std::endl;
  
  json_error_t error;
  json_t* root = 
    json_loads(text.c_str(), 0, &error);
  
  if (!root)
  {
    printf("json error: line %d: %s\n", error.line, error.text);
    return;
  }
  std::cout << "*** iterating JSON array (size=" << json_array_size(root) << ")" << std::endl;  
  
  for(size_t i = 0; i < json_array_size(root); i++)
  {
    json_t* node = json_array_get(root, i);
    // all array members should be "complex" objects
    if (!json_is_object(node))
    {
      std::cout << "index " << i + 1 << " is not an object" << std::endl;  
      json_decref(root);
      break;
    }
    
    json_t* title = json_object_get(node, "title");
    // all title member fields should be strings
    if (!json_is_string(title))
    {
      std::cout << "index " << i + 1 << " did not have string: title" << std::endl;  
      json_decref(root);
      break;
    }
    
    // read title text
    const char* title_text =
        json_string_value(title);
    
    std::cout << title_text << std::endl;
  }
  
  std::cout << "*** done" << std::endl;
}
