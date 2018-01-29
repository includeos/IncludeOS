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

#ifndef MANA_ROUTER_HPP
#define MANA_ROUTER_HPP

#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "route.hpp"
#include "params.hpp"

namespace mana {

  //-------------------------------
  // This class is used to provide
  // route resolution
  //-------------------------------
  class Router {
  private:
    //-------------------------------
    // Internal class type aliases
    //using Span        = gsl::span<char>;
    using Route_table = std::unordered_map<http::Method, std::vector<Route>>;
    //-------------------------------
  public:

    /**
     * @brief      Returned in match-method.
     *             Contains both the End_point and the route parameters so that both can be returned.
     */
    struct ParsedRoute {
      End_point job;
      Params    parsed_values;
    };

    //-------------------------------
    // Default constructor to set up
    // default routes
    //-------------------------------
    explicit Router() = default;

    //-------------------------------
    // Default destructor
    //-------------------------------
    ~Router() noexcept = default;

    //-------------------------------
    // Default move constructor
    //-------------------------------
    Router(Router&&) = default;

    //-------------------------------
    // Default move assignment operator
    //-------------------------------
    Router& operator = (Router&&) = default;

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_options(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_get(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_head(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_post(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_put(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_delete(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_trace(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_connect(Routee&& route, End_point result);

    //-------------------------------
    // Add a route mapping for route
    // resolution upon request
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on_patch(Routee&& route, End_point result);

    //-------------------------------
    // General way to add a route mapping for route
    // resolution upon request
    //
    // @param method - HTTP method
    //
    // @tparam (std::string) route - The route to map unto a
    //                               resulting destination
    //
    // @param result - The route mapping
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee>
    Router& on(http::Method method, Routee&& route, End_point result);

    //-------------------------------
    // Install a new route table for
    // route resolutions
    //
    // @tparam (http::Router) new_routes - The new route table
    //                                     to install
    //
    // @return - The object that invoked this method
    //-------------------------------
    template <typename Routee_Table>
    Router& install_new_configuration(Routee_Table&& new_routes);


    /**
     * Get the route callback where Route_expr matched a given path
     *
     * @param path : the route path
     * @note : not const becuase it uses index operator to a map
     **/
    inline ParsedRoute match(http::Method, const std::string&);

    /**
     * @brief Make the router use another router on a given route
     * @details Currently only copies the content from the outside
     * Router and adds new Route in RouteTable by combining
     * root route and the route to the other Route.
     *
     * Maybe Router should be able to keep a collection of other routers.
     *
     * @param Routee Root path
     * @param Router another router with Routes
     *
     * @return this Router
     */
    template <typename Routee>
    Router& use(Routee&&, const Router&);

    /**
     * @brief Copies Routes from another Router object
     *
     * @param  Router to be copied from
     * @return this Router
     */
    Router& add(const Router&);

    /**
     * @brief Optimize route search for all routes by bringing
     * the most hitted route to the front of the search queue
     *
     * @return The object that invoked this method
     */
    Router& optimize_route_search();

    /**
     * @brief Optimize route search for the specified HTTP method
     * by bringing the most hitted route to the front of the
     * search queue
     *
     * @param method
     * The HTTP method to optimize search for
     *
     * @return The object that invoked this method
     */
    Router& optimize_route_search(const http::Method method);

    Router& operator<<(const Router& obj)
    { return add(obj); }

    std::string to_string() const;

  private:

    Router(const Router&) = delete;
    Router& operator = (const Router&) = delete;

    Route_table route_table_;

  }; //< class Router

  class Router_error : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  /**--v----------- Implementation Details -----------v--**/

  template <typename Routee>
  inline Router& Router::on_options(Routee&& route, End_point result) {
    route_table_[http::OPTIONS].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_get(Routee&& route, End_point result) {
    route_table_[http::GET].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_head(Routee&& route, End_point result) {
    route_table_[http::HEAD].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_post(Routee&& route, End_point result) {
    route_table_[http::POST].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_put(Routee&& route, End_point result) {
    route_table_[http::PUT].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_delete(Routee&& route, End_point result) {
    route_table_[http::DELETE].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_trace(Routee&& route, End_point result) {
    route_table_[http::TRACE].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_connect(Routee&& route, End_point result) {
    route_table_[http::CONNECT].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on_patch(Routee&& route, End_point result) {
    route_table_[http::PATCH].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee>
  inline Router& Router::on(http::Method method, Routee&& route, End_point result) {
    route_table_[method].emplace_back(std::forward<Routee>(route), result);
    return *this;
  }

  template <typename Routee_Table>
  inline Router& Router::install_new_configuration(Routee_Table&& new_routes) {
    route_table_ = std::forward<Routee_Table>(new_routes).route_table_;
    return *this;
  }

  inline Router::ParsedRoute Router::match(http::Method method, const std::string& path) {
    auto routes = route_table_[method];

    if (routes.empty()) {
      throw Router_error("No routes for method " + std::string(http::method::str(method)));
    }

    for (auto& route : routes) {
      if (std::regex_match(path, route.expr)) {
        ++route.hits;

        // Set the pairs in params:
        Params params;
        std::smatch res;

        for (std::sregex_iterator i = std::sregex_iterator{path.begin(), path.end(), route.expr};
          i != std::sregex_iterator{}; ++i) { res = *i; }

        // First parameter/value is in res[1], second in res[2], and so on
        for (size_t i = 0; i < route.keys.size(); i++)
          params.insert(route.keys[i].name, res[i + 1]);

        ParsedRoute parsed_route;
        parsed_route.job = route.end_point;
        parsed_route.parsed_values = params;

        return parsed_route;
      }
    }

    throw Router_error("No matching route for " + std::string(http::method::str(method)) + " " + path);
  }

  template <typename Routee>
  inline Router& Router::use(Routee&& root, const Router& router) {
    // pair<Method, vector<Route>>
    for(auto& method_routes : router.route_table_)
    {
      auto& method = method_routes.first;
      auto& routes = method_routes.second;
      // vector<Route>
      for(auto& route : routes)
      {
        std::string path = root + route.path;
        on(method, path, route.end_point);
      }
    }
    return *this;
  }

  inline Router& Router::add(const Router& router) {
    for (const auto& e : router.route_table_) {
      auto it = route_table_.find(e.first);
      if (it not_eq route_table_.end()) {
        it->second.insert(it->second.cend(), e.second.cbegin(), e.second.cend());
        continue;
      }
      route_table_[e.first] = e.second;
    }
    return *this;
  }

  inline Router& Router::optimize_route_search() {
    auto it  = route_table_.begin();
    auto end = route_table_.end();

    while (it not_eq end) {
      std::stable_sort(it->second.begin(), it->second.end());
      ++it;
    }

    return *this;
  }

  inline Router& Router::optimize_route_search(const http::Method method) {
    auto it = route_table_.find(method);
    if (it not_eq route_table_.end()) {
      std::stable_sort(it->second.begin(), it->second.end());
    }
    return *this;
  }

  inline std::string Router::to_string() const {
    std::ostringstream ss;

    for(const auto& method_routes : route_table_) {
      auto&& method = method_routes.first;
      auto&& routes = method_routes.second;
      for(auto&& route : routes) {
        ss << method << '\t' << route.path << '\n';
      }
    }

    return ss.str();
  }

  /**--^----------- Implementation Details -----------^--**/

} //< namespace mana


#endif //< MANA_ROUTER_HPP
