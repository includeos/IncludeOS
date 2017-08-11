#include <hw/cmos.hpp>
#include <statman>
#include "../platform/x86_pc/acpi.hpp"

using namespace hw;
uint8_t   CMOS::reg_b_value = 0;
uint32_t* CMOS::now_called  = nullptr;

void CMOS::init()
{
  now_called = &Statman::get().create(Stat::UINT32, "cmos.now").get_uint32();
  // Changing modes doesn't give expected results on e.g. GCE
  //set(r_status_b, b_24_hr_clock | b_binary_mode);
  reg_b_value = get(r_status_b);
  INFO("CMOS", "RTC is %i hour format, %s mode",
       mode_24_hour() ? 24 : 12, mode_binary() ? "binary" : "BCD");
}

CMOS::Time& CMOS::Time::hw_update() {
  // We're supposed to check this before every read
  while (update_in_progress());
  const reg_t r_cent = x86::ACPI::get().cmos_century();

  if (CMOS::mode_binary()) {
    f.second = get(r_sec);
    f.minute = get(r_min);
    f.hour = get(r_hrs);
    f.day_of_week = get(r_dow);
    f.day_of_month = get(r_day);
    f.month = get(r_month);
    f.year =  get(r_year);
    f.century = get(r_cent);
  } else {
    f.second = bcd_to_binary(get(r_sec));
    f.minute = bcd_to_binary(get(r_min));
    f.hour = bcd_to_binary(get(r_hrs));
    f.day_of_week = bcd_to_binary(get(r_dow));
    f.day_of_month = bcd_to_binary(get(r_day));
    f.month = bcd_to_binary(get(r_month));
    f.year =  bcd_to_binary(get(r_year));
    f.century = bcd_to_binary(get(r_cent));
  }

  // Convert to 24-hour clock if necessary
  if (not mode_24_hour() and f.hour & 0x80) {
    f.hour = ((f.hour & 0x7F) + 12) % 24;
  }

  return *this;
}


std::string CMOS::Time::to_string(){
  std::array<char,20> str;
  sprintf(str.data(), "%.2i-%.2i-%iT%.2i:%.2i:%.2iZ",
          f.century * 100 + f.year,
          f.month,
          f.day_of_month,
          f.hour, f.minute, f.second);
  return std::string(str.data(), str.size());
}


int CMOS::Time::day_of_year() {
  static std::array<uint8_t, 13> days_in_normal_month =
    {{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }};

  int res = f.day_of_month;
  auto month = f.month;
  while (month --> 0)
    res += days_in_normal_month[ month ];

  if (is_leap_year(year()) && f.month > 2)
    res += 1;

  return res;
}

CMOS::Time CMOS::now()
{
  CMOS::now_called++;
  return Time().hw_update();
}
