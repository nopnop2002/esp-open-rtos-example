import select, socket

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('<broadcast>', 8200))
s.setblocking(0)

while True:
    result = select.select([s],[],[])
    msg = result[0][0].recv(1024)
    print msg.strip()

