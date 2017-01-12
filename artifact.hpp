
#ifndef MENDER_ARTIFACT_HPP
#define MENDER_ARTIFACT_HPP

#include "common.hpp"
#include <tar>

namespace mender {

  class Artifact {
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
      printf("<Artifact> Parsing data as mender arifact (%u bytes)\n", data_.size());
      artifact_ = reader_.read_uncompressed((const char*)data_.data(), data_.size());

      for (auto& element : artifact_.elements()) {
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
    }

  private:
    byte_seq data_;
    tar::Reader reader_;
    tar::Tar artifact_;

  }; // < class Artifact

} // < namespace mender

#endif
