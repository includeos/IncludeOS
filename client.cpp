
#include "client.hpp"
#include "state.hpp"
#include <botan/base64.h>
#include <rtc> // RTC::now()
#include <tar>

namespace mender {

  Client::Client(Auth_manager&& man, net::TCP& tcp, const std::string& server, const uint16_t port)
    : am_{std::forward<Auth_manager>(man)},
      device_{},
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
      device_{},
      server_{socket.address().to_string()},
      cached_(std::move(socket)),
      httpclient_{std::make_unique<http::Client>(tcp)},
      state_(&state::Init::instance())
  {
    printf("<Client> Client created\n");
    run_state();
  }

  void Client::run_state()
  {
    switch(state_->handle(*this, context_))
    {
      using namespace state;
      case State::Result::GO_NEXT:
        run_state();
        return;

      case State::Result::DELAYED_NEXT:
        run_state_delayed();
        return;

      case State::Result::AWAIT_EVENT:
        // todo: setup timeout
        return;
    }
  }

  void Client::make_auth_request()
  {
    auto auth = am_.make_auth_request();

    using namespace std::string_literals;
    using namespace http;

    std::string data{auth.data.begin(), auth.data.end()};

    //printf("Signature:\n%s\n", Botan::base64_encode(auth.signature).c_str());

    printf("<Client> Making Auth request\n");
    // Make post
    httpclient_->post(cached_,
      API_PREFIX + "/authentication/auth_requests",
      create_auth_headers(auth.signature),
      {data.begin(), data.end()},
      {this, &Client::response_handler});
  }

  http::Header_set Client::create_auth_headers(const byte_seq& signature) const
  {
    return {
      { http::header::Content_Type, "application/json" },
      { http::header::Accept, "application/json" },
      { "X-MEN-Signature", Botan::base64_encode(signature) }
    };
  }

  void Client::response_handler(http::Error err, http::Response_ptr res)
  {
    if(err) printf("<Client> Error: %s\n", err.to_string().c_str());
    if(!res)
    {
      printf("<Client> No reply.\n");
      //assert(false && "Exiting...");
    }
    else
    {
      if(is_valid_response(*res))
      {
        printf("<Client> Valid Response (payload %u bytes)\n", res->body().size());
      }
      else
      {
        printf("<Client> Invalid response:\n%s\n", res->to_string().c_str());
        //assert(false && "Exiting...");
      }
    }

    context_.response = std::move(res);
    run_state();
  }

  bool Client::is_valid_response(const http::Response& res) const
  {
    return is_json(res) or is_artifact(res);
  }

  bool Client::is_json(const http::Response& res) const
  {
    return res.header().value(http::header::Content_Type).find("application/json") != std::string::npos;
  }

  bool Client::is_artifact(const http::Response& res) const
  {
    return res.header().value(http::header::Content_Type).find("application/vnd.mender-artifact") != std::string::npos;
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
      API_PREFIX + "/deployments/device/deployments/next",
      headers, {this, &Client::response_handler});
  }

  void Client::fetch_update(http::Response_ptr res)
  {
    if(res == nullptr)
      res = std::move(context_.response);

    auto uri = parse_update_uri(*res);
    printf("<Client> Fetching update from: %s\n", uri.to_string().to_string().c_str());

    using namespace http;

    const auto& token = am_.auth_token();
    // Setup headers
    const Header_set headers{
      { header::Accept, "application/json;application/vnd.mender-artifact" },
      { header::Authorization, "Bearer " + std::string{token.begin(), token.end()}}
    };

    httpclient_->request(GET, {cached_.address(), uri.port()},
      uri.path().to_string() + "?" + uri.query().to_string(), // note: Add query support in http(?)
      headers, {this, &Client::response_handler});
  }

  http::URI Client::parse_update_uri(http::Response& res)
  {
    using namespace nlohmann;
    auto body = json::parse(res.body().to_string());

    std::string uri = body["artifact"]["source"]["uri"];

    return http::URI{uri};
  }

  void Client::update_inventory_attributes()
  {
    printf("<Client> Uploading attributes\n");

    using namespace http;

    const auto& token = am_.auth_token();
    // Setup headers
    const Header_set headers{
      { header::Content_Type, "application/json" },
      { header::Accept, "application/json" },
      { header::Authorization, "Bearer " + std::string{token.begin(), token.end()}}
    };

    auto data = device_.inventory().json_str();

    httpclient_->request(PATCH, cached_,
      "/api/devices/0.1/inventory/device/attributes",
      headers, data, {this, &Client::response_handler});

    context_.last_inventory_update = RTC::now();
  }

  void Client::save_server_state(liu::Storage storage, liu::buffer_len len)
  {
    // save seq here, whever

    printf("<Live Update> Saving server state\n");
  }

  void Client::install_update(http::Response_ptr res)
  {
    printf("<Client> Installing update\n");

    if(res == nullptr)
      res = std::move(context_.response);
    assert(res);

    auto data = res->body().to_string();

    // Process data:

    tar::Reader reader;
    tar::Tar& read_data = reader.read_uncompressed(data.data(), data.size());

    for (auto element : read_data.elements()) {
      tar::Reader tgzr;

      // If this element/file is a .tar.gz file: decompress it and store content in a new Tar object
      if (element.name().size() > 7 and element.name().substr(element.name().size() - 7) == ".tar.gz") {
        tar::Tar& read_compressed = tgzr.decompress(element);

        // Loop through the elements of the tar.gz file and find the .img file and pass on
        for (auto e : read_compressed.elements()) {
          if (e.name().size() > 4 and e.name().substr(e.name().size() - 4) == ".img") {
            printf("<Client> Found img file\n");

            // Sending the IncludeOS image to LiveUpdate
            liu::LiveUpdate::begin(LIVEUPD_LOCATION, {e.content(), e.size()}, save_server_state);
          }
        }
      }
    }
  }
};
