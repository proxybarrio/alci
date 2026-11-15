#!/usr/bin/env perl
#Get the version from the kernel makefile

while (<>) {
        /^\s*VERSION\s*=\s*(\d+)/       && ($ver = $1);
        /^\s*PATCHLEVEL\s*=\s*(\d+)/    && ($pat = $1);
        /^\s*SUBLEVEL\s*=\s*(\d+)/      && ($sub = $1);
        /^\s*PRE\s*=\s*(.*)/            && ($pre = $1);
}

print "\nALCI 0.1: Asymmetric Linux Compact Initiative\n";
print "based on ELKS ${ver}.${pat}.${sub}${pre} by Alan Cox, Greg Haerr and the ELKS team\n\n";
