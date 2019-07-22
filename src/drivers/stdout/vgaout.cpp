
#include <vga>

// output to VGA as well as the default stdout
__attribute__((constructor))
static void add_vga_output()
{
  os::add_stdout(TextmodeVGA::get().get_print_handler());
}
