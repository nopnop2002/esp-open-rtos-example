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

This program send current file content using TCP (port:23)   
You can comfirm current file content using nc(NetCat)   
There is initial files in files directory.   

![spiffs-2](https://user-images.githubusercontent.com/6020549/52171730-4f000800-27a5-11e9-93a1-01326ac757a2.jpg)

---


This program update file content using TCP (port:8100)   
You can update current file content using nc(NetCat)   

  cat new.json | nc ESP8266's-IP 8100   

![spiffs-3](https://user-images.githubusercontent.com/6020549/52171734-57584300-27a5-11e9-8ffc-34b14c7de3f5.jpg)

Update done.   

![spiffs-4](https://user-images.githubusercontent.com/6020549/52171747-9be3de80-27a5-11e9-82f6-3026ba108298.jpg)
