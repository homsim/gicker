# gicker

## Ideas

* `git reflog` can be used to see when a branch has been checked out
  * Need to read into this further
* Then, this information can be compared with the data from `docker inpect <IMAGE>` to see which branch the image is based on
  * Could also give the possibility to show last commit

## Improvements

* I have no idea what the content of the metadata is when you try to use this on an image that is pulled e.g. from dockerhub. I guess I need some checks for that, too.