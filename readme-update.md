# norns update procedure

## update information and bundling

upon entering the `SYSTEM > UPDATE` menu the update folder `~/update/` is scanned for files
matching the `norns*.tgz` pattern. any attached USB disk has its root folder checked as well.
if present the following process happens:

- file is extracted
- md5 check
- if success, extract data to folder
- cleanup (delete archives and md5)

updates are stored as folders.\*

when an update is "executed" via the menu, the application runs `update.sh`
within the selected update's folder and then updates `version.txt` which is
also stored in `~/update`.

\* there is no reason to keep the update folders, they should be deleted after being run. also, an
update's version number should be checked against the current version, to prevent backwards
updating (which would likely only break a bunch of functionality).

## anatomy of an update .tgz

update versions tracked by date YYMMDD.

ie `norns180401.tgz`

inside this archive is:

`180401.tgz`
`180401.md5`

the md5 validates the tgz. if passed, `180401.tgz` is extracted to `180401/`.

inside `180401/` is `update.sh`, `version.txt`, and the payload.

`update.sh` copies included files to the correct locations (and does whatever
else needs to be done).

`version.txt` simply contains the version (ie `180401`) and is copied to `~/update`
(which is later read by matron).

files relevant to matron, crone, or maiden should be kept in respective
subfolders in the archive.

## preparation of an update

generating an update is straightforward. make a YYMMDD folder, put all the
files in there with a correct `update.sh`.

```
tar czvf YYMMDD.tgz YYMMDD
md5sum YYMMDD.tgz > YYMMDD.md5
tar czvf nornsYYMMDD.tgz YYMMDD.*
```

## delivery

presently update files are copied to `~/update` via the `SYSTEM > UPDATE` menu, which checks the remote github release against the current version.

it's possible to manually install an update by copying the update file to `~/update`, extracting it, and running `update.sh` directly.
