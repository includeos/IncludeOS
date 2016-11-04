

void kprintf(const char* format, ...) {
  int bufsize = strlen(format) * 2;
  char buf[bufsize];
  va_list aptr;
  va_start(aptr, format);
  vsnprintf(buf, bufsize, format, aptr);
  hw::Serial::print1(buf);
}
