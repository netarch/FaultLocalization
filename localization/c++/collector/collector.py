import socket
import time
import threading
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('192.168.100.101', 6000)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(server_address)

#listen for incoming connections
sock.listen(1)

count = 0
def recv_timeout(connection, timeout=0.01):
    connection.setblocking(0)
    total_data = []
    data = ''
    begin = time.time()
    while True:
        curr_time = time.time()
        #if you got some data, then break after wait sec
        if total_data and curr_time - begin > timeout:
            #print("Breaking after", curr_time - begin, "seconds")
            break
        #if you got no data at all, wait a little longer
        elif time.time() - begin > timeout*2:
            #print("Breaking after", curr_time - begin, "seconds")
            break
        try:
            data = connection.recv(1000)
            if data == "":
                #print("BBBBBreaking after", curr_time - begin, "seconds")
                break
            if data:
                total_data.append(data)
                begin = curr_time
            else:
                time.sleep(0.001)
        except:
            pass
    return ''.join(total_data)

def ListenToClient(connection, client_address):
    try:
        #count += 1
        #print >>sys.stderr, 'connection from', client_address
        # Receive the data in small chunks and retransmit it
        #while True:
            #data = connection.recv(400)
        data = recv_timeout(connection)
        if "IPV4_SRC_ADDR" in data:
            print >>sys.stderr, 'received "%s"' % data
        else:
            pass
            #print >>sys.stderr, 'no more data from', client_address
            #break
    finally:
        # Clean up the connection
        connection.close()


while True and count==0:
    # Wait for a connection
    #print >>sys.stderr, 'waiting for a connection'
    connection, client_address = sock.accept()
    connection.settimeout(0.1)
    threading.Thread(target = ListenToClient,args = (connection,client_address)).start()
