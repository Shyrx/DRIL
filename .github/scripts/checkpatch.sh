#!/bin/sh

echo "###### Checking coding style using Checkpatch #####"

# ../../drivers/gistre/gistre_card/commands/*.(c|h)

FILES_TO_CHECK=$(find ./drivers/gistre/gistre_card -name '*.[ch]')
echo "Checking:
$FILES_TO_CHECK"

CHECKPATCH=$(find ./drivers/gistre/gistre_card -name '*.[ch]' -exec ./resources/checkpatch.pl --no-tree -f {} 2>/dev/null \;)

echo '

############# CHECKING ERRORS #######################

'

ERRORS=$(echo "$CHECKPATCH" | grep  -A 3 "ERROR:")
if [ ! -z "$ERRORS" ]; then
    NB_ERRORS=$(echo "$ERRORS" | grep -o "$ERROR" | wc -l)
    echo "### ERRORS DETECTED ($NB_ERROR found) ###"
    echo "$ERRORS"
    exit 1
else
    echo "No Error found, skipping..."
fi

echo '

############# CHECKING WARNINGS #######################

'

WARNINGS=$(echo "$CHECKPATCH" | grep -A 3 "WARNING:")

if [ ! -z  "$WARNINGS" ]; then
    NB_WARNINGS=$(echo "$WARNINGS" | grep -o "WARNING:" | wc -l)
    echo "### WARNINGS DETECTED ($NB_WARNINGS found) ###
"
    echo "$WARNINGS"
else
    echo "No Warning found, skipping..."
fi

echo "

###### Checkpatch Done #####

"
