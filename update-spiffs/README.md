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

An IP address of ESP8266 may never be same.   
You can know an IP address of ESP8266.   

![spiffs-1](https://user-images.githubusercontent.com/6020549/52171728-4c051780-27a5-11e9-81ad-b5adda456ca1.jpg)

---

This program receive any command using TCP (port:23)   

nc show ESP8266's-IP 23   
 show current file content   
 There is initial files in files directory.   
nc info ESP8266's-IP 23   
 show SPIFFS Information   
nc dir ESP8266's-IP 23   
 show Directory Information   
nc stop ESP8266's-IP 23   
 stop Broadcast service   
nc start ESP8266's-IP 23
 start Broadcast service   

![spiffs-2](https://user-images.githubusercontent.com/6020549/52172428-2fbba780-27b2-11e9-9e3d-3b467585ed78.jpg)

![spiffs-5](https://user-images.githubusercontent.com/6020549/52172426-2fbba780-27b2-11e9-8d48-6a65f43ee3ca.jpg)

---


This program update file content using TCP (port:8100)   
You can update current file content using nc(NetCat)   

  $ cat new.json | nc ESP8266's-IP 8100

![spiffs-3](https://user-images.githubusercontent.com/6020549/52172429-2fbba780-27b2-11e9-98d9-238df8c48367.jpg)

Update done.   

![spiffs-4](https://user-images.githubusercontent.com/6020549/52172430-2fbba780-27b2-11e9-85b0-8139135ee6b5.jpg)
