#!/usr/bin/env python
# -*- coding: utf-8 -*-

import mcache

client = mcache.Client(["localhost:11211", ])
client.set("szn", "www.seznam.cz")
print("set")
if client.add("szn", "aaa"):
    print("EEEEEEEEEEEEEEEE1")
print("add")
if not client.prepend("szn", "http://"):
    print("EEEEEEEEEEEEEEEE2")
print("prepend")
if not client.append("szn", "/"):
    print("EEEEEEEEEEEEEEEE3")
print("append")
data = client.get("szn")
print(data)
print(data["data"])
print(type(data["data"]))
print(client.get("szna"))

#client.set("x", long(3))
#print(client.get("x"))
#print(type(client.get("x")["data"]))

client.set("x", 3.3)
print(client.get("x"))
print(type(client.get("x")["data"]))

client.set("szn", "www.seznam.cz", {"flags": 3})
print(client.get("szn"))

client.set("x", True)
print(client.get("x"))
print(type(client.get("x")["data"]))

client.set("x", 3)
print(client.incr("x", 3))
print(client.decr("x", 3))

client.set("x", {"three": 3})
print(client.get("x"))
print(type(client.get("x")["data"]))

