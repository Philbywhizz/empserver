#!/usr/bin/perl
#
#  Empire - A multi-player, client/server Internet based war game.
#  Copyright (C) 1986-2007, Dave Pare, Jeff Bailey, Thomas Ruschak,
#                           Ken Stevens, Steve McClure
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  ---
#
#  See files README, COPYING and CREDITS in the root of the source
#  tree for related information and legal notices.  It is expected
#  that future projects/authors will amend these files as needed.
#
#  ---
#
#  mksubj.pl: Create the index for a subject
#
#  Known contributors to this file:
#     Ken Stevens (when it was still info.pl)
#     Markus Armbruster, 2006
#

# Usage: mksubj.pl OUTFILE INFILE...
# The INFILE... contain all the topics belonging to a subject.  Read
# and check the information required for the index from them, write
# the index to OUTFILE.

use strict;
use warnings;

# The chapters, in order
our @Chapters = qw/Introduction Concept Command Server/;

our $filename;
my (%subject, %level, %desc);
my $largest = "";

my $out = shift @ARGV;
$out =~ /([^\/]*)\.t$/
    or die "Strange subject file name $out";
my $subj = $1;

for (@ARGV) {
    my ($topic, $chap, $lvl, $desc) = parse_file($_);
    $largest = $topic if length $topic > length $largest;
    $subject{$chap} .= "$topic\n";
    $level{$topic} = $lvl;
    $desc{$topic} = $desc;
}

open(SUBJ, ">$out")
    or die "Can't open $out for writing: $!";

print SUBJ '.\" DO NOT EDIT THIS FILE.  It was automatically generated by mksubj.pl'."\n";
print SUBJ ".TH Subject \U$subj\n";
$largest =~ s/-/M/g;
print SUBJ ".in \\w'$largest", "XX\\0\\0\\0\\0'u\n";

for my $chap (@Chapters) {
    next unless exists $subject{$chap};
    print SUBJ ".s1\n";
    for (split(/\n/, $subject{$chap})) {
	print SUBJ ".L \"$_ ";
	if ($level{$_} eq 'Basic') {
	    print SUBJ "* \"\n";
	} else {
	    print SUBJ "  \"\n";
	}
	print SUBJ "$desc{$_}\n";
    }
}

print SUBJ <<EOF;
.s1
.in 0
For info on a particular subject, type "info <subject>" where <subject> is
one of the subjects listed above.  Subjects marked by * are the most
important and should be read by new players.
EOF
close SUBJ;


# Check .TH, .NA, .LV and .SA.
# Parse .NA into %desc and .SA into %see_also
sub parse_file {
    ($filename) = @_;
    my ($topic, $chap, $lvl, $desc);

    $topic = $filename;
    $topic =~ s,.*/([^/]*)\.t$,$1,;

    open(F, "<$filename")
	or die "Can't open $filename: $!";

    $_ = <F>;
    if (/^\.TH (\S+) (\S.+\S)$/) {
	if (!grep(/^$1$/, @Chapters)) {
	    error("First argument to .TH was '$1', which is not a known chapter");
	}
	$chap = $1;
	if ($1 eq "Command" && $2 ne "\U$topic") {
	    error("Second argument to .TH was '$2' but it should be '\U$topic'");
	}
    } else {
	error("The first line in the file must be a .TH request");
    }

    $_ = <F>;
    if (/^\.NA (\S+) "(\S.+\S)"$/) {
	if ($topic ne $1) {
	    error("First argument to .NA was '$1' but it should be '$topic'");
	}
	$desc = $2;
    } else {
	error("The second line in the file must be a .NA request");
    }

    $_ = <F>;
    if (/^\.LV (\S+)$/) {
	if ($1 ne 'Basic' && $1 ne 'Expert') {
	    error("The argument to .LV was '$1' but it must be either 'Basic' or 'Expert'");
	}
	$lvl = $1;
    } else {
	error("The third line in the file must be a .LV request");
    }

    close F;

    return ($topic, $chap, $lvl, $desc);
}

# Print an integrity error message and exit with code 1
sub error {
    my ($error) = @_;

    print STDERR "mksubj.pl:$filename:$.: $error\n";
    exit 1;
}
