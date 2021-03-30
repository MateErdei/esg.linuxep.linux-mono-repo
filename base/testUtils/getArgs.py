#!/usr/bin/python

args = open("robotArgs").read()

final_args = ""
x = 0
y = 0
while True:
    x = args.find('"', y)
    if x == -1:
        final_args += args[y:]
        break
    final_args += args[y:x]
    y = args.find('"', x+1) + 1
    quoted_text = args [x:y]
    final_args += quoted_text.replace(' ', '_').replace('"', '')

print(final_args.strip("\n"))
