#!/usr/bin/env bash
set -e

echo "Building glassmon..."

mkdir -p build
cd build
cmake ..
make -j$(nproc)

echo "Installing..."
sudo cp glassmon /usr/local/bin/

echo "Setting up autostart..."
mkdir -p ~/.config/autostart

cat > ~/.config/autostart/glassmon.desktop <<EOF
[Desktop Entry]
Type=Application
Name=glassmon
Exec=/usr/local/bin/glassmon
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
EOF

echo "Done. Run 'glassmon' to start."
