
#include "client.hpp"
#include "state.hpp"
#include <botan/base64.h>

namespace mender {

  Client::Client(Auth_manager&& man, net::TCP& tcp, const std::string& server, const uint16_t port)
    : am_{std::forward<Auth_manager>(man)},
      server_(server),
      cached_{0, port},
      httpclient_{std::make_unique<http::Client>(tcp)},
      state_(&state::Init::instance())
  {
    printf("<Client> Client created\n");
    run_state();
  }

  Client::Client(Auth_manager&& man, net::TCP& tcp, net::tcp::Socket socket)
    : am_{std::forward<Auth_manager>(man)},
      server_{socket.address().to_string()},
      cached_(std::move(socket)),
      httpclient_{std::make_unique<http::Client>(tcp)},
      state_(&state::Init::instance())
  {
    printf("<Client> Client created\n");
    run_state();
  }

  void Client::make_auth_request()
  {
    auto auth = am_.make_auth_request();

    using namespace std::string_literals;
    using namespace http;

    std::string data{auth.data.begin(), auth.data.end()};

    //printf("Signature:\n%s\n", Botan::base64_encode(auth.signature).c_str());

    // Setup headers
    const Header_set headers{
      { header::Content_Type, "application/json" },
      { header::Accept, "application/json" },
      { "X-MEN-Signature", Botan::base64_encode(auth.signature) }
    };

    printf("<Client> Making Auth request\n");
    // Make post
    httpclient_->post(cached_,
      "/api/devices/0.1/authentication/auth_requests",
      create_headers(auth.signature),
      {data.begin(), data.end()},
      {this, &Client::auth_handler});
  }

  http::Header_set Client::create_headers(const byte_seq& signature) const
  {
    return {
      { http::header::Content_Type, "application/json" },
      { http::header::Accept, "application/json" },
      { "X-MEN-Signature", Botan::base64_encode(signature) }
    };
  }

  void Client::response_handler(http::Error err, http::Response_ptr res)
  {
    context_.response = std::move(res);
    run_state();
  }

  void Client::auth_handler(http::Error err, http::Response_ptr res)
  {
    if(!res)
    {
      printf("No reply.\n");
    }
    else
    {
      if(is_valid_response(*res))
      {
        switch(res->status_code())
        {
          case 200:
            printf("<Client> 200 OK:\n");
            auth_success(*res);
            break;

          case 401:
            printf("<Client> 401 Unauthorized:\n%s\n", res->body().to_string().c_str());
            break;

          default:
            printf("<Client> Failed with error code: %u\n%s\n",
              res->status_code(), res->body().to_string().c_str());
        }
      }
      else
        printf("<Client> Invalid response:\n%s\n", res->to_string().c_str());
    }
    run_state();
  }

  bool Client::is_valid_response(const http::Response& res) const
  {
    const bool is_json = res.header().value(http::header::Content_Type).find("application/json") != std::string::npos;
    return is_json;
  }

  void Client::auth_success(const http::Response& res)
  {
    printf("%s\n", res.to_string().c_str());
    const auto body{res.body().to_string()};
    am_.set_auth_token({body.begin(), body.end()});

    if(on_auth_)
      on_auth_(is_authed());
  }

  void Client::authenticate(Timers::duration_t interval, AuthCallback cb)
  {
    on_auth(std::move(cb));

    using namespace std::chrono;
  }

  void Client::check_for_update()
  {
    printf("<Client> Checking for update\n");

    using namespace http;

    const auto& token = am_.auth_token();
    // Setup headers
    const Header_set headers{
      { header::Content_Type, "application/json" },
      { header::Accept, "application/json" },
      { header::Authorization, "Bearer " + std::string{token.begin(), token.end()}}
    };

    httpclient_->get(cached_,
      "/api/devices/0.1/deployments/device/deployments/next",
      headers, {this, &Client::update_handler});
  }

  void Client::update_handler(http::Error err, http::Response_ptr res)
  {
    if(!res)
    {
      printf("No reply.\n");
    }
    else
    {
      if(is_valid_response(*res))
      {
        switch(res->status_code())
        {
          case 200:
            printf("<Client> 200 OK:\n");
            printf("%s\n", res->body().to_string().c_str());
            //auth_success(*res);
            break;

          case 401:
            printf("<Client> 401 Unauthorized:\n%s\n", res->body().to_string().c_str());
            break;

          default:
            printf("<Client> Failed with error code: %u\n%s\n",
              res->status_code(), res->body().to_string().c_str());
        }
      }
      else
        printf("<Client> Invalid response:\n%s\n", res->to_string().c_str());
    }
    //run_state();
  }





};
