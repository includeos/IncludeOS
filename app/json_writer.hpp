#ifndef CEREAL_JSON_WRITER
#define CEREAL_JSON_WRITER

#define RAPIDJSON_HAS_STDSTRING 1

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

template<typename OutputStream = rapidjson::StringBuffer>
class JSON_Writer : rapidjson::Writer<OutputStream> {
public:
  using base = rapidjson::Writer<OutputStream>;
  JSON_Writer(OutputStream& os) : base(os) {

  }

  void add(std::string key, unsigned u) {
    base::Key(key.c_str(), (unsigned)key.size());
    base::Uint(u);
  }

  void add(std::string key, std::string str) {
    base::Key(key.c_str(), (unsigned)key.size());
    base::String(str.c_str(), (unsigned)str.size());
  }

  inline void start_object() {
    base::StartObject();
  }

  inline void end_object() {
    base::EndObject();
  }

  inline void start_array() {
    base::StartArray();
  }

  inline void end_array() {
    base::EndArray();
  }



private:

};

#endif
