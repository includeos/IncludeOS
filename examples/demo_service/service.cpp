// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <unordered_set>
#include <tuple>
#include <string>

#include <os>
#include <hw/serial.hpp>
#include <net/inet4>
#include <util/fixed_queue.hpp>
#include <timers>
#include <memdisk>


struct server_data
{
	explicit server_data(fs::Buffer buf) noexcept :
		html_file(buf)
	{}

	std::unordered_set<net::tcp::Connection_ptr> open_connections;
	int64_t page_requests = 0;
	util::fixed_queue<std::string, 10> client_agents;
	fs::Buffer html_file;
};

class tcp_server
{
public:
	explicit tcp_server(net::tcp::Listener& tcp_listener, fs::Buffer buf) :
		tcp_listener_(tcp_listener),
		state_(std::make_shared<server_data>(buf))
	{}

	tcp_server(const tcp_server&) = delete;
	tcp_server(tcp_server&&) = default;

	tcp_server& operator= (const tcp_server&) = delete;
	tcp_server& operator= (tcp_server&&) = default;

	~tcp_server() = default;

	net::tcp::Listener& tcp_listener() { return tcp_listener_; }
	std::shared_ptr<server_data> state() { return state_; }

private:
	net::tcp::Listener& tcp_listener_;
	std::shared_ptr<server_data> state_;
};

std::string generate_demo_page(std::shared_ptr<server_data> server_state)
{
	return
	{
	server_state->html_file.to_string() +
	"<div class=\"main\">"
		"<div class=\"visitor_info\">"
		"<span style=\"font-size: 1.6em\">--- Recent visitors ---</span><hr>"
		"<span class=\"info_text\">"
		+ [&server_state] {
			auto s = util::merge_ring_range(server_state->client_agents);
			s.resize(s.size() - sizeof("<hr>"));
			return s;
		}()
		+
		"</span><hr>"
		"</div>"
	"</div>"
	"<footer class=\"main\">"
		"<span class=\"server_info\">"
			"Open connections: <span style=\"color: #BF2D42;\">"
			+ std::to_string(server_state->open_connections.size()) +
			"</span><br><hr>"
			"Page requests: <span style=\"color: #BF2D42;\">"
			+ std::to_string(server_state->page_requests) +
			"</span>"
		"</span>"
		"&copy; 2017 Include<span style=\"color: #BF2D42;\">OS</span>"
	"</footer>"
	"</div>"
	"</body>"
	"</html>"
	};
}

std::string generate_demo_header(size_t html_size, bool keep_open)
{
	return{
		"HTTP/1.1 200 OK\n"
		"Date: Mon, 01 Jan 1970 00:00:01 GMT\n"
		"Server: IncludeOS prototype 4.0\n"
		"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n"
		"Content-Type: text/html; charset=UTF-8\n"
		"Content-Length: " + std::to_string(html_size) + "\n"
		"Accept-Ranges: bytes\n"
		+ std::string{ keep_open
		? "Connection: Keep-Alive\n\n"
		: "Connection: close\n\n"
	}
	};
}

std::string html_response(
	std::shared_ptr<server_data> server_state
)
{
	auto clients = util::merge_ring_range(server_state->client_agents);
	clients.resize(clients.size() - 4); // remove last <hr>

	const std::string html = generate_demo_page(server_state);

	const std::string header = generate_demo_header(html.size(), true);

	return header + html;
}

std::string not_found()
{
	return "HTTP/1.1 404 Not Found\nConnection: close\n\n";
}

std::string create_server_response(
	const std::string& client_response,
	std::shared_ptr<server_data> server_state
)
{
	if (client_response.find("GET / ") == std::string::npos)
		return not_found();

	++server_state->page_requests;

	auto start = client_response.find("User-Agent: ");
	auto end = client_response.find("Accept:");

	server_state->client_agents.push_back(
		start != std::string::npos && end > start
		? client_response.substr(start, end - start) + "<hr>"
		: "INVALID<hr>"
	);

	return html_response(server_state);
}

