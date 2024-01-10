# wsproxy.c
A reasonably fast implementation of wsproxy in C, with a custom frame decoder. For when you want to use TCP in the browser

## How to compile
```
git clone https://github.com/MercuryWorkshop/wsproxy.c
cd wsproxy.c
make
./wsproxy
```
will start listening on port 8081

## How to use

```js
new WebSocket("ws://localhost:8081/domain.tld:port")
```
Binary events sent and recieved from the websocket will be converted to the tcp stream


This provides no protection against multicast ips. Be careful!
