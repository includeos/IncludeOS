# path_to_regex
Turns a route path (string, string pattern or regular expression) into a regex and populates a vector with the route's parameters. [Mana](https://github.com/includeos/mana) uses this library to match incoming request paths with the routes specified by the developer. This is done by the [Router](https://github.com/includeos/mana/blob/master/include/mana/router.hpp), and the developer has access to the parameters through the request. Path_to_regex is a port of the most essential functionality in the JavaScript library [pillarjs/path-to-regexp](https://github.com/pillarjs/path-to-regexp).

**Example service:** [Acorn Web Server Appliance](https://github.com/includeos/acorn).

## Usage
Specifying a route in service.cpp:
```cpp
Router router;

// GET /users/5, /users/101 and so on
router.on_get("/users/:id(\\d+)", [](auto req, auto res) {
  auto id = req->params().get("id");
  
  // Do actions according to "id"
  if(id == "42")
    // ...
});

server.set_routes(router);
```
Some route path examples:
```cpp
// GET /
router.on_get("/", [](auto req, auto res) {
  res->send(true);
});

// GET /about
router.on_get("/about", [](auto req, auto res) {
  res->send(true);
});

// GET /acd and /abcd
router.on_get("/ab?cd", [](auto req, auto res) {
  res->send(true);
});

// GET /abcd, /abbcd, /abbbcd and so on
router.on_get("/ab+cd", [](auto req, auto res) {
  res->send(true);
});

// GET /abcd, /abxcd, /abRANDOMcd, /ab123cd and so on
router.on_get("/ab*cd", [](auto req, auto res) {
  res->send(true);
});

// GET /abe and /abcde
router.on_get("/ab(cd)?e", [](auto req, auto res) {
  res->send(true);
});

// GET /science-paper, /newspaper and so on, but not /newspapers or /paper f.ex.
router.on_get("/.*paper$/", [](auto req, auto res) {
  res->send(true);
});

// GET /users/jane/books/aeneid, /users/john/books/poetics and so on
router.on_get("/users/:username([a-z]+)/books/:title([a-z]+)", [](auto req, auto res) {
  auto username = req->params().get("username");
  auto title = req->params().get("title");
  // First example: username == "jane", title == "aeneid"
  // Params' get-method throws ParamException if key doesn't exist
  
  // Do actions according to the values
  
  res->send(true);
});

// GET /users/2/books/5, /users/15/books/312 and so on
router.on_get("/users/:userId(\\d+)/books/:bookId(\\d+)", [](auto req, auto res) {
  auto userId = req->params().get("userId");
  auto bookId = req->params().get("bookId");
  // First example: userId == "2", bookId == "5"
  // Params' get-method throws ParamException if key doesn't exist
  
  // Do actions according to the values
  
  res->send(true);
});
```

## Requirements
* [IncludeOS](https://github.com/hioa-cs/IncludeOS) installed (together with its dependencies)
* [Mana](https://github.com/includeos/mana)
* git
