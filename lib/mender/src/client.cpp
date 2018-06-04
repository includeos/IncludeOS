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

#include <mender/client.hpp>
#include <mender/artifact.hpp>

#include <botan/base64.h>
#include <rapidjson/document.h>
#include <rtc> // RTC::now()

namespace mender {

  Client::Client(Auth_manager&& man, Device&& dev, net::TCP& tcp, const std::string& server, const uint16_t port)
    : am_{std::forward<Auth_manager>(man)},
      device_{std::forward<Device>(dev)},
      server_(server),
      cached_{net::Addr{}, port},
      httpclient_{std::make_unique<http::Basic_client>(tcp)},
      state_(&state::Init::instance()),
      context_({this, &Client::run_state}),
      on_state_store_(nullptr),
      on_state_resume_(nullptr)
  {
    MENDER_INFO("Client", "Client created");
    liu::LiveUpdate::register_partition("mender", {this, &Client::store_state});
  }

  Client::Client(Auth_manager&& man, Device&& dev, net::TCP& tcp, net::Socket socket)
    : am_{std::forward<Auth_manager>(man)},
      device_{std::forward<Device>(dev)},
      server_{socket.address().to_string()},
      cached_(std::move(socket)),
      httpclient_{std::make_unique<http::Basic_client>(tcp)},
      state_(&state::Init::instance()),
      context_({this, &Client::run_state}),
      on_state_store_(nullptr),
      on_state_resume_(nullptr)
  {
    MENDER_INFO("Client", "Client created");
    liu::LiveUpdate::register_partition("mender", {this, &Client::store_state});
  }

  void Client::boot()
  {
    if(liu::LiveUpdate::is_resumable())
    {
      MENDER_INFO("Client", "Found resumable state, try restoring...");
      auto success = liu::LiveUpdate::resume("mender", {this, &Client::load_state});
      if(!success)
        MENDER_INFO2("Failed.");
    }

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

    MENDER_INFO("Client", "Making Auth request");
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

  void Client::response_handler(http::Error err, http::Response_ptr res, http::Connection&)
  {
    if(err) {
      MENDER_INFO("Client", "Error: %s", err.to_string().c_str());
      set_state(state::Error_state::instance(*state_));
    }
    if(!res)
    {
      MENDER_INFO("Client", "No reply.");
      //assert(false && "Exiting...");
    }
    else
    {
      if(is_valid_response(*res))
      {
        MENDER_INFO("Client", "Valid Response (payload %u bytes)", (uint32_t)res->body().size());
      }
      else
      {
        MENDER_INFO("Client", "Invalid response:\n%s", res->to_string().c_str());
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
    MENDER_INFO("Client", "Checking for update");

    using namespace http;

    const auto& token = am_.auth_token();
    // Setup headers
    const Header_set headers{
      { header::Content_Type, "application/json" },
      { header::Accept, "application/json" },
      { header::Authorization, "Bearer " + std::string{token.begin(), token.end()}}
    };

    std::string path{API_PREFIX + "/deployments/device/deployments/next"};

    auto artifact_name = device_.inventory().value("artifact_name");
    path.append("?artifact_name=").append(std::move(artifact_name)).append("&");

    auto device_type = device_.inventory().value("device_type");
    path.append("device_type=").append(std::move(device_type));

    httpclient_->get(cached_,
      std::move(path),
      headers, {this, &Client::response_handler});
  }

  void Client::fetch_update(http::Response_ptr res)
  {
    if(res == nullptr)
      res = std::move(context_.response);

    auto uri = parse_update_uri(*res);
    MENDER_INFO("Client", "Fetching update from: %s",
                uri.to_string().c_str());

    using namespace http;

    const auto& token = am_.auth_token();
    // Setup headers
    const Header_set headers{
      { header::Accept, "application/json;application/vnd.mender-artifact" },
      { header::Authorization, "Bearer " + std::string{token.begin(), token.end()}},
      { header::Host, uri.host_and_port() }
    };

    httpclient_->get({cached_.address(), uri.port()},
      std::string(uri.path()) + "?" + std::string(uri.query()),
      headers, {this, &Client::response_handler});
  }

  http::URI Client::parse_update_uri(http::Response& res)
  {
    using namespace rapidjson;
    Document d;
    d.Parse(std::string(res.body()).c_str());
    return http::URI{d["artifact"]["source"]["uri"].GetString()};
  }

  void Client::update_inventory_attributes()
  {
    MENDER_INFO("Client", "Uploading attributes");

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
      API_PREFIX + "/inventory/device/attributes",
      headers, data, {this, &Client::response_handler});

    context_.last_inventory_update = RTC::now();
  }

  void Client::install_update(http::Response_ptr res)
  {
    MENDER_INFO("Client", "Installing update ...");

    if(res == nullptr)
      res = std::move(context_.response);
    assert(res);

    auto data = std::string(res->body());

    // Process data:
    Artifact artifact{{data.begin(), data.end()}};

    // do stuff with artifact
    // checksum/verify
    // artifact.update() <- this is what liveupdate wants

    //artifact.verify();
    const auto& blob = artifact.get_update_blob(0);  // returns element with index
    auto tar = tar::Reader::read(blob.data(), blob.size());
    const auto& e = *tar.begin();

    device_.inventory("artifact_name") = artifact.name();

    httpclient_.release();

    const char* content = reinterpret_cast<const char*>(e.content());
    liu::LiveUpdate::exec(liu::buffer_t{content, content + e.size()});
  }

  void Client::store_state(liu::Storage& store, const liu::buffer_t*)
  {
    liu::Storage::uid id = 0;

    // ARTIFACT_NAME
    store.add_string(id++, device_.inventory("artifact_name"));

    if(on_state_store_ != nullptr)
      on_state_store_(store);

    MENDER_INFO("Client", "State stored.");
  }

  void Client::load_state(liu::Restore& store)
  {
    // ARTIFACT_NAME
    device_.inventory("artifact_name") = store.as_string(); store.go_next();

    if(on_state_resume_ != nullptr)
      on_state_resume_(store);

    MENDER_INFO("Client", "State restored.");
  }

};
