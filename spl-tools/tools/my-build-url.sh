#!/bin/bash
# A script for the lazy ;) print out the CI URL for the build of your branch so you can just click it and
# don't have to look for it. You can run this from within any git repo directory, not just sspl-tools
# e.g from everest-base dir etc. you can run ../tools/my-build-url.sh
repo=$(basename $(git rev-parse --show-toplevel))
branch=$(git rev-parse --abbrev-ref HEAD)
encoded=$(python3 -c "import sys, urllib.parse as ul; print (ul.quote(sys.argv[1], safe=''))" $branch)
echo https://esg-ci.eng.sophos/job/UnifiedPipelines/job/linuxep/job/${repo}/job/${encoded}/
