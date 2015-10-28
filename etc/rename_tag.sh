# Rename tag $1 to $2'
git tag $2 $1              # Go to the associated commit
git tag -d $1                 # Locally delete the tag
git push origin :refs/tags/$1   # Push this deletion up to GitHub
git push --tags                  # Send the fixed tags to GitHub
