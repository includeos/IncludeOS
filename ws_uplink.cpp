
#include "ws_uplink.hpp"
#include "common.hpp"

#include <memdisk>

namespace uplink {

  const std::string WS_uplink::UPLINK_CFG_FILE{"uplink.json"};

  WS_uplink::WS_uplink(net::Inet<net::IP4>& inet)
    : parser_({this, &WS_uplink::handle_transport})
  {
    Expects(inet.ip_addr() != 0 && "Network interface not configured");
    start(inet);
  }

  void WS_uplink::start(net::Inet<net::IP4>& inet) {
    MYINFO("Starting WS uplink on %s", inet.ifname().c_str());

    client_ = std::make_unique<http::Client>(inet.tcp(), 
      http::Client::Request_handler{this, &WS_uplink::inject_token});
    
    auth();
  }

  void WS_uplink::auth()
  {
    std::string url{"http://"};
    url.append(config_.server).append("/auth");

    static const std::string auth_data{"{ \"id\": \"testor\", \"key\": \"kappa123\"}"};

    client_->post(http::URI{"http://" + config_.server + "/auth"}, 
      { {"Content-Type", "application/json"} }, 
      auth_data, 
      {this, &WS_uplink::handle_auth_response});
  }

  void WS_uplink::handle_auth_response(http::Error err, http::Response_ptr res, http::Connection&)
  {
    if(err)
    {
      MYINFO("Auth failed - %s\n", err.to_string().c_str());
      return;
    }

    if(res->status_code() != http::OK)
    {
      MYINFO("Auth failed - %s\n", res->to_string().c_str());
      return;
    } 
    
    MYINFO("Auth success (token received)");
    token_ = res->body().to_string();

    dock();
  }

  void WS_uplink::dock()
  {
    Expects(not token_.empty() and client_ != nullptr);

    std::string url{"ws://"};
    url.append(config_.server).append("/dock");

    http::WebSocket::connect(*client_, http::URI{url}, {this, &WS_uplink::establish_ws});
  }

  void WS_uplink::establish_ws(http::WebSocket_ptr ws)
  {
    if(ws == nullptr) {
      MYINFO("Failed to establish websocket");
      return;
    }

    ws_ = std::move(ws);
    ws_->on_read = {this, &WS_uplink::parse_transport};

    MYINFO("Websocket established");
  }

  void WS_uplink::parse_transport(const char* data, size_t len)
  {
    printf("Received data %lu\n", len);
    parser_.parse(data, len);
  }

  void WS_uplink::handle_transport(Transport_ptr t)
  {
    if(UNLIKELY(t == nullptr))
    {
      printf("Something went terribly wrong...\n");
      return;
    }
    
    MYINFO("New transport (%zu bytes)", t->size());
    switch(t->code())
    {
      case Transport_code::UPDATE:
      {
        auto msg = t->message();
        INFO2("Received update: %s\n", msg.c_str());
        return;
      }

      default:
      {
        INFO2("Bad transport\n");
      }
    }
  }

  void WS_uplink::read_config()
  {
    MYINFO("Reading uplink config from %s", UPLINK_CFG_FILE.c_str());
    auto& disk = fs::memdisk();
    
    if (not disk.fs_ready())
    {
      disk.init_fs([] (auto err, auto&) {
        Expects(not err && "Error occured when mounting FS.");
      });  
    }

    auto cfg = disk.fs().stat(UPLINK_CFG_FILE); // name hardcoded for now
    
    Expects(cfg.is_file() && "File not found.");
    Expects(cfg.size() && "File is empty.");

    auto content = cfg.read();

    parse_config(content);
  }

  void WS_uplink::parse_config(const std::string& cfg)
  {

  }
}