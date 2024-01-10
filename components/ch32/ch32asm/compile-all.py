#!/usr/bin/env python3

import sys, os, subprocess, pathlib

if len(sys.argv) > 3:
    print("Usage: compile-all.py [indir] [outdir]")
    exit(1)
indir  = "."            if len(sys.argv) < 2 else sys.argv[1]
outdir = "../generated" if len(sys.argv) < 3 else sys.argv[2]

compiler = str(pathlib.Path(__file__).parent.resolve()) + "/compile.py"

for file in os.listdir(indir):
    if file.endswith(".S") or file.endswith(".asm"):
        subprocess.run([compiler, "c", indir + "/" + file, outdir + "/" + file + ".c"])
