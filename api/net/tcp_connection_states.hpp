// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_TCP_CONNECTION_STATES_HPP
#define NET_TCP_CONNECTION_STATES_HPP

#include <net/tcp.hpp>
///////////////// CONCRETE STATES /////////////////
using TCP = net::TCP;
using Connection = TCP::Connection;
using State = TCP::Connection::State;

/*
	CLOSED
*/
class Connection::Closed : public State {
public:
	inline static State& instance() {
		static Closed instance;
		return instance;
	}
	/*
		<- Do nothing (Start listening).

		=> Listen.
	*/
	//virtual void passive_open(TCP::Connection*);
	/*
		<- Send SYN.

		=> SynSent.
	*/
	//virtual void active_open(TCP::Connection*);
	
	/*
		(Do we need a parameter for ACTIVE/PASSIVE close, or always transit from Closed -> Listen -> SynSent.)
		
		PASSIVE:		
		<- Do nothing (Start listening).

		=> Listen.

		ACTIVE:
		<- Send SYN.

		=> SynSent
	*/
	virtual void open(TCP::Connection&, TCP::Packet_ptr out) override;

	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	inline std::string to_string() const override {
		return "CLOSED";
	};
private:
	inline Closed() {};
};

/*
	LISTEN
*/
class Connection::Listen : public State {
public:
	inline static State& instance() {
		static Listen instance;
		return instance;
	}
	virtual void open(TCP::Connection&, TCP::Packet_ptr out) override;
	/*
		-> Receive SYN.

		<- Send SYN+ACK.

		=> SynReceived.
	*/
	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	inline std::string to_string() const override {
		return "LISTENING";
	};
private:
	inline Listen() {};
};

/*
	SYN-SENT
*/
class Connection::SynSent : public State {
public:
	inline static State& instance() {
		static SynSent instance;
		return instance;
	}
	/*
		-> Receive SYN+ACK

		<- Send ACK.

		=> Established.
	*/
	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	inline std::string to_string() const override {
		return "SYN-SENT";
	};
private:
	inline SynSent() {};
};

/*
	SYN-RCV
*/
class Connection::SynReceived : public State {
public:
	inline static State& instance() {
		static SynReceived instance;
		return instance;
	}

	/*
		-> Receive ACK.

		<- Do nothing (Connection is Established)

		=> Established.
	*/
	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	inline std::string to_string() const override {
		return "SYN-RCV";
	};

private:
	inline SynReceived() {};
};

/*
	ESTABLISHED
*/
class Connection::Established : public State {
public:
	inline static State& instance() {
		static Established instance;
		return instance;
	}
	/*
		
	*/
	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	/*
		-> Receive FIN.

		<- Send ACK.

		=> CloseWait
	*/
	// What if we wanna close?? => FinWait1
	virtual void close(TCP::Connection&, TCP::Packet_ptr) override;

	inline std::string to_string() const override {
		return "ESTABLISHED";
	};

private:
	inline Established() {};
};

/*
	CLOSE-WAIT
*/
class Connection::CloseWait : public State {
public:
	inline static State& instance() {
		static CloseWait instance;
		return instance;
	}
	/*
		-> Nothing I think...

		<- Send FIN.
		
		=> LastAck
	*/
	virtual void close(TCP::Connection&, TCP::Packet_ptr) override;

	inline std::string to_string() const override {
		return "CLOSE-WAIT";
	};

private:
	inline CloseWait() {};
};

/*
	FIN-WAIT-1
*/
class Connection::FinWait1 : public State {
public:
	inline static State& instance() {
		static FinWait1 instance;
		return instance;
	}
	/*
		-> Receive ACK.

		=> FinWait2.
	*/
	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	inline std::string to_string() const override {
		return "FIN-WAIT-1";
	};

private:
	inline FinWait1() {};
};

/*
	FIN-WAIT-1
*/
class Connection::FinWait2 : public State {
public:
	inline static State& instance() {
		static FinWait2 instance;
		return instance;
	}
	/*
		
	*/
	
	inline std::string to_string() const override {
		return "FIN-WAIT-2";
	};
private:
	inline FinWait2() {};
};

/*
	LAST-ACK
*/
class Connection::LastAck : public State {
public:
	inline static State& instance() {
		static LastAck instance;
		return instance;
	}
	/*
		-> Receive ACK.

		<- conn.onClose();

		=> Closed (Tell TCP to remove this connection)
	*/
	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	inline std::string to_string() const override {
		return "LAST-ACK";
	};

private:
	inline LastAck() {};
};

/*
	CLOSING
*/
class Connection::Closing : public State {
public:
	inline static State& instance() {
		static Closing instance;
		return instance;
	}
	/*
		-> Receive ACK.

		=> TimeWait (Guess this isnt needed, just start a Close-timer)
	*/
	virtual int handle(TCP::Connection&, TCP::Packet_ptr in, TCP::Packet_ptr out) override;

	inline std::string to_string() const override {
		return "CLOSING";
	};

private:
	inline Closing() {};
};

/*
	TIME-WAIT
*/
class Connection::TimeWait : public State {
public:
	inline static State& instance() {
		static TimeWait instance;
		return instance;
	}
	/*
		
	*/


	inline std::string to_string() const override {
		return "TIME-WAIT";
	};

private:
	inline TimeWait() {};
};

#endif