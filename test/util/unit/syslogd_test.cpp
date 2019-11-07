
#include <common.cxx>
#include <syslog.h>
#include <util/syslogd.hpp>

CASE("valid_priority() returns whether supplied priority is valid")
{
  EXPECT(Syslog::valid_priority(LOG_EMERG) == true);
  EXPECT(Syslog::valid_priority(LOG_DEBUG) == true);
  EXPECT_NOT(Syslog::valid_priority(8192007) == true);
}

CASE("valid_logopt() returns whether supplied logopt is valid")
{
  EXPECT(Syslog::valid_logopt(LOG_PID || LOG_NOWAIT) == true);
}

CASE("valid_facility() returns whether supplied facility is valid")
{
  EXPECT(Syslog::valid_facility(LOG_USER) == true);
}

