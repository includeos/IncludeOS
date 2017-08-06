#include <syslog.h>

int main()
{
  /* ------------------------- POSIX syslog on lubuntu ------------------------- */

  int invalid_priority = -1;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 10;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  invalid_priority = 55;
  syslog(invalid_priority, "Invalid %d", invalid_priority);

  syslog(LOG_INFO, "(Info) No open has been called prior to this");
  syslog(LOG_NOTICE, "(Notice) Program created with two arguments: %s and %s", "one", "two");

  openlog("Prepended message", LOG_CONS | LOG_NDELAY, LOG_MAIL);

  syslog(LOG_ERR, "(Err) Log after prepended message with one argument: %d", 44);
  syslog(LOG_WARNING, "(Warning) Log number two after openlog set prepended message");

  closelog();

  syslog(LOG_WARNING, "(Warning) Log after closelog with three arguments. One is %u, another is %s, a third is %d", 33, "this", 4011);

  openlog("Second prepended message", LOG_PID | LOG_CONS, LOG_USER);

  syslog(LOG_EMERG, "This is a test of an emergency log. You might need to stop the program manually.");
  syslog(LOG_ALERT, "Alert log with the m argument: %m");

  closelog();

  syslog(LOG_CRIT, "Critical after cleared prepended message (closelog has been called)");

  closelog();

  openlog("Open after close prepended message", LOG_CONS, LOG_MAIL);

  syslog(LOG_INFO, "Info after openlog with both m: %m and two hex arguments: 0x%x and 0x%x", 100, 50);

  closelog();

  return 0;
}
