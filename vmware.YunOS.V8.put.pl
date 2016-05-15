#!/usr/bin/perl -w

use Cwd;
use File::Find;
use File::Copy;
use File::Path;
use Data::Dump qw(dump);
use strict;

my $curdir = getcwd;
my @arr = (
	'opt/ir.h      ',
	'opt/ir.cpp      ',
	'opt/ir_rp.cpp      ',
	'opt/ir_du.cpp ',
	'opt/ir_du.h ',
	'opt/ir_ssa.cpp     ',
	'opt/ir_ssa.h     ',
	'opt/region.cpp',
	'opt/region.h',
	'opt/ir_refine.cpp',
	'opt/ir_cp.cpp',
	'opt/option.cpp',
	'opt/option.h',
);

foreach (@arr) {
    chomp;
    my $arrelem = $_;
    print "\nFile:", $arrelem, "\n";
    my $cmdline = "pscp -l zhenyu -pw 123 -r $curdir\\$arrelem   zhenyu\@192.168.184.132:/home/zhenyu/v8prj/yunos/v8/src/xoc/$arrelem";
	print $cmdline;
	my $retval = system($cmdline);
	if ($retval != 0) {
		print "Return Value is not 0, but is $retval";
		exit($retval);
	}
}
    
