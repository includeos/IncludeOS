# mana
IncludeOS C++ Web Application Framework

[Acorn](../../examples/acorn) is a web server built with Mana which demonstrates a lot of its potential.

Some insight in the implementation of mana can be found in [this post](http://blog.includeos.org/2016/10/05/middleware-implementation-in-mana).

## Usage

It's easy to get started - check out the [examples](examples/).

```cpp
using namespace mana;
using namespace std::string_literals;

std::unique_ptr<Server> server;

void Service::start(const std::string&)
{
  Router router;

  // GET /
  router.on_get("/", [](auto, auto res) {
    res->add_body("<html><body><h1>Simple example</h1></body></html>"s);
    res->send();
  });

  server = std::make_unique<Server>(net::Inet4::stack());
  server->set_routes(router).listen(80);
}
```

### Routes

Routes is where the server end-points are defined.

```cpp
Router router;

// GET /
router.on_get("/", [] (auto req, auto res) {
  // Serve index.html
});

// POST /users
router.on_post("/users", [] (auto req, auto res) {
  // Register new user
});

server.set_routes(router);
```

There is also support for named parameters in routes.

```cpp
// GET /users/:id
router.on_get("/users/:id(\\d+)", [](auto req, auto res) {
  auto id = req->params().get("id");
  // Do actions according to "id"
  if(id == "42")
    // ...
});
```

### Middleware

Middleware are tasks which are executed before the user code defined in routes.

```cpp
// Declare a new middleware
class MyMiddleware : public mana::Middleware {
  // ...
};

// Add a middleware object
Middleware_ptr my_mw = std::make_shared<MyMiddleware>();
server.use(my_mw);
```

It's also possible to just add a simple task with a lambda.

```cpp
// Add a middleware lambda
server.use([] (auto req, auto res) {
  // My custom middleware function
  (*next)(); // Don't forget to call next if no response was sent!
});
```

*Psst, there is already some [ready-made middleware](include/mana/middleware) for Mana!*


### Attributes

Attributes is a way to extend the Request object with additional data.

```cpp
// Declare a new attribute
class MyAttribute : public Attribute {
  // ...
};

// Set attribute in middleware
MyMiddleware::process(auto req, auto res, auto next) {
  auto attr = std::make_shared<MyAttribute>();
  req->set_attribute(attr);
  (*next)();
}

// Use attribute in route
router.use("/my-route", [] (auto req, auto res) {
  if(auto attr = req->get_attribute<MyAttribute>()) {
    // Do things with "attr"
  }
});
```

