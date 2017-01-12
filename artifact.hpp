
#ifndef MENDER_ARTIFACT_HPP
#define MENDER_ARTIFACT_HPP

#include "common.hpp"
#include <tar>

namespace mender {

  class Artifact {

    enum Index {
      VERSION = 0,
      HEADER_TAR = 1,
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

      // Version
      // get version
      elements.at(VERSION);

      // Header
      // parse headers
      elements.at(HEADER_TAR);

      // Data
      // parse all data

      printf("Parsing DATA\n");

      for (int i = DATA_START; i < elements.size(); i++) {

        auto& element = elements.at(i);
        printf("Decompressing %s\n", element.name().c_str());

        tar::Tar tar = reader_.decompress(element);

        printf("Decompressed tar element %s\n", tar.elements().at(0).name().c_str());

        updates_.push_back(tar);

        // assert data/ .... og .tar.gz



      }



        // If this element/file is a .tar.gz file: decompress it and store content in a new Tar object

        /*if (element.name().size() > 7 and element.name().substr(element.name().size() - 7) == ".tar.gz") {
          tar::Tar& read_compressed = tgzr.decompress(element);

          // Loop through the elements of the tar.gz file and find the .img file and pass on
          for (auto e : read_compressed.elements()) {
            if (e.name().size() > 4 and e.name().substr(e.name().size() - 4) == ".img") {
              printf("<Client> Found img file\n");

            }
          }
        }*/

    }

    const tar::Element& get_update(int index = 0) const {
      return updates_.at(index).elements().at(0);
    }

  private:
    byte_seq data_;
    tar::Reader reader_;
    tar::Tar artifact_;     // version, header.tar.gz, data/0000.tar.gz


    tar::Tar header_;       // unpacked header.tar.gz
    std::vector<tar::Tar> updates_;    // unpacked data/0000.tar.gz

  }; // < class Artifact

} // < namespace mender

#endif
