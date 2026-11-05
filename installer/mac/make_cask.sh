#!/bin/bash
# Generates Casks/peaky.rb for your Homebrew tap repo (github.com/<you>/homebrew-peaky).
# Usage: installer/mac/make_cask.sh <version> <github_user> <path_to_Peaky-macOS-plugins.zip>
set -euo pipefail
VERSION="$1"; OWNER="$2"; ZIP="$3"
SHA="$(shasum -a 256 "$ZIP" | cut -d' ' -f1)"
mkdir -p Casks
cat > Casks/peaky.rb <<RUBY
cask "peaky" do
  version "$VERSION"
  sha256 "$SHA"

  url "https://github.com/$OWNER/Peaky/releases/download/v#{version}/Peaky-macOS-plugins.zip"
  name "Peaky"
  desc "Automatic pre-compression vocal peak smoother by Aron H"
  homepage "https://www.instagram.com/aronh.official"

  vst3_plugin "Peaky.vst3"
  audio_unit_plugin "Peaky.component"

  # not notarized: strip quarantine so DAWs can load it
  postflight do
    %w[Peaky.vst3 Peaky.component].each do |b|
      system_command "/usr/bin/xattr", args: ["-cr", "#{staged_path}/#{b}"]
    end
  end

  zap trash: [
    "~/Documents/Aron H/Peaky",
  ]
end
RUBY
echo "Wrote Casks/peaky.rb  (sha256 $SHA)"
