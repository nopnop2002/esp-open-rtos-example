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

**echo view | nc ESP8266's-IP 23**   
view current file content   
There is initial files in files directory.   

**echo parse | nc ESP8266's-IP 23**   
parse current file content   

**echo info | nc ESP8266's-IP 23**   
show SPIFFS Information   

**echo dir | nc ESP8266's-IP 23**   
show Directory Information   

**echo stop | nc ESP8266's-IP 23**   
stop Broadcast service   

**echo start | nc ESP8266's-IP 23**   
start Broadcast service

![spiffs-11](https://user-images.githubusercontent.com/6020549/52176511-49331280-27f7-11e9-84fd-ab58948bb822.jpg)

![spiffs-12](https://user-images.githubusercontent.com/6020549/52176513-4df7c680-27f7-11e9-85af-f994ed04960c.jpg)

![spiffs-41](https://user-images.githubusercontent.com/6020549/52176526-62d45a00-27f7-11e9-9ff9-9b605372047b.jpg)

---


This program update file content using TCP (port:8100)   
You can update current file content using nc.   

**cat new.json | nc ESP8266's-IP 8100**   

![spiffs-21](https://user-images.githubusercontent.com/6020549/52176534-71227600-27f7-11e9-9a33-af788b03a003.jpg)

Update done.   

![spiffs-31](https://user-images.githubusercontent.com/6020549/52176541-7da6ce80-27f7-11e9-8cb8-f142b4c089de.jpg)

![spiffs-32](https://user-images.githubusercontent.com/6020549/52176544-826b8280-27f7-11e9-8f70-a8a78d0691d8.jpg)

