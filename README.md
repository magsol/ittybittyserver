# Itty Bitty Webserver

Lightweight webserver, web proxy, and RPC image compression.

## Overview

This was the third and final iteration of my CS4210 Operating Systems project in the spring of 2008. It consists of a multithreaded webserver, proxy, and RPC image compression node.

WARNING: It's extremely finicky. Use with caution!

## Requirements

Any *nix system should work just fine. You'll need all the POSIX standards, in addition to the Xmlrpc-c library.

http://xmlrpc-c.sourceforge.net

## Installation

Pretty straightforward.

    make [client | server | proxy]

To build the RPC server:

    cd rpc
    make

## Running

Here I'll demonstrate each module of the application and its required arguments, followed by example usages.

Server:

    ./server <port> <threads> [docroot] [-o]
    ./server 3333 10
    ./server 4444 5 someDir -o

Client:

    ./client [-p <proxy> <port> http://]<server> <port> <threads> <doclist>
    ./client localhost 3333 10 documentlist.txt
    ./client -p localhost 4444 http://www.cc.gatech.edu 80 5 docs.txt

Proxy:

    ./proxy <port> <threads> [-c <RPC server> <port>] [-o]
    ./proxy 3333 10 -o
    ./proxy 4444 5 -c localhost 8080

RPC Server:

    ./rpcserver <port>
    ./rpcserver 8080

## Notes

Shared memory works fully in this release; its implementation moved much faster when it was revealed to me that pointers in shared memory need to be stored as relative offsets, rather than absolute addresses.  As in the previous release, no socket communication occurs between client and server if optimizations on both ends are enabled.

The proper functioning of this release depends on the Xmlrpc-c RPC package.  This marshals and unmarshals data within an XML schema and transmits the data as HTTP packets.  The implementation also took care of concurrency issues.  It is an extremely mature application.  The only problem is it has to exist, along with the curl library, on whatever machine acts as an RPC server.  Luckily, every CoC machine has curl, and it is a very simple task of installing the Xmlrpc-c package and having its shared libraries compiled within the home directory.

JPG request and transfer still fails sometimes.  I was unable to ascertain  exactly why.  The proxy occasionally hangs or outright crashes on particular requests to arbitrary web servers.  This may be due to broken pipes.

Also, images in excess of 380K are not transferred correctly.  The reason for this is unknown at this time, but it probably has something to do with the Xmlrpc-c implementation.