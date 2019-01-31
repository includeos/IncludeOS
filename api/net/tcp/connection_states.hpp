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

#pragma once
#ifndef NET_TCP_CONNECTION_STATES_HPP
#define NET_TCP_CONNECTION_STATES_HPP

#include <net/tcp/connection.hpp>

namespace net {
namespace tcp {

///////////////// CONCRETE STATES /////////////////

using State = Connection::State;

/*
  CLOSED
*/
class Connection::Closed : public State {
public:
  inline static State& instance() {
    static Closed instance;
    return instance;
  }

  virtual void open(Connection&, bool active = false) override;

  virtual size_t send(Connection&, WriteBuffer&) override;

  /*
    PASSIVE:
    <- Do nothing (Start listening).

    => Listen.

    ACTIVE:
    <- Send SYN.

    => SynSent
  */
  Result handle(Connection&, Packet_view& in) override;

  bool is_closed() const override {
    return true;
  }

  std::string to_string() const override {
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
  virtual void open(Connection&, bool active = false) override;

  virtual size_t send(Connection&, WriteBuffer&) override;

  virtual void close(Connection&) override;
  /*
    -> Receive SYN.

    <- Send SYN+ACK.

    => SynReceived.
  */
  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
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

  virtual size_t send(Connection&, WriteBuffer&) override;

  virtual void close(Connection&) override;
  /*
    -> Receive SYN+ACK

    <- Send ACK.

    => Established.
  */
  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
    return "SYN-SENT";
  };

  virtual bool is_writable() const override
  { return true; }

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

  virtual size_t send(Connection&, WriteBuffer&) override;

  virtual void close(Connection&) override;

  virtual void abort(Connection&) override;
  /*
    -> Receive ACK.

    <- Do nothing (Connection is Established)

    => Established.
  */
  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
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

  virtual size_t send(Connection&, WriteBuffer&) override;

  virtual void close(Connection&) override;

  virtual void abort(Connection&) override;

  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
    return "ESTABLISHED";
  };

  inline virtual bool is_connected() const override {
    return true;
  }

  inline virtual bool is_writable() const override {
    return true;
  }

  inline virtual bool is_readable() const override {
    return true;
  }

private:
  inline Established() {};
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

  virtual void close(Connection&) override;

  virtual void abort(Connection&) override;

  /*
    -> Receive ACK.

    => FinWait2.
  */
  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
    return "FIN-WAIT-1";
  };

  inline virtual bool is_readable() const override {
    return true;
  }

  inline virtual bool is_closing() const override {
    return true;
  }

private:
  inline FinWait1() {};
};

/*
  FIN-WAIT-2
*/
class Connection::FinWait2 : public State {
public:
  inline static State& instance() {
    static FinWait2 instance;
    return instance;
  }

  virtual void close(Connection&) override;

  virtual void abort(Connection&) override;
  /*

   */
  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
    return "FIN-WAIT-2";
  };

  inline virtual bool is_readable() const override {
    return true;
  }

  inline virtual bool is_closing() const override {
    return true;
  }

private:
  inline FinWait2() {};
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

  virtual size_t send(Connection&, WriteBuffer&) override;

  virtual void close(Connection&) override;

  virtual void abort(Connection&) override;
  /*
    -> Nothing I think...

    <- Send FIN.

    => LastAck
  */
  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
    return "CLOSE-WAIT";
  };

  inline virtual bool is_writable() const override {
    return true;
  }

private:
  inline CloseWait() {};
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
  virtual Result handle(Connection&, Packet_view& in) override;

  inline virtual std::string to_string() const override {
    return "CLOSING";
  };

  inline virtual bool is_closing() const override {
    return true;
  }

private:
  inline Closing() {};
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
  virtual Result handle(Connection&, Packet_view& in) override;

  std::string to_string() const override {
    return "LAST-ACK";
  };

  bool is_closing() const override {
    return true;
  }

  bool is_closed() const override {
    return true;
  }

private:
  inline LastAck() {};
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
  virtual Result handle(Connection&, Packet_view& in) override;

  std::string to_string() const override {
    return "TIME-WAIT";
  };

  bool is_closing() const override {
    return true;
  }
  bool is_closed() const override {
    return true;
  }

private:
  inline TimeWait() {};
};

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_CONNECTION_STATES_HPP
