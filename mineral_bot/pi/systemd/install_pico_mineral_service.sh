#!/usr/bin/env bash
set -euo pipefail

SERVICE_NAME="pico-mineral.service"
DEFAULT_USER="pi"
DEFAULT_GROUP="pi"
DEFAULT_REPO="/home/pi/IEEE_R5"
DEFAULT_PORT="/dev/serial0"
DEFAULT_BAUD="115200"
DEFAULT_SPEED="170"
DEFAULT_THRESHOLD="22.0"

USER_NAME="$DEFAULT_USER"
GROUP_NAME="$DEFAULT_GROUP"
REPO_DIR="$DEFAULT_REPO"
PORT="$DEFAULT_PORT"
BAUDRATE="$DEFAULT_BAUD"
SPEED="$DEFAULT_SPEED"
THRESHOLD="$DEFAULT_THRESHOLD"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --user)
      USER_NAME="$2"
      shift 2
      ;;
    --group)
      GROUP_NAME="$2"
      shift 2
      ;;
    --repo)
      REPO_DIR="$2"
      shift 2
      ;;
    --port)
      PORT="$2"
      shift 2
      ;;
    --baudrate)
      BAUDRATE="$2"
      shift 2
      ;;
    --speed)
      SPEED="$2"
      shift 2
      ;;
    --threshold)
      THRESHOLD="$2"
      shift 2
      ;;
    -h|--help)
      cat <<'EOF'
Install and enable systemd service for pico_mineral_controller.py

Usage:
  ./install_pico_mineral_service.sh [options]

Options:
  --user <name>         Service user (default: pi)
  --group <name>        Service group (default: pi)
  --repo <path>         Repo root path (default: /home/pi/IEEE_R5)
  --port <device>       Serial port (default: /dev/serial0)
  --baudrate <int>      Baudrate (default: 115200)
  --speed <0-255>       Speed (default: 170)
  --threshold <float>   Obstacle threshold cm (default: 22.0)
  -h, --help            Show this help
EOF
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

SCRIPT_PATH="$REPO_DIR/raspberry_pi/pico_mineral_controller.py"
SERVICE_PATH="/etc/systemd/system/$SERVICE_NAME"

if [[ ! -f "$SCRIPT_PATH" ]]; then
  echo "Controller script not found: $SCRIPT_PATH" >&2
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is not installed" >&2
  exit 1
fi

echo "Installing $SERVICE_NAME at $SERVICE_PATH"
sudo tee "$SERVICE_PATH" >/dev/null <<EOF
[Unit]
Description=Pico Mineral Collector Controller
After=network.target

[Service]
Type=simple
User=$USER_NAME
Group=$GROUP_NAME
WorkingDirectory=$REPO_DIR
ExecStart=/usr/bin/python3 $SCRIPT_PATH --port $PORT --baudrate $BAUDRATE --speed $SPEED --threshold $THRESHOLD
Restart=always
RestartSec=2
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now "$SERVICE_NAME"

echo "Service installed and started."
echo "Status: sudo systemctl status $SERVICE_NAME --no-pager"
echo "Logs:   sudo journalctl -u $SERVICE_NAME -f"
