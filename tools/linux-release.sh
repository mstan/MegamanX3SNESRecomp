# Title metadata and payload requirements for the engine release driver.
APP_NAME="MegaManX3"
RELEASE_SLUG="MegaManX3SNESRecomp"        # matches the windows zip prefix
CMAKE_TARGET="MegaManX3SNESRecomp"
ROM_EXTS="sfc smc"
EXTRA_ARGS=""
REGEN_CMD="bash tools/regen.sh --no-tests"
PREBUILD_CMD=""
POSTBUILD_CMD=""
BOXART="recomp/launcher/boxart.tga"          # AppImage icon source (optional)
EXTRA_PAYLOAD=()                             # repo-relative files -> usr/bin/
# Release-owned mod catalog: packaging fails if the built tree is missing one
# of these rather than shipping an AppImage with an empty Mods page.
REQUIRED_MOD_MANIFESTS=(
  "packages/megaman-x3.enhancement.widescreen/1.0.0/manifest.toml"
)
PROD_CMAKE_FLAGS=( -DSNESRECOMP_ENABLE_TRACE=OFF )
DEBUG_CMAKE_FLAGS=( -DSNESRECOMP_ENABLE_TRACE=ON )

MOD_CATALOG_DIRS=(.)
