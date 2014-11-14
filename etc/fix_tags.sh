
# Fixing tag named '1.0.1'
git checkout $1               # Go to the associated commit
git tag -d $1                 # Locally delete the tag
git push origin :refs/tags/$1 # Push this deletion up to GitHub

# Create the tag, with a date derived from the current head
GIT_COMMITTER_DATE="$(git show --format=%aD | head -1)" git tag -a $1 

git push --tags                  # Send the fixed tags to GitHub
