#!/usr/bin/env python3

import licant

licant.execute("./mqtt-genos.g.py")

licant.cxx_application("target",
                       sources=["src/main.cpp"],
                       mdepends=["mqtt-genos"],
                       libs=["igris"],
                       )

licant.ex("target")
