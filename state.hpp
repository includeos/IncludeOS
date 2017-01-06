
#pragma once

#ifndef MENDER_STATE_HPP
#define MENDER_STATE_HPP

#include <util/timer.hpp>
#include <net/http/response.hpp>
#include <net/http/error.hpp>

namespace mender {
  class Client;

  namespace state {

    struct Context {
      // Latest response
      http::Response_ptr response;
      // timer stuff
      Timer timer;
      int delay = 0;

      void clear()
      {
        response = nullptr;
        timer.stop();
        delay = 0;
      }


    }; // struct Context

    class State {
    public:
      enum Result
      {
        GO_NEXT,
        AWAIT_EVENT,
        DELAYED_NEXT
      };

      virtual Result handle(Client&, Context&) = 0;
      virtual std::string to_string() const = 0;
    protected:
      explicit State() = default;

      void set_state(Client&, State&);

      template<typename NewState, typename Client>
      void set_state(Client& cli)
      {
        static_assert(std::is_base_of<State, NewState>::value, "NewState is not a State");
        auto& s = NewState::instance();
        cli.set_state(s);
        printf("<State> New state: %s\n", s.to_string().c_str());
      }

    }; // < class State



    class Init : public State {
    public:
      static State& instance()
      { static Init state_; return state_; }

      Result handle(Client&, Context&) override;
      std::string to_string() const override { return "Init"; }
    };

    class Auth_wait : public State {
    public:
      static State& instance()
      { static Auth_wait state_; return state_; }

      Result handle(Client&, Context&) override;
      std::string to_string() const override { return "Auth_wait"; }
    };

    class Authorized : public State {
    public:
      static State& instance()
      { static Authorized state_; return state_; }

      Result handle(Client&, Context&) override;
      std::string to_string() const override { return "Authorized"; }
    };
  }


} // < namespace mender

#endif // < MENDER_STATE_HPP
