
#include <unordered_map>

#include <net/http/mime_types.hpp>

#include "../../../api/util/detail/string_view"

namespace http {

const std::unordered_map<util::sview, util::csview> mime_type_table {
  //< Text mimes
  {"html", "text/html"},
  {"htm" , "text/html"},
  {"js"  , "text/javascript"},
  {"txt" , "text/plain"},
  {"css" , "text/css"},
  {"xml" , "text/xml"},

  //< Image mimes
  {"bmp" , "image/bmp"},
  {"gif" , "image/gif"},
  {"png" , "image/png"},
  {"jpg" , "image/jpeg"},
  {"jpeg", "image/jpeg"},
  {"ico" , "image/x-icon"},
  {"svg" , "image/svg+xml"},

  //< Audio mimes
  {"mid" , "audio/midi"},
  {"midi", "audio/midi"},
  {"kar" , "audio/midi"},
  {"mp3" , "audio/mpeg"},
  {"ogg" , "audio/ogg"},
  {"m4a" , "audio/x-m4a"},
  {"ra"  , "audio/x-realaudio"},

  //< Video mimes
  {"3gp" , "video/3gpp"},
  {"3gpp", "video/3gpp"},
  {"ts"  , "video/mp2t"},
  {"mp4" , "video/mp4"},
  {"mpg" , "video/mpeg"},
  {"mpeg", "video/mpeg"},
  {"mov" , "video/quicktime"},
  {"webm", "video/webm"},
  {"flv" , "video/x-flv"},
  {"m4v" , "video/x-m4v"},
  {"mng" , "video/x-mng"},
  {"asf" , "video/x-ms-asf"},
  {"asx" , "video/x-ms-asf"},
  {"wmv" , "video/x-ms-wmv"},
  {"avi" , "video/x-msvideo"},

  //< Application mimes
  {"zip"  , "application/zip"},
  {"7z"   , "application/x-7z-compressed"},
  {"jar"  , "application/java-archive"},
  {"war"  , "application/java-archive"},
  {"ear"  , "application/java-archive"},
  {"json" , "application/json"},
  {"pdf"  , "application/pdf"},
  {"xhtml", "application/xhtml+xml"},
  {"xspf" , "application/xspf+xml"},
  {"der" , "application/x-x509-ca-cert"},
  {"pem" , "application/x-x509-ca-cert"},
  {"crt" , "application/x-x509-ca-cert"},
  {"bin" , "application/octet-stream"},
  {"exe" , "application/octet-stream"},
  {"dll" , "application/octet-stream"},
  {"deb" , "application/octet-stream"},
  {"dmg" , "application/octet-stream"},
  {"iso" , "application/octet-stream"},
  {"img" , "application/octet-stream"},
  {"msi" , "application/octet-stream"},
  {"msp" , "application/octet-stream"},
  {"msm" , "application/octet-stream"}
}; //< mime_type_table

util::sview ext_to_mime_type(util::csview extension) noexcept {
  const auto it = mime_type_table.find(extension);
  //------------------------------------------------
  return (it not_eq mime_type_table.cend())
          ? it->second
          : "application/octet-stream";
}

} //< namespace http
