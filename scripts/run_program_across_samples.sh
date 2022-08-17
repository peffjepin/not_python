#!/bin/sh

for filename in $2/*; do
    echo ""
    echo file: $filename
    echo =====================================================================
    cat $filename
    echo ---------------------------------------------------------------------
    $1 $filename
    echo ""
done
