# $Id: gen-dep-helper.awk,v 1.1 2005/04/12 17:43:25 nomenquis Exp $
#
# $Log:  $
#

/:/ {
    split($0,temp,":")
    print objectdir"/"temp[1]" "depfilename" : "temp[2]
    next
    }
// {print $0}
