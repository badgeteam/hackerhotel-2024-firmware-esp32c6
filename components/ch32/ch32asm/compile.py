#!/usr/bin/env python3

import sys, os, subprocess, tempfile, re

if len(sys.argv) < 4:
    print("Usage: compile.py [elf|bin|c] [infile...] [outfile]")
    exit(1)

mode = sys.argv[1].lower().strip()
if mode not in ["elf", "bin", "c"]:
    print("Invalid output type")
    exit(1)

# Find any RISC-V compiler.
def find_compiler():
    for dir in os.getenv("PATH").split(":"):
        for file in os.listdir(dir):
            if file.startswith("riscv32-") and file.endswith("-gcc"):
                return file
compiler = find_compiler()
prefix   = compiler[:-4]

# Create a linkerscript as a temporary file.
linkerscript="""
OUTPUT_ARCH( "riscv" )
OUTPUT_FORMAT( "elf32-littleriscv" )
ENTRY(_start)

PHDRS {
    codeseg   PT_LOAD;
    rodataseg PT_LOAD;
    dataseg   PT_LOAD;
}

SECTIONS {
    /DISCARD/ : { *(.note.gnu.build-id) }
    
    . = 0x00000000;
    .text : { *(.text) } :codeseg
    .rodata : { *(.rodata) } :rodataseg
    .data : { *(.data) } :dataseg
    .bss : { *(.bss) } :NONE
}
"""
linkerfile = tempfile.NamedTemporaryFile("w+", suffix=".ld")
linkerfile.write(linkerscript)
linkerfile.flush()

# Compile it to an ELF file.
if mode == "elf":
    cc_out_file = None
    cc_out = sys.argv[-1]
else:
    cc_out_file = tempfile.NamedTemporaryFile("w+b", suffix=".elf")
    cc_out = cc_out_file.name
cc_args = [
    compiler, "-T"+linkerfile.name, "-march=rv32ec", "-mabi=ilp32e", "-nostdlib", "-nodefaultlibs", "-nostartfiles"
] + sys.argv[2:-1] + [
    "-o", cc_out
]
for x in cc_args: print(x + " ", end="")
print()
res = subprocess.run(cc_args)
if res.returncode:
    exit(res.returncode)
if mode == "elf": exit(0)

# Put it into a binary file.
if mode == "bin":
    bin_out_file = None
    bin_out = sys.argv[-1]
else:
    bin_out_file = tempfile.NamedTemporaryFile("w+b", suffix=".elf")
    bin_out = bin_out_file.name
bin_args = [
    prefix + "-objcopy", "-O", "binary", cc_out, bin_out
]
for x in bin_args: print(x + " ", end="")
print()
res = subprocess.run(bin_args)
if res.returncode:
    exit(res.returncode)
if mode == "bin": exit(0)

# Put it into a C file.
infd  = open(bin_out, "rb")
outfd = open(sys.argv[-1], "w")
if m := re.match("^(?:.+?/)?([^/]+)(?:\.[ch])?$", sys.argv[-1]):
    name = ""
    for c in m.group(1):
        if not re.match("^\w$", c):
            name += "_"
        else:
            name += c
else:
    name = "name"

data = infd.read()

outfd.write("// WARNING: This is a generated file, do not edit it!\n")
outfd.write("// clang-format off\n")
outfd.write("#include <stdint.h>\n")
outfd.write("#include <stddef.h>\n")
outfd.write("uint8_t const {}[] = {{\n".format(name))
for byte in data:
    outfd.write("0x{:02x},".format(byte))
outfd.write("\n};\n")
outfd.write("size_t const {}_len = {};\n".format(name, len(data)))

infd.close()
outfd.flush()
outfd.close()