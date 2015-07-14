#!/usr/bin/env bash

# in the case a machine has already been provisioned, running this script shouldn't be a problem.

# quick and dirty test to see if update has been run yet.
if [ ! -d /tmp/firsttime ]
  then
    apt-get update
fi

apt-get install -y g++
apt-get install -y make
apt-get install -y libtool
apt-get install -y autoconf
apt-get install -y automake
apt-get install -y uuid-dev
apt-get install -y git
apt-get install -y unzip
apt-get install -y libcurl-openssl-dev
apt-get install -y libcurl4-openssl-dev
apt-get install -y wget
apt-get install -y screen
apt-get install -y libc6-dev-i386
apt-get install -y curl
apt-get install -y dos2unix
apt-get install -y build-essential
apt-get install -y libpcre3-dev
apt-get install -y zlib1g-dev
apt-get install -y emacs

if [ ! -d /tmp/firsttime ]
  then
  	mkdir /tmp/firsttime
fi
