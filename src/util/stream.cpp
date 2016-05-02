#include <stream>

#include <memory>

void Stream::upload(
    Connection    conn, 
    Disk          disk, 
    const Dirent& ent, 
    on_after_func callback,
    const size_t  CHUNK_SIZE)
{
  typedef std::function<void(size_t)> next_func_t;
  auto next = std::make_shared<next_func_t> ();
  
  *next =
  [next, conn, disk, ent, callback, CHUNK_SIZE] (size_t pos) {
    
    // number of write calls necessary
    const size_t writes = ent.size / CHUNK_SIZE;
    
    // done condition
    if (pos >= writes) {
      callback(fs::no_error, true);
      return;
    }
    
    // read chunk from file
    disk->fs().read(ent, pos * CHUNK_SIZE, CHUNK_SIZE,
    [next, pos, conn, ent, callback] (
        fs::error_t  err, 
        fs::buffer_t buffer, 
        uint64_t     length)
    {
      if (err) {
        printf("BAD\n");
        callback(err, false);
        return;
      }
      
      // write chunk to TCP connection
      conn->write(buffer.get(), length, 
      [next, pos, ent, callback] (size_t n) {
        
        // if all data written, go to next chunk
        if (n == ent.size)
          (*next)(pos+1);
        else
          // otherwise, fail
          callback(fs::no_error, false);
        
      }, true);
      
    });
    
  };
  
  (*next)(0);
}
