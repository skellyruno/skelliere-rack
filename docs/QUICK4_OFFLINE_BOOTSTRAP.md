# Offline Quick4 dependency bootstrap

This isolated infrastructure branch exports the official JUCE tag 8.0.12 and matching Debian 13 development packages through a network-enabled Actions runner. It does not change plugin source, active PR, production builds, or DNS settings. Artifacts are intended for authorized local development. The build machine must already have a C++ compiler, CMake, Ninja and Xvfb.

1. Download the Quick4-JUCE-8.0.12-Source artifact. Verify SHA256SUMS.txt and extract JUCE-8.0.12.tar.gz. Pass its extracted absolute path as JUCE_PATH.
2. Download the Debian kit. Verify its outer hash, extract, then check inner SHA256SUMS.txt and architecture. Inspect package versions against the local Debian 13 runtime. Do not blindly replace existing glibc, compiler or other system packages. Use matching development packages with an offline apt install (no-download) on a disposable compatible machine, or a selectively prepared sysroot. If runtime versions do not match, obtain the matching versions before installing. Do not mix Ubuntu headers into Debian's core system include path.
3. Configure the actual rack source with cmake -S . -B build-quick4 -G Ninja -DCMAKE_BUILD_TYPE=Release -DJUCE_PATH=/absolute/path/JUCE-8.0.12 -DSKELLIERE_BUILD_UI_RENDERER=ON -DFETCHCONTENT_FULLY_DISCONNECTED=ON. Build SkelliereRackUIRenderer and execute under xvfb-run with --output <fresh-folder>.
4. Inspect all four genuine captures and manifest. This bundle does not prove the local native build until compilation and capture pass on the target runtime.

Do not modify system DNS, hardcode GitHub IPs or disable TLS. Use existing local source caches in future sessions. Source is archived with upstream commit identity and checksums; retain the JUCE license. The Windows release build remains an independent validation.
