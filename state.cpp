

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
  printf("<Init> Made auth request => Auth_wait\n");
  return AWAIT_EVENT;
}

State::Result Auth_wait::handle(Client& cli, Context& ctx)
{
  if(cli.is_authed()) {
    printf("<Auth_wait> Auth success => Authorized\n");
    set_state<Authorized>(cli);
    ctx.delay = 0;
    ctx.response = nullptr;
    return GO_NEXT;
  }

  // increase up to 60 seconds
  if(ctx.delay < 60)
    ctx.delay++;

  printf("<Auth_wait> Auth failed, delay increased: %d => Init\n", ctx.delay);
  set_state(cli, Init::instance());

  return DELAYED_NEXT;
}

State::Result Authorized::handle(Client& cli, Context& ctx)
{
  if(ctx.last_inventory_update == 0)
  {
    printf("<Authorized> Inventory not updated => Authorized\n");
    cli.update_inventory_attributes();
  }
  else
  {
    printf("<Authorized> Checking for update => Update_check\n");
    cli.check_for_update();
    set_state<Update_check>(cli);
  }
  return AWAIT_EVENT;
}

State::Result Update_check::handle(Client& cli, Context& ctx)
{
  if(ctx.response == nullptr) {

  }
  else
  {
    printf("<Update_check> Received response\n");
    switch(ctx.response->status_code())
    {
      case 200: // there is an update! verify
        cli.fetch_update(std::move(ctx.response));
        set_state<Update_fetch>(cli);
        printf("<Update_check> Fetch update => Update_fetch\n");
        return AWAIT_EVENT;

      case 204: // no update found
        set_state<Authorized>(cli);
        ctx.delay = 10; // Ask again every 10th second
        printf("<Update_check> No update, delay asking again => Authorized\n");
        return DELAYED_NEXT;

      case 401: // Unauthorized, go directly to Init
        set_state<Init>(cli);
        printf("<Update_check> Unauthorized, try authorize => Init\n");
        return GO_NEXT;
    }
  }
  return AWAIT_EVENT;
}

State::Result Update_fetch::handle(Client& cli, Context& ctx)
{

  if(ctx.response == nullptr) {

  }
  else
  {
    switch(ctx.response->status_code())
    {
      case 200: // there is an update! verify
        printf("<Update_fetch> Update downloaded (%u bytes)\n", ctx.response->body().size());
        return AWAIT_EVENT;

      default:
        printf("<Update_fetch> Not handeled (%u)\n", ctx.response->status_code());
    }
  }
  return AWAIT_EVENT;
}

