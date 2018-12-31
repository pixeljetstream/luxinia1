svrport = 14285
svraddr = "localhost"

function server (logger,closeit,broadcastmsg)

  logger = logger or function () end
  closeit = closeit or function () end
  broadcastmsg = broadcastmsg or function () end	

	logger("Starting server")
  local udp,err = socket.udp()
  if not udp then 
  	logger("Error starting server:\n"..err)
  end
  local suc,err = udp:setsockname("*",svrport)
  if not suc then 
  	logger("Error starting server:\n"..err)
  	return
  end
  udp:settimeout(0)
  
  local sender = udp --socket.udp()
  
  
  local function readall ()
    local list = {}
    while true do 
      local data,from,port = udp:receivefrom()
      if data == nil and from == "timeout" then return list end
      print(data)
      assert(data,from) -- another error was thrown!
      table.insert(list,{data = data, from = from, port = port})
    end
  end
  
  local clientlist = {}
  local clientids = {}
  local clientidcnt = 1
  
  local function broadcast (msg)
    -- just send a message to everyone
    logger(msg)
    for i,client in pairs(clientlist) do
      assert(sender:sendto(msg,client.from,client.port))
    end
  end
  
  local function received (from,port,data)
    local clientid = clientids[from..port]
    if not clientid then -- unkown client
      if data == "hello" then -- let it "connect"
        clientid,clientidcnt = clientidcnt,clientidcnt + 1
        local client = {
          id = clientid, 
          from = from, 
          port = port
        }
        clientlist[client.id] = client
        clientids[from..port] = client.id
        assert(sender:sendto("connected "..client.id,from,port))
        broadcast("chat New client connected ("..client.id..")")
      end
      return -- in any case, just terminate her now 
    end
    local client = clientlist[clientid]
    
    if data == "disconnect" then
      clientlist[clientid] = nil -- delete it from the list
      assert(sender:sendto("disconnected",from,port))
      broadcast("chat Client "..client.id.." disconnected")
    end
    
    if data:match "^chat" then
      broadcast("chat ("..client.id.."): "..(data:sub(5)))
    end
  end
  
  logger ("Server initialized, listening on port "..svrport)
  logger ("Waiting for clients")
  while not closeit() do
    local msglist = readall()
    for i,msg in ipairs(msglist) do 
      received(msg.from,msg.port,msg.data)
    end
    local msg = broadcastmsg()
    if msg then broadcast(msg) end
    coroutine.yield() -- sleep
  end
  
  for i,client in pairs(clientlist) do
    sender:sendto("disconnect",client.from,client.port)
  end
  sender:close()
  udp:close()
end


----------------------

function client(svraddr,receiver,sender,closeit)
	svraddr = socket.dns.toip(svraddr)
  receiver "starting client"
  local udp,myid
  
  receiver = receiver or function () end
  sender = sender or function () end
  closeit = closeit or function () end
  
  local function connect ()
    udp = assert(socket.udp())
    udp:settimeout(0)
    --receiver(udp:getsockname())
    
    --assert(udp:setpeername(svraddr,svrport))
    print(svraddr)
    udp:sendto("hello",svraddr,svrport)
    while true do
      coroutine.yield(50)
      local answer,err,port = udp:receivefrom()
      if not answer then
        if err ~= "timeout" then
          -- something went wrong ...
          udp:close()
          return connect() -- let's try again
        end
      else
        if answer:match "^connected" then 
          myid = answer:match("^connected (.*)")
          return 
        end
      end
    end
  end
  
  local function receive ()    
    while true do 
      local data,err = udp:receivefrom()
      if not data and err=="timeout" then 
        return 
      else
        assert(data,err)
        return data
      end
    end
  end
  
  local function say (str)
    udp:sendto("chat "..str,svraddr,svrport)
  end
  
  connect() 
  receiver("Connected and got id "..myid)
  
  while not closeit() do
    local msg = receive()
    if msg then print(msg) end
    if msg and msg:match "^chat" then -- chatmessage
    	local who,tx = msg:match("^chat %((%d+)%)%s*:%s*(.*)")

    	--print(msg,who,tx)
    	if who then
      	receiver("\v950%i:%s%s",tonumber(who),(who==myid and "\v555" or "\v000"), tx)
      else
      	receiver("\v009%s",msg:match("chat%s*(.*)"))
      end
    end
    if msg == "disconnect" then print "dis" break end
    local msg = sender()
    if msg then say (msg) end
    coroutine.yield(50)
  end
  --udp:setpeername("*") -- set no peer, otherwise the peer
  udp:sendto("disconnect",svraddr,svrport) 
  while udp:receive() do end -- empty any queue that might exist
  udp:close() -- is closed too...
end