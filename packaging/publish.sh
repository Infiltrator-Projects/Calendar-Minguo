#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Publish only the three assets qualified and byte-verified by this commit.
set -euo pipefail
contract=release/contract.json
[[ -f $contract ]] || { echo 'Qualification contract missing' >&2; exit 1; }
commit=$(jq -er .commit "$contract")
version=$(jq -er .version "$contract")
name=$(jq -er .name "$contract")
[[ $commit == "$GITHUB_SHA" ]] || { echo 'Release commit mismatch' >&2; exit 1; }
[[ $(jq '.assets | length' "$contract") -eq 3 ]] || exit 1
mapfile -t assets < <(jq -r '.assets | keys[]' "$contract")
for asset in "${assets[@]}"; do
  [[ $asset == "$(basename "$asset")" && -f release/$asset ]] || exit 1
  expected=$(jq -er --arg key "$asset" '.assets[$key]' "$contract")
  actual=$(sha256sum "release/$asset" | cut -d' ' -f1)
  [[ $actual == "$expected" ]] || { echo "Artifact checksum mismatch: $asset" >&2; exit 1; }
done
[[ $(printf '%s\n' "${assets[@]}" | grep -Ec '\.deb$') -eq 1 ]] || exit 1
[[ $(printf '%s\n' "${assets[@]}" | grep -Ec '\.run$') -eq 1 ]] || exit 1
[[ $(printf '%s\n' "${assets[@]}" | grep -Ec '\.zip$') -eq 1 ]] || exit 1

head=$(gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" --jq .object.sha)
if [[ $head != "$commit" ]]; then
  echo 'Qualified commit superseded; publication skipped.'
  exit 0
fi
tag="v$version"
err=$(mktemp)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp" "$err"' EXIT
if gh api "repos/$GITHUB_REPOSITORY/git/ref/tags/$tag" > "$tmp/tag.json" 2> "$err"; then
  [[ $(jq -er .object.sha "$tmp/tag.json") == "$commit" &&
     $(jq -er .object.type "$tmp/tag.json") == commit ]] ||
     { echo 'Existing release tag points to another commit; advance VERSION' >&2; exit 1; }
elif ! grep -Eq 'HTTP 404|Not Found' "$err"; then
  cat "$err" >&2
  exit 1
fi

state=missing
if gh release view "$tag" --json isDraft > "$tmp/release.json" 2> "$err"; then
  if [[ $(jq -r .isDraft "$tmp/release.json") == true ]]; then
    state=draft
  else
    state=published
  fi
elif ! grep -Eqi 'HTTP 404|release not found' "$err"; then
  cat "$err" >&2
  exit 1
fi

paths=()
for asset in "${assets[@]}"; do paths+=("release/$asset"); done
if [[ $state == missing ]]; then
  gh release create "$tag" "${paths[@]}" --target "$commit" --draft \
    --title "$name $version" \
    --notes 'Portable amd64 Debian package, native installer and complete source ZIP. Each package installs this provider only.'
elif [[ $state == draft ]]; then
  gh release upload "$tag" "${paths[@]}" --clobber
fi
verify() {
  local remote file
  remote=$(gh release view "$tag" --json assets | jq -r '.assets[].name' | LC_ALL=C sort)
  [[ $remote == "$(printf '%s\n' "${assets[@]}" | LC_ALL=C sort)" ]] || {
    echo 'Published release asset set differs from qualification' >&2; return 1;
  }
  rm -rf "$tmp/download"
  mkdir -p "$tmp/download"
  gh release download "$tag" --dir "$tmp/download"
  for file in "${assets[@]}"; do
    [[ $(sha256sum "$tmp/download/$file" | cut -d' ' -f1) ==
       $(jq -er --arg key "$file" '.assets[$key]' "$contract") ]] ||
       { echo "Published asset differs: $file" >&2; return 1; }
  done
}
verify
if [[ $state != published ]]; then gh release edit "$tag" --draft=false --latest; fi
verify
echo "Published and byte-verified $tag with exactly three assets."
