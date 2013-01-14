#!/usr/bin/env python
#
#  Copyright (c) 2010 Corey Goldberg (corey@goldb.org)
#  License: GNU LGPLv3
#
#  This file is part of Multi-Mechanize


import time

import dbglog

dbglog.dbg.logMask("AI2W3E3F3")
dbglog.dbg.logFile("debug.log")

import mcache

class Transaction(object):
    def __init__(self):
        self.custom_timers = {}
        servers = ["email-central-mem1.go.seznam:11211", "email-central-mem2.go.seznam:11211", "email-central-mem1.ng.seznam:11211", "email-central-mem2.ng.seznam:11211"]
        opts = {"connect_timeout": 3000, "read_timeout": 3000, "write_timeout": 3000, "restoration_interval": 20}
        self.client = mcache.Client(servers, opts)
        self.idx = 1;

    def run(self):
        start_timer = time.time()

        self.client.set("lubo-test", self.idx, {"expiration": 300})

        self.custom_timers['set'] = time.time() - start_timer

        start_timer = time.time()

        got = self.client.get("lubo-test")["data"]

        assert (got == self.idx), 'Bad value got value ' + got

        self.custom_timers['get'] = time.time() - start_timer

        self.idx += 1;

        start_timer = time.time()

        if self.client.get("lubo-test-tohle-by-tam-urcite-nemelo-bejt"): raise RuntimeError("Bad get got value")

        self.custom_timers['bad-get'] = time.time() - start_timer


if __name__ == '__main__':
    trans = Transaction()
    trans.run()
    print trans.custom_timers
