#!/usr/bin/python
'''
sprite2xml.py
A quickly thrown together conversion script for StepMania .sprite files to .xml.
Usage: in windows at least, just drag a .sprite file onto the .py
- shakesoda
'''
import string
import sys

def split_line(input):
	line = string.split(input,"=")
	if len(line) > 1:
		return line
	else: # will be false on blank/comment lines and sections
		return False

def main(arg):
	# read the .sprite
	file = open(arg, 'r')
	output = "<Sprite\n"
	for line in file:
		value = split_line(line)
		if value != False:
			output += "\t" + value[0] + "=\"" + value[1][0:-1] + "\"\n"
	output += "/>"
	file.close()
	# write out the .xml
	file = open(arg[0:-7] + ".xml", 'w')
	file.write(output)
	file.close()

main(sys.argv[1])