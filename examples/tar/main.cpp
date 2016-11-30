#include "../../api/util/tar_reader.hpp"

int main() {
  Tar_reader tr;
  //tr.read("artifact.mender"); // check in read if is a mender-file (ends with .mender) - has no typeflag value
                                // or have a read_mender-method
  //tr.read("tar_ex_2.tar");
  //tr.read("tar_virtio_ex.tar");

  //tr.read("readme_service_ex.tar"); // ok
  tr.read("multiple_folders_no_virtio.tar");
}
