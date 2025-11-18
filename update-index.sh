#!/bin/bash
set -e

# Generate PEP 503 simple package index from GitHub releases

mkdir -p simple/pycpl

# Root index
cat > simple/index.html <<EOF
<!DOCTYPE html>
<html>
<head>
    <title>Simple Index</title>
</head>
<body>
    <a href="pycpl/">pycpl</a><br>
</body>
</html>
EOF

# Package index header
cat > simple/pycpl/index.html <<EOF
<!DOCTYPE html>
<html>
<head>
    <title>Links for pycpl</title>
</head>
<body>
    <h1>Links for pycpl</h1>
EOF

# Add links for all releases
gh release list --limit 100 --json tagName | \
  jq -r '.[].tagName' | \
  while read tag; do
    gh release view "$tag" --json assets --jq '.assets[] | select(.name | endswith(".whl") or endswith(".tar.gz")) | .name' | \
      while read filename; do
        echo "    <a href=\"https://github.com/ivh/pycpl/releases/download/$tag/$filename\">$filename</a><br>" >> simple/pycpl/index.html
      done
  done

# Footer
cat >> simple/pycpl/index.html <<EOF
</body>
</html>
EOF

echo "Index updated in simple/"
