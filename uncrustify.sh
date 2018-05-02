#!/bin/bash
# run this from top-level norns directory before committing...
find ws-wrapper/src -regex .*[\.][ch] | xargs uncrustify -c uncrustify.cfg --no-backup
find matron/src -regex .*[\.][ch] | xargs uncrustify -c uncrustify.cfg --no-backup
