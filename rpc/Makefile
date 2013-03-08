CC=gcc
FLAGS=-pedantic -Wall
RPC=rpcserver.c
CLI=rpcclient.c
DFLAGS=-g -DDEBUG
LSFLAGS=-L/includes -lxmlrpc_server_abyss -lxmlrpc_server -lxmlrpc_abyss -lpthread -lxmlrpc -lxmlrpc_util -lxmlrpc_xmlparse -lxmlrpc_xmltok lowres.o lowres-write.o libjpeg.a
LCFLAGS=-L/usr/local/lib -lcurl -lgssapi_krb5 -lxmlrpc_client -lxmlrpc -lxmlrpc_util -lxmlrpc_xmlparse -lxmlrpc_xmltok

all: rpcserver

debug: debugserver

rpcserver:
	$(CC) -o rpcserver $(RPC) $(LSFLAGS) $(FLAGS)

debugserver:
	$(CC) -o rpcserver $(RPC) $(LSFLAGS) $(FLAGS) $(DFLAGS)

clean:
	rm rpcserver
