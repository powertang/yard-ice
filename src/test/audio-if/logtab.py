#!/usr/bin/python2

from math import *

def main():

	print "#include <stdint.h>"
	print "#include \"fixpt.h\""
	print ""
	print "/* Q15 dB to gain lookup table */"
	print "const uint16_t q15_db2pwr_ltu[] = {"

	db = 0.0
	d = 10000
	i = 0
	while d > 1:
		x = 10 ** (db / 10.0)
		d = round(x * 32768)
		print "\t%6d, /* %d dB */" % (d, db)
		db = db - 1
		i = i + 1

	print "};"
	print ""

	print "#define Q15_DB2PWR_MIN %d" % (db)
	print "const int8_t q15_db2pwr_min = Q15_DB2PWR_MIN;"
	print ""
	print "const uint16_t q15_db2pwr(int pwr)"
	print "{"
	print "\tif (pwr < Q15_DB2PWR_MIN)"
	print "\t\tpwr = Q15_DB2PWR_MIN;"
	print "\telse if (pwr > 0)"
	print "\t\tpwr = 0;"
	print ""
	print "\treturn q15_db2pwr_ltu[-pwr];"
	print "}"

	print ""
	print "/* Q15 dB to amplitude lookup table */"
	print "const uint16_t q15_db2amp_ltu[] = {"

	db = 0.0
	d = 10000
	i = 0
	while d > 1:
		x = 10 ** (db / 20.0)
		d = round(x * 32768)
		print "\t%6d, /* %d dB */" % (d, db)
		db = db - 1
		i = i + 1

	print "};"
	print ""

	print "#define Q15_DB2AMP_MIN %d" % (db)
	print "const int8_t q15_db2amp_min = Q15_DB2AMP_MIN;"
	print ""
	print "const uint16_t q15_db2amp(int amp)"
	print "{"
	print "\tif (amp < Q15_DB2AMP_MIN)"
	print "\t\tamp = Q15_DB2AMP_MIN;"
	print "\telse if (amp > 0)"
	print "\t\tamp = 0;"
	print ""
	print "\treturn q15_db2amp_ltu[-amp];"
	print "}"

if __name__ == "__main__":
    main()
