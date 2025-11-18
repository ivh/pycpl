#!/bin/bash
set -e

# Generate PEP 503 simple package index from GitHub releases

mkdir -p simple/pycpl

# Root index
cat > simple/index.html <<'EOF'
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

# Package index - generate complete HTML
{
    cat <<'HEADER'
<!DOCTYPE html>
<html>
<head>
    <title>Links for pycpl</title>
</head>
<body>
    <h1>Links for pycpl</h1>
HEADER

    # Generate links for all releases
    for tag in $(gh release list --limit 100 --json tagName --jq '.[].tagName'); do
        gh release view "$tag" --json assets --jq '.assets[] | select(.name | endswith(".whl") or endswith(".tar.gz")) | .name' | \
            while IFS= read -r filename; do
                echo "    <a href=\"https://github.com/ivh/pycpl/releases/download/$tag/$filename\">$filename</a><br>"
            done
    done

    cat <<'FOOTER'
</body>
</html>
FOOTER
} > simple/pycpl/index.html

echo "Index updated in simple/"
