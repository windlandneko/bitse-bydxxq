#!/usr/bin/env bash
# Source this file. Standard apt/Qt SDK installs require no Qt overrides.
# Load tools installed by setup_env.sh without replacing existing global binaries.
for charging_tool_bin in \
  "$HOME/.local/opt/charging-tools/node-v24.20.0/bin" \
  "$HOME/.local/opt/charging-tools/pnpm/bin" \
  "$HOME/.local/opt/charging-tools/uv/bin"; do
  if [[ -d "$charging_tool_bin" && ":$PATH:" != *":$charging_tool_bin:"* ]]; then
    export PATH="$charging_tool_bin:$PATH"
  fi
done
unset charging_tool_bin

charging_qt_prefix="${CHARGING_QT_PREFIX:-$HOME/.local/opt/charging-qt/usr}"
charging_qt_triplet="$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)"
charging_qt_lib_dir="$charging_qt_prefix/lib/${charging_qt_triplet:-x86_64-linux-gnu}"
if [[ ! -d "$charging_qt_lib_dir" && -d "$charging_qt_prefix/lib" ]]; then
  charging_qt_lib_dir="$charging_qt_prefix/lib"
fi
charging_qt_qml_dir="$charging_qt_lib_dir/qt6/qml"
[[ -d "$charging_qt_qml_dir" ]] || charging_qt_qml_dir="$charging_qt_prefix/qml"
if [[ -d "$charging_qt_qml_dir" ]]; then
  export CHARGING_QT_PREFIX="$charging_qt_prefix"
  export CHARGING_QT_LIB_DIR="$charging_qt_lib_dir"
  export CHARGING_QT_QML_DIR="$charging_qt_qml_dir"
  if [[ ":${LD_LIBRARY_PATH:-}:" != *":$charging_qt_lib_dir:"* ]]; then
    export LD_LIBRARY_PATH="$charging_qt_lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  fi
  if [[ ":${QML2_IMPORT_PATH:-}:" != *":$charging_qt_qml_dir:"* ]]; then
    export QML2_IMPORT_PATH="$charging_qt_qml_dir${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
  fi
  if [[ ":${QML_IMPORT_PATH:-}:" != *":$charging_qt_qml_dir:"* ]]; then
    export QML_IMPORT_PATH="$charging_qt_qml_dir${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}"
  fi
  if [[ -x "$charging_qt_prefix/lib/qt6/libexec/QtWebEngineProcess" ]]; then
    export QTWEBENGINEPROCESS_PATH="${QTWEBENGINEPROCESS_PATH:-$charging_qt_prefix/lib/qt6/libexec/QtWebEngineProcess}"
  fi
  # Qt 6.2 reads resource/translation paths from qt.conf; build.sh writes it
  # beside executables and the local WebEngine helper. Do not disable sandboxing.
fi
unset charging_qt_prefix charging_qt_triplet charging_qt_lib_dir charging_qt_qml_dir
