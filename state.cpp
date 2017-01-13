

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

State::Result Init::handle(Client& cli, Context&)
{
  cli.make_auth_request();
  set_state<Auth_wait>(cli);
  return AWAIT_EVENT;
}

State::Result Auth_wait::handle(Client& cli, Context& ctx)
{
  if(ctx.response == nullptr) {
    set_state(cli, Error_state::instance(*this));
  }
  else
  {
    auto& res = *ctx.response;
    const auto body{res.body().to_string()};
    switch(res.status_code())
    {
      case 200:
        printf("<Auth_wait> %s\n", body.c_str());
        printf("<Auth_wait> Successfully authorized!\n");
        // set token
        cli.set_auth_token({body.begin(), body.end()});
        // remove delay
        ctx.delay = 0;
        set_state<Authorized>(cli);
        return GO_NEXT;

      case 400:
      case 401: // Unauthorized
        printf("<Auth_wait> Auth failed:\n%s\n", body.c_str());

        // increase up to 60 seconds
        if(ctx.delay < 60)
          ctx.delay++;

        printf("<Auth_wait> Delay increased: %d\n", ctx.delay);
        set_state<Init>(cli);
        return DELAYED_NEXT;

      default:
        printf("<Auth_wait> Not handeled (%u)\n", ctx.response->status_code());
    }
  }
  return AWAIT_EVENT;
}

State::Result Authorized::handle(Client& cli, Context& ctx)
{
  if(ctx.last_inventory_update == 0) // todo: come up with some kind of limit
  {
    printf("<Authorized> Inventory not updated\n");
    cli.update_inventory_attributes();
  }
  else
  {
    printf("<Authorized> Inventory already updated\n");
    cli.check_for_update();
    set_state<Update_check>(cli);
  }
  return AWAIT_EVENT;
}

State::Result Update_check::handle(Client& cli, Context& ctx)
{
  if(ctx.response == nullptr) {
    set_state(cli, Error_state::instance(*this));
  }
  else
  {
    printf("<Update_check> Received response\n");
    switch(ctx.response->status_code())
    {
      case 200: // there is an update! "verify"
        cli.fetch_update(std::move(ctx.response));
        set_state<Update_fetch>(cli);
        return AWAIT_EVENT;

      case 204: // no update found
        ctx.delay = 10; // Ask again every 10th second
        printf("<Update_check> No update, delay asking again.\n");
        set_state<Authorized>(cli);
        return DELAYED_NEXT;

      case 401: // Unauthorized, go directly to Init
        printf("<Update_check> Unauthorized, try authorize\n");
        set_state<Init>(cli);
        return GO_NEXT;

      default:
        printf("<Update_check> Not handeled (%u)\n", ctx.response->status_code());
    }
  }
  return AWAIT_EVENT;
}

State::Result Update_fetch::handle(Client& cli, Context& ctx)
{

  if(ctx.response == nullptr) {
    set_state(cli, Error_state::instance(*this));
  }
  else
  {
    switch(ctx.response->status_code())
    {
      case 200: // Update fetched! prepare for install
        printf("<Update_fetch> Update downloaded (%u bytes)\n", ctx.response->body().size());
        cli.install_update(std::move(ctx.response));
        return AWAIT_EVENT;

      default:
        printf("<Update_fetch> Not handeled (%u)\n", ctx.response->status_code());
    }
  }
  return AWAIT_EVENT;
}

State::Result Error_state::handle(Client&, Context&)
{
  printf("<Error_state> Previous state %s resulted in error.\n", prev_->to_string().c_str());
  return AWAIT_EVENT;
}

