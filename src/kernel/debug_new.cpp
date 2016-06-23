#include <debug_new>

std::unordered_map<uintptr_t, debug_new_entry> __dnew_entries;

void debug_new_print_entries()
{
  printf("- listing entries:\n");
  for (auto& ent : __dnew_entries) {
    printf("entry %#x:  size %u  %s:%u  func %s +%#x\n",
      ent.first, ent.second.size, ent.second.file.c_str(), ent.second.line, 
      ent.second.func.name.c_str(), ent.second.func.offset);
  }
  printf("- total: %u\n", __dnew_entries.size());
}
