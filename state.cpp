

#include "state.hpp"
#include "client.hpp"
#include <timers>

using namespace mender;
using namespace mender::state;

void State::set_state(Client& cli, State& s)
{
  cli.set_state(s);
  printf("New state: %s\n", s.to_string().c_str());
}

State::Result Init::handle(Client& cli, Context& ctx)
{
  cli.make_auth_request();
  set_state<Auth_wait>(cli);
  printf("<Init::handle> Made auth request => Auth_wait\n");
  return AWAIT_EVENT;
}

State::Result Auth_wait::handle(Client& cli, Context& ctx)
{
  if(cli.is_authed()) {
    printf("<Auth_wait::handle> Auth success => Authorized\n");
    set_state<Authorized>(cli);
    ctx.delay = 0;
    ctx.response = nullptr;
    return GO_NEXT;
  }

  // increase up to 60 seconds
  if(ctx.delay < 60)
    ctx.delay++;

  printf("<Auth_wait::handle> Auth failed, delay increased: %d => Init\n", ctx.delay);
  set_state(cli, Init::instance());

  return DELAYED_NEXT;
}

State::Result Authorized::handle(Client& cli, Context& ctx)
{
  printf("<Authorized::handle> Checking for update => TBD\n");
  cli.check_for_update();
  return AWAIT_EVENT;
}
