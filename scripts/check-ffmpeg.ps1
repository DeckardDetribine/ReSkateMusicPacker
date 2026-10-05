#!/usr/bin/env pwsh
# SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
# SPDX-License-Identifier: GPL-3.0-only
# Fails when the pinned ffmpeg download no longer resolves or its bytes changed, so link rot is caught
# before a release. The URL and hash are read from src/ffmpeg_fetch.cpp so there is one source of truth.
$ErrorActionPreference = 'Stop'
$source = Get-Content -Raw (Join-Path $PSScriptRoot '..\src\ffmpeg_fetch.cpp')
$urlBlock = [regex]::Match($source, 'default_url\[\]\s*=\s*(.*?);', [System.Text.RegularExpressions.RegexOptions]::Singleline).Groups[1].Value
$url = (([regex]::Matches($urlBlock, '"([^"]*)"') | ForEach-Object { $_.Groups[1].Value }) -join '')
$sha = [regex]::Match($source, 'default_sha256\[\]\s*=\s*"([0-9a-fA-F]+)"').Groups[1].Value
if (-not $url -or -not $sha) { Write-Error 'Could not read the pinned ffmpeg URL/hash from src/ffmpeg_fetch.cpp.'; exit 1 }
Write-Host "Checking $url"
$temp = Join-Path ([IO.Path]::GetTempPath()) ('ffmpeg-link-check-' + [guid]::NewGuid().ToString('N') + '.zip')
try {
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $url -OutFile $temp -UseBasicParsing -MaximumRedirection 5
    $actual = (Get-FileHash $temp -Algorithm SHA256).Hash.ToLower()
    if ($actual -ne $sha.ToLower()) { Write-Error "SHA-256 mismatch: expected $sha, got $actual"; exit 1 }
    Write-Host "OK: SHA-256 matches ($actual)."
} finally {
    Remove-Item $temp -Force -ErrorAction SilentlyContinue
}
