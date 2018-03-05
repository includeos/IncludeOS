// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <mender/state.hpp>
#include <mender/client.hpp>

#include <timers>

using namespace mender;
using namespace mender::state;

void State::set_state(Client& cli, State& s)
{
  cli.set_state(s);
  MENDER_INFO("State", "New state: %s", s.to_string().c_str());
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
    const auto body{std::string(res.body())};
    switch(res.status_code())
    {
      MENDER_INFO("Auth_wait", "%s", body.c_str());
      case 200:
        MENDER_INFO2("Successfully authorized!");
        // set token
        cli.set_auth_token({body.begin(), body.end()});
        // remove delay
        ctx.delay = 0;
        set_state<Authorized>(cli);
        return GO_NEXT;

      case 400:
      case 401: // Unauthorized
        MENDER_INFO2("Auth failed:\n%s", body.c_str());

        // increase up to 60 seconds
        if(ctx.delay < 60)
          ctx.delay += 5;

        MENDER_INFO2("Delay increased: %d", ctx.delay);
        set_state<Init>(cli);
        return DELAYED_NEXT;

      default:
        MENDER_INFO2("Not handeled (%u)", ctx.response->status_code());
    }
  }
  return AWAIT_EVENT;
}

State::Result Authorized::handle(Client& cli, Context& ctx)
{
  if(ctx.last_inventory_update == 0) // todo: come up with some kind of limit
  {
    MENDER_INFO("Authorized", "Inventory not updated");
    cli.update_inventory_attributes();
  }
  else
  {
    MENDER_INFO("Authorized", "Inventory already updated");
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
    MENDER_INFO("Update_check", "Received response");
    switch(ctx.response->status_code())
    {
      case 200: // there is an update! "verify"
        cli.fetch_update(std::move(ctx.response));
        set_state<Update_fetch>(cli);
        return AWAIT_EVENT;

      case 204: // no update found
        ctx.delay = cli.update_poll_interval.count(); // Ask again every n second
        MENDER_INFO2("No update, delay asking again.");
        set_state<Authorized>(cli);
        return DELAYED_NEXT;

      case 401: // Unauthorized, go directly to Init
        MENDER_INFO2("Unauthorized, try authorize");
        set_state<Init>(cli);
        return GO_NEXT;

      default:
        MENDER_INFO2("Not handeled (%u)", ctx.response->status_code());
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
        MENDER_INFO("Update_fetch", "Update downloaded (%u bytes)", (uint32_t)ctx.response->body().size());
        cli.install_update(std::move(ctx.response));
        return AWAIT_EVENT;

      default:
        MENDER_INFO("Update_fetch", "Not handeled (%u)", ctx.response->status_code());
    }
  }
  return AWAIT_EVENT;
}

State::Result Error_state::handle(Client&, Context&)
{
  MENDER_INFO("Error_state", "Previous state %s resulted in error.", prev_->to_string().c_str());
  return AWAIT_EVENT;
}
