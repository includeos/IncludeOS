# cookie
Cookie support for [Mana](https://github.com/includeos/mana). Following [RFC 6265](https://tools.ietf.org/html/rfc6265).

**Example service:** [Acorn Web Server Appliance](https://github.com/includeos/acorn) with its [routes that use cookies](https://github.com/includeos/acorn/blob/master/app/routes/languages.hpp).

## Features
* Create, update and clear cookies
* Easy access to an incoming request's existing cookies

## Usage
**Create** a cookie with a name and value on the [response](https://github.com/includeos/mana/blob/master/include/mana/response.hpp):
```cpp
res->cookie(Cookie{"lang", "nb-NO"});

// Or if you want to create a cookie with more options, f.ex. expires, path and domain:
res->cookie(Cookie{"lang", "nb-NO", {"Expires", "Sun, 11 Dec 2016 08:49:37 GMT",
  "Path", "/path", "Domain", "domain.com"}});
```

**Update** an existing cookie's value:
```cpp
res->update_cookie<Cookie>("lang", "en-US");

// Or if you have specified a path and/or domain when creating the cookie:
res->update_cookie<Cookie>("lang", "/path", "domain.com", "en-US");
```

**Clear** a cookie:
```cpp
res->clear_cookie<Cookie>("lang");

// Or if you have specified a path and/or domain when creating the cookie:
res->clear_cookie<Cookie>("lang", "/path", "domain.com");
```

The cookie library contains the middleware [CookieParser](https://github.com/includeos/cookie/blob/master/cookie_parser.hpp) that parses a request's cookie header and puts the cookies in a [CookieJar](https://github.com/includeos/cookie/blob/master/cookie_jar.hpp). The CookieJar is then added as an [attribute](https://github.com/includeos/mana/blob/master/include/mana/attribute.hpp) to the request, making the cookies available to the developer:

```cpp
if (req->has_attribute<CookieJar>()) {
  // Get the CookieJar
  auto req_cookies = req->get_attribute<CookieJar>();
  
  { // Print all the request-cookies (name-value pairs)
    const auto& all_cookies = req_cookies->get_cookies();
    for (const auto& c : all_cookies)
      printf("Cookie: %s=%s\n", c.first.c_str(), c.second.c_str());
  }

  // Get the value of a cookie named lang
  const auto& value = req_cookies->cookie_value("lang");
  
  // Do actions based on the value
}
```

## Requirements
* [IncludeOS](https://github.com/hioa-cs/IncludeOS) installed (together with its dependencies)
* [Mana](https://github.com/includeos/mana)
* git
