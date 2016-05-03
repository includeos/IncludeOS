#include <async>

#include <memory>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

void Async::upload_file(
    Disk          disk, 
    const Dirent& ent, 
    Connection    conn, 
    on_after_func callback,
    const size_t  CHUNK_SIZE)
{
  disk_transfer(disk, ent,
  [conn, callback, CHUNK_SIZE] (fs::buffer_t buffer, 
                                size_t       length, 
                                next_func    next)
  {
    // write chunk to TCP connection
    conn->write(buffer.get(), length, 
    [callback, CHUNK_SIZE, next] (size_t n) {
      
      // if all data written, go to next chunk
      next(n == CHUNK_SIZE);
      
    }, true);
    
  }, callback, CHUNK_SIZE);
}

void Async::disk_transfer(
    Disk          disk, 
    const Dirent& ent, 
    on_write_func write_func, 
    on_after_func callback,
    const size_t  CHUNK_SIZE)
{
  typedef std::function<void(size_t)> next_func_t;
  auto next = std::make_shared<next_func_t> ();
  
  *next =
  [next, disk, ent, write_func, callback, CHUNK_SIZE] (size_t pos) {
    
    // number of write calls necessary
    const size_t writes = ent.size() / CHUNK_SIZE;
    
    // done condition
    if (pos >= writes) {
      callback(fs::no_error, true);
      return;
    }
    
    // read chunk from file
    disk->fs().read(ent, pos * CHUNK_SIZE, CHUNK_SIZE,
    [next, pos, write_func, callback, CHUNK_SIZE] (
        fs::error_t  err, 
        fs::buffer_t buffer, 
        uint64_t     length)
    {
      if (err) {
        printf("%s\n", err.to_string().c_str());
        callback(err, false);
        return;
      }
      
      // call write callback with data
      write_func(buffer, length,
      [next, pos, callback] (bool good) {
        // if the write succeeded, call next
        if (likely(good))
          (*next)(pos+1);
        else
          // otherwise, fail
          callback(fs::no_error, false);
      });
    });
    
  };
  // start async loop
  (*next)(0);
}
