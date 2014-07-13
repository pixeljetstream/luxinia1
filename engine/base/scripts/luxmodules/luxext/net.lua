socket = require "socket"
ftp = require("socket.ftp")
http = require("socket.http")
ltn12 = require("ltn12")

LuxModule.registerclass("lux_ext","http",
[[HTTP (Hyper Text Transfer Protocol) is the protocol used to exchange 
information between web-browsers and servers. The http namespace offers 
full support for the client side of the HTTP protocol (i.e., the facilities 
that would be used by a web-browser implementation). The implementation 
conforms to the HTTP/1.1 standard, RFC 2616.

The module exports functions that provide HTTP functionality in different 
levels of abstraction. From the simple string oriented requests, through 
generic LTN12 based, down to even lower-level if you bother to look through 
the source code.

URLs must conform to RFC 1738, that is, an URL is a string in the form:

 [http://][<user>[:<password>]@]<host>[:<port>][/<path>] 

MIME headers are represented as a Lua table in the form:

 headers = {
   field-1-name = field-1-value,
   field-2-name = field-2-value,
   field-3-name = field-3-value,
   ...
   field-n-name = field-n-value
 }

Field names are case insensitive (as specified by the standard) and all 
functions work with lowercase field names. Field values are left unmodified.

Note: MIME headers are independent of order. Therefore, there is no problem 
in representing them in a Lua table.

The following constants can be set to control the default behavior of the 
HTTP module:

* PORT: default port used for connections;
* PROXY: default proxy used for connections;
* TIMEOUT: sets the timeout for all I/O operations;
* USERAGENT: default user agent reported to server. 

 http.request(url [, body])
 http.request{
  url = string,
  [sink = LTN12 sink,]
  [method = string,]
  [headers = header-table,]
  [source = LTN12 source],
  [step = LTN12 pump step,]
  [proxy = string,]
  [redirect = boolean,]
  [create = function]
 }

The request function has two forms. The simple form downloads a URL using the 
GET or POST method and is based on strings. The generic form performs any HTTP 
method and is LTN12 based.

If the first argument of the request function is a string, it should be an url. 
In that case, if a body is provided as a string, the function will perform a 
POST method in the url. Otherwise, it performs a GET in the url

If the first argument is instead a table, the most important fields are the 
url and the simple LTN12 sink that will receive the downloaded content. Any 
part of the url can be overridden by including the appropriate field in the 
request table. If authentication information is provided, the function uses 
the Basic Authentication Scheme (see note) to retrieve the document. If sink 
is nil, the function discards the downloaded data. The optional parameters 
are the following:

* method: The HTTP request method. Defaults to "GET";
* headers: Any additional HTTP headers to send with the request;
* source: simple LTN12 source to provide the request body. If there is a body, 
you need to provide an appropriate "content-length" request header field, or 
the function will attempt to send the body as "chunked" (something few servers 
support). Defaults to the empty source;
* step: LTN12 pump step function used to move data. Defaults to the LTN12 
pump.step function.
* proxy: The URL of a proxy server to use. Defaults to no proxy;
* redirect: Set to false to prevent the function from automatically following 
301 or 302 server redirect messages;
* create: An optional function to be used instead of socket.tcp when the 
communications socket is created. 

In case of failure, the function returns nil followed by an error message. 
If successful, the simple form returns the response body as a string, followed 
by the response status code, the response headers and the response status line. 
The complex function returns the same information, except the first return value 
is just the number 1 (the body goes to the sink).

Even when the server fails to provide the contents of the requested URL (URL not 
found, for example), it usually returns a message body (a web page informing the 
URL was not found or some other useless page). To make sure the operation was 
successful, check the returned status code. For a list of the possible values 
and their meanings, refer to RFC 2616.

Here are a few examples with the simple interface:

 -- connect to server "www.cs.princeton.edu" and retrieves this manual
 -- file from "~diego/professional/luasocket/http.html" and print it to stdout
 http.request{ 
    url = "http://www.cs.princeton.edu/~diego/professional/luasocket/http.html", 
    sink = ltn12.sink.file(io.stdout)
 }

 -- connect to server "www.example.com" and tries to retrieve
 -- "/private/index.html". Fails because authentication is needed.
 b, c, h = http.request("http://www.example.com/private/index.html")
 -- b returns some useless page telling about the denied access, 
 -- h returns authentication information
 -- and c returns with value 401 (Authentication Required)
 
 -- tries to connect to server "wrong.host" to retrieve "/"
 -- and fails because the host does not exist.
 r, e = http.request("http://wrong.host/")
 -- r is nil, and e returns with value "host not found"

And here is an example using the generic interface:

 -- load the http module
 http = require("socket.http")

 -- Requests information about a document, without downloading it.
 -- Useful, for example, if you want to display a download gauge and need
 -- to know the size of the document in advance
 r, c, h = http.request {
  method = "HEAD",
  url = "http://www.tecgraf.puc-rio.br/~diego"
 }
 -- r is 1, c is 200, and h would return the following headers:
 -- h = {
 --   date = "Tue, 18 Sep 2001 20:42:21 GMT",
 --   server = "Apache/1.3.12 (Unix)  (Red Hat/Linux)",
 --   ["last-modified"] = "Wed, 05 Sep 2001 06:11:20 GMT",
 --   ["content-length"] = 15652,
 --   ["connection"] = "close",
 --   ["content-Type"] = "text/html"
 -- }

Note: Some URLs are protected by their servers from anonymous download. For 
those URLs, the server must receive some sort of authentication along with the 
request or it will deny download and return status "401 Authentication Required".

The HTTP/1.1 standard defines two authentication methods: the Basic 
Authentication Scheme and the Digest Authentication Scheme, both explained 
in detail in RFC 2068.

The Basic Authentication Scheme sends <user> and <password> unencrypted to the 
server and is therefore considered unsafe. Unfortunately, by the time of this 
implementation, the wide majority of servers and browsers support the Basic 
Scheme only. Therefore, this is the method used by the toolkit whenever 
authentication is required.

 -- load required modules
 http = require("socket.http")
 mime = require("mime")

 -- Connect to server "www.example.com" and tries to retrieve
 -- "/private/index.html", using the provided name and password to
 -- authenticate the request
 b, c, h = http.request("http://fulano:silva@www.example.com/private/index.html")

 -- Alternatively, one could fill the appropriate header and authenticate
 -- the request directly.
 r, c = http.request {
  url = "http://www.example.com/private/index.html",
  headers = { authentication = "Basic " .. (mime.b64("fulano:silva")) }
 }]],
 http,
{}
)



LuxModule.registerclass("lux_ext","ftp",
[[
FTP (File Transfer Protocol) is a protocol used to transfer files between hosts. 
The ftp namespace offers thorough support to FTP, under a simple interface. The 
implementation conforms to RFC 959.

High level functions are provided supporting the most common operations. These 
high level functions are implemented on top of a lower level interface. Using 
the low-level interface, users can easily create their own functions to access 
any operation supported by the FTP protocol. For that, check the implementation.

To really benefit from this module, a good understanding of LTN012, Filters 
sources and sinks is necessary.

URLs MUST conform to RFC 1738, that is, an URL is a string in the form:

    [ftp://][<user>[:<password>]@]<host>[:<port>][/<path>][type=a|i] 

The following constants in the namespace can be set to control the 
default behavior of the FTP module:

* PASSWORD: default anonymous password.
* PORT: default port used for the control connection;
* TIMEOUT: sets the timeout for all I/O operations;
* USER: default anonymous user; 

 ftp.get(url)
 ftp.get{
  host = string,
  sink = LTN12 sink,
  argument or path = string,
  [user = string,]
  [password = string]
  [command = string,]
  [port = number,]
  [type = string,]
  [step = LTN12 pump step,]
  [create = function]
 }

The get function has two forms. The simple form has fixed functionality: it 
downloads the contents of a URL and returns it as a string. The generic form 
allows a lot more control, as explained below.

If the argument of the get function is a table, the function expects at least 
the fields host, sink, and one of argument or path (argument takes precedence). 
Host is the server to connect to. Sink is the simple LTN12 sink that will 
receive the downloaded data. Argument or path give the target path to the 
resource in the server. The optional arguments are the following:

* user, password: User name and password used for authentication. 
Defaults to "ftp:anonymous@anonymous.org";
* command: The FTP command used to obtain data. Defaults to "retr", but see example below;
* port: The port to used for the control connection. Defaults to 21;
* type: The transfer mode. Can take values "i" or "a". Defaults to 
whatever is the server default;
* step: LTN12 pump step function used to pass data from the server to 
the sink. Defaults to the LTN12 pump.step function;
* create: An optional function to be used instead of socket.tcp when 
the communications socket is created. 

If successful, the simple version returns the URL contents as a string, and 
the generic function returns 1. In case of error, both functions return nil 
and an error message describing the error.

 -- Log as user "anonymous" on server "ftp.tecgraf.puc-rio.br",
 -- and get file "lua.tar.gz" from directory "pub/lua" as binary.
 f, e = ftp.get("ftp://ftp.tecgraf.puc-rio.br/pub/lua/lua.tar.gz;type=i")

 -- load needed modules
 local ftp = require("socket.ftp")
 local ltn12 = require("ltn12")
 local url = require("socket.url")

 -- a function that returns a directory listing
 function nlst(u)
    local t = {}
    local p = url.parse(u)
    p.command = "nlst"
    p.sink = ltn12.sink.table(t)
    local r, e = ftp.get(p)
    return r and table.concat(t), e
 end

 ftp.put(url, content)
 ftp.put{
  host = string,
  source = LTN12 sink,
  argument or path = string,
  [user = string,]
  [password = string]
  [command = string,]
  [port = number,]
  [type = string,]
  [step = LTN12 pump step,]
  [create = function]
 }

The put function has two forms. The simple form has fixed functionality: 
it uploads a string of content into a URL. The generic form allows a lot more 
control, as explained below.

If the argument of the put function is a table, the function expects at least 
the fields host, source, and one of argument or path (argument takes precedence). 
Host is the server to connect to. Source is the simple LTN12 source that will 
provide the contents to be uploaded. Argument or path give the target path to 
the resource in the server. The optional arguments are the following:

* user, password: User name and password used for authentication. 
Defaults to "ftp:anonymous@anonymous.org";
* command: The FTP command used to send data. Defaults to "stor", but see example below;
* port: The port to used for the control connection. Defaults to 21;
* type: The transfer mode. Can take values "i" or "a". Defaults to 
whatever is the server default;
* step: LTN12 pump step function used to pass data from the server to the sink. 
Defaults to the LTN12 pump.step function;
* create: An optional function to be used instead of socket.tcp when the 
communications socket is created. 

Both functions return 1 if successful, or nil and an error message 
describing the reason for failure.

 -- Log as user "fulano" on server "ftp.example.com",
 -- using password "silva", and store a file "README" with contents 
 -- "wrong password, of course"
 f, e = ftp.put("ftp://fulano:silva@ftp.example.com/README", 
    "wrong password, of course")

 -- load the ftp support
 local ftp = require("socket.ftp")
 local ltn12 = require("ltn12")

 -- Log as user "fulano" on server "ftp.example.com",
 -- using password "silva", and append to the remote file "LOG", sending the
 -- contents of the local file "LOCAL-LOG"
 f, e = ftp.put{
  host = "ftp.example.com", 
  user = "fulano",
  password = "silva",
  command = "appe",
  argument = "LOG",
  source = ltn12.source.file(io.open("LOCAL-LOG", "r"))
 }
]],
ftp,
{}
)

LuxModule.registerclass("lux_ext","socket.DNS",
[[
Name resolution functions return all information obtained from the resolver in 
a table of the form:

    resolved = {
      name = canonic-name,
      alias = alias-list,
      ip = ip-address-list
    } 

Note that the alias list can be empty. 
]],
socket.DNS,
{
gethostname = "(?):() - Returns the standard host name for the machine as a string. ",
tohostname = [[(?):(address) -
Converts from IP address to host name.

Address can be an IP address or host name.

The function returns a string with the canonic host name of the given address, 
followed by a table with all information returned by the resolver. In case of 
error, the function returns nil followed by an error message.]],
toip = [[(?):(address) -
Converts from host name to IP address.

Address can be an IP address or host name.

Returns a string with the first IP address found for address, followed by a 
table with all information returned by the resolver. In case of error, the 
function returns nil followed by an error message. ]]
}
)

LuxModule.registerclass("lux_ext","socket",
[[
The socket namespace contains the core functionality of LuaSocket.
]],
socket,
{
--------------------------------------------------------------------------------
bind = [[(?):(address, port [, backlog]) - 
This function is a shortcut that creates and returns a TCP server 
object bound to a local address and port, ready to accept client 
connections. Optionally, user can also specify the backlog argument to the 
listen method (defaults to 32).

Note: The server object returned will have the option "reuseaddr" set to true.]],

--------------------------------------------------------------------------------
connect = [[(?):(address, port [, locaddr, locport]) - 
This function is a shortcut that creates and returns a TCP client object connected 
to a remote host at a given port. Optionally, the user can also specify the 
local address and port to bind (locaddr and locport). ]],


_DEBUG = [[boolean - This constant is set to true if the library was 
compiled with debug support.]],

--------------------------------------------------------------------------------
newtry = [[(?):(finalizer) -
Creates and returns a clean try function that allows for cleanup before 
the exception is raised.

Finalizer is a function that will be called before try throws the exception. 
It will be called in protected mode.

The function returns your customized try function.

Note: This idea saved a lot of work with the implementation of protocols in LuaSocket:

 foo = socket.protect(function()
    -- connect somewhere
    local c = socket.try(socket.connect("somewhere", 42))
    -- create a try function that closes 'c' on error
    local try = socket.newtry(function() c:close() end)
    -- do everything reassured c will be closed 
    try(c:send("hello there?\r\n"))
    local answer = try(c:receive())
    ...
    try(c:send("good bye\r\n"))
    c:close()
 end)
]],

--------------------------------------------------------------------------------
protect = [[():(func) -
Converts a function that throws exceptions into a safe function. This function 
only catches exceptions thrown by the try and newtry functions. It does not 
catch normal Lua errors.

Func is a function that calls try (or assert, or error) to throw exceptions.

Returns an equivalent function that instead of throwing exceptions, returns nil 
followed by an error message.

Note: Beware that if your function performs some illegal operation that raises 
an error, the protected function will catch the error and return it as a string. 
This is because the try function uses errors as the mechanism to throw exceptions. ]],

--------------------------------------------------------------------------------
select = [[(?):(recvt, sendt [, timeout]) - 
Waits for a number of sockets to change status.

Recvt is an array with the sockets to test for characters available for reading. 
Sockets in the sendt array are watched to see if it is OK to immediately write 
on them. Timeout is the maximum amount of time (in seconds) to wait for a change 
in status. A nil, negative or omitted timeout value allows the function to block 
indefinitely. Recvt and sendt can also be empty tables or nil. Non-socket values 
(or values with non-numeric indices) in the arrays will be silently ignored.

The function returns a table with the sockets ready for reading, a table with the 
sockets ready for writing and an error message. The error message is "timeout" 
if a timeout condition was met and nil otherwise. The returned tables are 
associative, to simplify the test if a specific socket has changed status.

Important note: a known bug in WinSock causes select to fail on non-blocking 
TCP sockets. The function may return a socket as writable even though the socket 
is not ready for sending.

Another important note: calling select with a server socket in the receive 
parameter before a call to accept does not guarantee accept will return 
immediately. Use the settimeout method or accept might block forever. ]],

--------------------------------------------------------------------------------
sink = [[(?):(mode, socket) -
Creates an LTN12 sink from a stream socket object.

Mode defines the behavior of the sink. The following options are available:

* "http-chunked": sends data through socket after applying the chunked transfer 
coding, closing the socket when done;
* "close-when-done": sends all received data through the socket, closing the 
socket when done;
* "keep-open": sends all received data through the socket, leaving it open 
when done. 

Socket is the stream socket object used to send the data.

The function returns a sink with the appropriate behavior. ]],

--------------------------------------------------------------------------------
skip = [[(?):(d [, ret1, ret2 ... retN])
Drops a number of arguments and returns the remaining.

D is the number of arguments to drop. Ret1 to retN are the arguments.

The function returns retd+1 to retN.

Note: This function is useful to avoid creation of dummy variables:

 -- get the status code and separator from SMTP server reply 
 local code, sep = socket.skip(2, string.find(line, "^(%d%d%d)(.?)"))]],

--------------------------------------------------------------------------------
sleep = [[(?):(time) -
Freezes the program execution during a given amount of time.

Time is the number of seconds to sleep for. The function truncates time down to 
the nearest integer. ]],
--------------------------------------------------------------------------------
source = [[(?):(mode, socket [, length]) -
Creates an LTN12 source from a stream socket object.

Mode defines the behavior of the source. The following options are available:

* "http-chunked": receives data from socket and removes the chunked transfer 
coding before returning the data;
* "by-length": receives a fixed number of bytes from the socket. This mode 
requires the extra argument length;
* "until-closed": receives data from a socket until the other side closes 
the connection. 

Socket is the stream socket object used to receive the data.

The function returns a source with the appropriate behavior. ]],
--------------------------------------------------------------------------------
gettime = [[(?):() -
Returns the time in seconds, relative to the origin of the universe. 
You should subtract the values returned by this function to get meaningful values.

 t = socket.gettime()
 -- do stuff
 print(socket.gettime() - t .. " seconds elapsed")]],
--------------------------------------------------------------------------------
try = [[(?):(ret1 [, ret2 ... retN]) -
Throws an exception in case of error. The exception can only be caught by 
the protect function. It does not explode into an error message.

Ret1 to retN can be arbitrary arguments, but are usually the return values 
of a function call nested with try.

The function returns ret1 to retN if ret1 is not nil. Otherwise, it calls 
error passing ret2.

 -- connects or throws an exception with the appropriate error message
 c = socket.try(socket.connect("localhost", 80))]],
--------------------------------------------------------------------------------
_VERSION = [[string -
This constant has a string describing the current LuaSocket version. ]]
}
)
