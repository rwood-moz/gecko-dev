#!/bin/bash -ve

############################### tc-repo-cache.sh ###############################

### If specified repository exists on the S3 repo cache, pull it down and untar

# Usage: tc-repo-cache repo-cache.json <repository> <target>

repo_cache() {
  local registry=$1
  local repository=$2
  local target=$3

  if [ ! -f "$registry" ]; then
    echo "$registry doesn't exist"
    exit 1
  fi

  cache_url=$(cat $registry | jq ."[\""$repository\""]")

  if [ "$cache_url" == "null" ]; then
    echo "$repository not found in $registry"
    echo "Repo not found in the cache, clone instead"
    exit 1
  fi
  cache_url=${cache_url:1:${#cache_url}-2}
  cache_file=$(basename $cache_url)

  echo "Downloading $cache_url"
  if curl --retry 5 --output /dev/null --silent --head --fail "$cache_url"; then
    if [ ! -d "$target" ]; then
      mkdir -p "$target"
    fi
    curl --retry 5 "$cache_url" > "$target/$cache_file"
    if [ -f "$target/$cache_file" ]; then
      echo "Decompressing"
      tar -x -z -f "$target/$cache_file" --strip-components=1 -C $target
      rm $target/$cache_file
    else
      echo "Download failed"
      exit 1
    fi
  else
    echo "Cache URL $cache_url doesn't exist"
    exit 1
  fi
}

if [ "$#" != 3 ]; then
  echo "Usage: tc-repo-cache repo-cache.json <repository> <target>"
  exit 1
fi

repo_cache $1 $2 $3

############################### tc-repo-cache.sh ###############################
