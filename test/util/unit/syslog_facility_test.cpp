
#include <common.cxx>
#include <util/syslog_facility.hpp>

CASE("facility-related functions get/set facility, facility_name() returns facility name as string")
{
  Syslog_print sf;
  // default created with LOG_USER
  EXPECT(sf.facility_name() == "USER");
  sf.set_facility(LOG_MAIL);
  EXPECT(sf.facility_name() == "MAIL");
  EXPECT(sf.facility() == LOG_MAIL);
}

CASE("ident-related functions get/set ident, returns whether ident is set")
{
  Syslog_print sf;
  EXPECT(sf.ident_is_set() == false);
  sf.set_ident("foo");
  EXPECT(sf.ident_is_set() == true);
  EXPECT(sf.ident() == "foo");
}

CASE("priority-related functions get/set priority")
{
  Syslog_print sf;
  sf.set_priority(LOG_CRIT);
  EXPECT_NOT(sf.priority() == LOG_INFO);
  EXPECT(sf.priority() == LOG_CRIT);
  EXPECT(sf.priority_name() == "CRIT");
}

CASE("logopt-related functions get/set logopt")
{
  Syslog_print sf("bar", LOG_INTERNAL);
  sf.set_logopt(LOG_NOWAIT);
  EXPECT_NOT(sf.logopt() == LOG_USER);
  EXPECT(sf.logopt() == (LOG_NOWAIT));
}
