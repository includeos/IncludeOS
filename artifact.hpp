
#ifndef MENDER_ARTIFACT_HPP
#define MENDER_ARTIFACT_HPP

#include "common.hpp"
#include "json.hpp"
#include <tar>
#include <cassert>

namespace mender {

  class Artifact {

    enum Index {
      VERSION = 0,
      HEADER = 1,
      DATA_START = 2
    };

  public:
    Artifact(byte_seq data)
      : data_(std::move(data)),
        reader_{},
        artifact_{}
    {
      parse();
    }

    /**
     * @brief      Parse according to the mender file structure
     */
    void parse()
    {
      using namespace tar;

      // Print and add

      printf("<Artifact> Parsing data as mender arifact (%u bytes)\n", data_.size());
      artifact_ = reader_.read_uncompressed(data_.data(), data_.size());

      auto& elements = artifact_.elements();
      printf("<Artifact> Printing content\n");

      // Version
      auto& version_element = elements.at(VERSION);
      parse_version(version_element.content());
      printf("<Artifact> %s\n", version_element.name().c_str());

      // Header
      auto& header_element = elements.at(HEADER);
      printf("<Artifact> %s\n", header_element.name().c_str());
      header_ = reader_.decompress(header_element);

      auto& header_elements = header_.elements();

      for (size_t i = 0; i < header_.elements().size(); i++) {
        const std::string header_element_name = header_.elements().at(i).name();
        printf("<Artifact>\t%s\n", header_element_name.c_str());
      }

      // header_info
      auto& header_info = header_elements.at(0);
      parse_header_info(header_info.content());

      // Data
      for (size_t i = DATA_START; i < elements.size(); i++) {
        auto& data_tar_gz = elements.at(i);
        const std::string data_name = data_tar_gz.name();

        assert(data_name.find("data/") != std::string::npos and data_tar_gz.is_tar_gz());

        printf("<Artifact> %s\n", data_tar_gz.name().c_str());

        auto tar = reader_.decompress(data_tar_gz);

        for (auto& data_element : tar.elements())
          printf("<Artifact>\t%s\n", data_element.name().c_str());

        updates_.push_back(tar);
      }
    }

    int version() const { return version_; }
    const std::string& format() const { return format_; }
    const std::string& name() const { return name_; }
    const tar::Element& get_update(int index = 0) const { return updates_.at(index).elements().at(0); }

    void parse_version(const uint8_t* version) {
      using namespace nlohmann;
      auto ver = json::parse((char*)version);
      format_ = ver["format"];
      version_ = ver["version"];
    }

    void parse_header_info(const uint8_t* header_info) {
      using namespace nlohmann;
      auto info = json::parse((char*)header_info);
      name_ = info["artifact_name"];
    }

  private:
    byte_seq data_;
    tar::Reader reader_;
    tar::Tar artifact_;               // version, header.tar.gz, data/0000.tar.gz, data/0001.tar.gz, etc.
    tar::Tar header_;                 // unpacked header.tar.gz
    std::vector<tar::Tar> updates_;   // unpacked data/0000.tar.gz, data/0001.tar.gz, etc.

    int version_;                     // version specified in version file
    std::string format_;              // format specified in version file
    std::string name_;                // name of artifact as specified in header_info (first element inside header.tar.gz)

  }; // < class Artifact

} // < namespace mender

#endif
