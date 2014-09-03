#!/bin/bash
read -p "$1" yn
case $yn in
	[Yy]* ) exit 0;;
	* ) exit -1;;
esac