void write_callback(size_t n, std::string socket_name)
{
	printf("<Service> - Sent %u bytes to:\t\t%s\n",
		n,
		socket_name.c_str()
	);
}

void setup_read(
	net::tcp::Connection_ptr conn_ptr,
	std::shared_ptr<server_data> server_state
)
{
	conn_ptr->on_read(1024,
		[conn_ptr, server_state](net::tcp::buffer_t buf, size_t n) -> void
	{
		server_state->open_connections.insert(conn_ptr);

		std::string client_response{
			reinterpret_cast<char*>(buf.get()),
			n
		};

		std::string server_response = create_server_response(
			client_response,
			server_state
		);

		conn_ptr->write(
			server_response.data(), server_response.size(),
			[conn_ptr](size_t n)
		{ write_callback(n, conn_ptr->remote().to_string()); }
		);
	}
	);
}

void disconnect(
	net::tcp::Connection_ptr conn_ptr,
	const std::string& reason
)
{
	conn_ptr->close();

	printf("<Service> - Disconnected:\t\t%s - Reason: %s\n\n",
		conn_ptr->remote().to_string().c_str(),
		reason.c_str()
	);
}

void setup_disconnect(
	net::tcp::Connection_ptr conn_ptr,
	std::shared_ptr<server_data> server_state
)
{
	conn_ptr->on_disconnect(
		[server_state](auto conn_ptr, auto reason) -> void
	{
		disconnect(conn_ptr, reason.to_string());
		//server_state->open_connections.erase(conn_ptr);
	}
	).on_error([](auto err) -> void
	{
		printf("<Service> - Disconnect error: %s\n", err.what());
	}
	);
}

bool validate_connection(net::tcp::Socket socket)
{
	printf("<Service> - Connection attempt from:\t%s\n",
		socket.to_string().c_str());

	return true; // allow all connections
}

void setup_tcp_handler(tcp_server& server)
{
	auto& ref = server.tcp_listener().on_accept(
		validate_connection
		//[](auto) { return true; }
	);

	ref.on_connect(
		[server_state = server.state()](auto conn_ptr) -> void
	{
		const auto socket_name = conn_ptr->remote().to_string();
		printf("<Service> - Connected:\t\t\t%s\n", socket_name.c_str());

		setup_read(conn_ptr, server_state);
		setup_disconnect(conn_ptr, server_state);
	}
	);
}

void setup_shutdown(tcp_server& server)
{
	auto& com = hw::Serial::port<1>();
	com.on_readline(
		[server_state = server.state()](const std::string& line) -> void
	{
		printf("\n");

		if (line != "q")
		{
			printf("unrecognized command\n");
			return;
		}

		printf("----- service shutdown started -----\n\n");

		for (auto& conn_ptr : server_state->open_connections)
			disconnect(conn_ptr, "Server shutdown");

		printf("----- service shutdown complete -----\n\n");

		OS::shutdown();
	}
	);
}

tcp_server create_server(fs::Buffer buf)
{
	// static IP
	auto& inet = net::Inet4::ifconfig(
		net::IP4::addr{ 10,0,0,42 },     // IP
		net::IP4::addr{ 255,255,255,0 }, // Netmask
		net::IP4::addr{ 10,0,0,1 },      // Gateway
		net::IP4::addr{ 8,8,8,8 }		 // DNS
	);

	return tcp_server{ inet.tcp().bind(80), std::move(buf) };
}

void Service::start(const std::string&)
{
	fs::Disk_ptr disk_ptr = fs::new_shared_memdisk();

	disk_ptr->init_fs([disk_ptr](fs::error_t err)
	{
		if (err)  panic("Could not mount filesystem, retreating...\n");

		auto server = create_server(disk_ptr->fs().read_file("index.html"));
		setup_tcp_handler(server);
		setup_shutdown(server); ;
	}
	);

	Timers::oneshot(std::chrono::milliseconds(10),
		[](auto) -> void
	{
		printf(
			"\n----- TEST SERVICE STARTED -----\n\n"
			"Open 10.0.0.42 in a browser to access service\n"
			"Enter 'q' to shutdown service\n\n"
		);
	}
	);
}
