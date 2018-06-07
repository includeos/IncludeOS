#include <profile>

void StackSampler::begin() {}
uint64_t StackSampler::samples_total() noexcept {
  return 1;
}
uint64_t StackSampler::samples_asleep() noexcept {
  return 0;
}
void StackSampler::print(int) {}
void StackSampler::set_mode(mode_t) {}

std::string HeapDiag::to_string()
{
  return "";
}
