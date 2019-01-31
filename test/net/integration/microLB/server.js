var http = require('http');
var url = require('url')

var dataString = function(len) {
  return '#'.repeat(len);
}

function randomData(len) {
  return Array.from({length:len}, () => Math.floor(Math.random() * 40));
}

var stringToColour = function(str) {
  var hash = 0;
  for (var i = 0; i < str.length; i++) {
    hash = str.charCodeAt(i) + ((hash << 5) - hash);
  }
  var colour = '#';
  for (var i = 0; i < 3; i++) {
    var value = (hash >> (i * 8)) & 0xFF;
    colour += ('00' + value.toString(16)).substr(-2);
  }
  return colour;
}

function handleDigest(path, request, response) {
  response.setTimeout(500);
  var addr = request.connection.localPort;
  response.end(addr.toString() + dataString());
}

function handleFile(path,request, response) {
  response.setTimeout(500);
  var addr = request.connection.localPort;
  var size = parseInt(path.replace("/",""),10);

  if (size == 0) { 
    size=1024*64;
  }
  response.end(addr.toString() + dataString(size));
}

function defaultHandler(path,request,response) {
  response.setTimeout(500);
  var addr = request.connection.localPort;
  response.end(addr.toString() + dataString(1024*1024*50));
}

var routes = new Map([
    ['/digest' , handleDigest],
    ['/file' , handleFile]
  ]);

function findHandler(path)
{
  for (const [key,value] of routes.entries()) {
    if (path.startsWith(key))
    {
      return { pattern: key, func: value};
    }
  }
  return { pattern :'',func : defaultHandler};
}

function handleRequest(request, response){
  var parts = url.parse(request.url);

  var route = findHandler(parts.pathname);
  if (route.func)
  {
    var path = parts.pathname.replace(route.pattern,'');
    route.func(path,request,response);
  }
}

http.createServer(handleRequest).listen(6001, '10.0.0.1');
http.createServer(handleRequest).listen(6002, '10.0.0.1');
http.createServer(handleRequest).listen(6003, '10.0.0.1');
http.createServer(handleRequest).listen(6004, '10.0.0.1');
