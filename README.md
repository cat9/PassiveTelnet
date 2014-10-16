PassiveTelnet
=============

PassiveTelnet is something like telnet,but it is Passive.

The sources contain two part:TelnetPassive and TelnetPipe.

TelnetPassive is run at server Part,it listen on a port ,wait for the client TelnetPipe start a request.

TelnetPipe run at the client that you want to telnet to.it run as a service on daemon.it listen on port 10109

you can use socket send string "ip=192.168.1.101&port=12345" to port 10109,when the telnetPipe receive the string,

it will make a pipe that connected the local telnetd port 23 and request a request connect to the server(ip=192.168.1.101&port=12345)

if the TelnetPassive is online,the the connect is success.

Enjoy it!

Project write with Eclipse.

Step by step:

step 1:server part use the command:
  TelnetPassive port
  as an example: ./TelnetPassive 12345
  
step 2:add TelnetPassive to the boot list of the device you want to telnet to.
  by default,TelnetPassive listening on port 10109
  you can change the port it listening by the source code.
  
step 3:your won application can check the server state at some time,when the server want to use the telnet,

your own app can use socket send string "ip=192.168.1.101&port=12345" to port 10109,the TelnetPassive will connect to the server.


end.
