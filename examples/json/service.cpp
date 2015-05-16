#include <os>
#include <string>
#include <iostream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace rapidjson;

void Service::start()
{
  std::string text = R"(
    { "stars": 11,
      "title": "this string is ..."
    }
  )";
  
  std::cout << "*** parsing JSON string:" << text << std::endl;
  
  Document d;
  d.Parse(text.c_str());
  
  // 2. Modify it by DOM.
  Value& s = d["stars"];
  s.SetInt(s.GetInt() + 89);
  
  // 3. Stringify the DOM
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  
  // "stars":100
  std::cout << buffer.GetString() << std::endl;
  // "this string is ..."
  std::cout << "String: " << d["title"].GetString() << std::endl;
  
  std::cout << "*** done" << std::endl;
}
