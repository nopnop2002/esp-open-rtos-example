# update-spiffs

Example which renews a file on SPIFFS via a network.   

This program broadcast my IP-Address using UDP (port:8200)   
You can use this simple udp receiver, writen in python:   

	import select, socket

	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	s.bind(('<broadcast>', 8200))
	s.setblocking(0)

	while True:
	    result = select.select([s],[],[])
	    msg = result[0][0].recv(1024)
	    print msg.strip()

An IP address of ESP8266 may never be same, but you can know an IP address of ESP8266.   

![spiffs-1](https://user-images.githubusercontent.com/6020549/52171728-4c051780-27a5-11e9-81ad-b5adda456ca1.jpg)

---

This program receive any command using TCP (port:23)   
This is very easy using nc.   

cat show | nc ESP8266's-IP 23   
show current file content   
There is initial files in files directory.   

cat info | nc ESP8266's-IP 23   
show SPIFFS Information   

cat dir | nc ESP8266's-IP 23   
show Directory Information   

cat stop | nc ESP8266's-IP 23   
stop Broadcast service   

cat start | nc ESP8266's-IP 23   
start Broadcast service

![spiffs-2](https://user-images.githubusercontent.com/6020549/52171730-4f000800-27a5-11e9-93a1-01326ac757a2.jpg)

---


This program update file content using TCP (port:8100)   
You can update current file content using nc(NetCat)   

![spiffs-3](https://user-images.githubusercontent.com/6020549/52171734-57584300-27a5-11e9-8ffc-34b14c7de3f5.jpg)

Update done.   

![spiffs-4](https://user-images.githubusercontent.com/6020549/52171747-9be3de80-27a5-11e9-82f6-3026ba108298.jpg)
