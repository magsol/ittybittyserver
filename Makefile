CC=gcc
FLAGS=-pedantic -Wall -lpthread -lrt
SERVER=server.c
CLIENT=client.c
PROXY=proxy.c
DFLAGS=-g -DDEBUG
PROXYFLAGS=-L/usr/local/lib -lcurl -lgssapi_krb5 -lxmlrpc_client -lxmlrpc -lxmlrpc_util -lxmlrpc_xmlparse -lxmlrpc_xmltok

all: server client proxy

debug: debugserver debugclient debugproxy

server:
	$(CC) -o server $(SERVER) $(FLAGS)

client:
	$(CC) -o client $(CLIENT) $(FLAGS)

proxy:
	$(CC) -o proxy $(PROXY) $(PROXYFLAGS) $(FLAGS)

debugserver:
	$(CC) -o server $(SERVER) $(FLAGS) $(DFLAGS)

debugclient:
	$(CC) -o client $(CLIENT) $(FLAGS) $(DFLAGS)

debugproxy:
	$(CC) -o proxy $(PROXY) $(PROXYFLAGS) $(FLAGS) $(DFLAGS)

spam:
	$(CC) -o spam generateSpam.c $(FLAGS)

clean:
	rm server client proxy spam

#valgrind:
#	valgrind -v --show-reachable=yes ./server
