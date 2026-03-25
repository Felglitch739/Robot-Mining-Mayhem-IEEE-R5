# Pico mineral controller as systemd service

## Files

- `pico-mineral.service`: example unit file with default values.
- `install_pico_mineral_service.sh`: installer that generates and enables a service with your parameters.

## 1) On Raspberry Pi, install dependency

```bash
pip3 install pyserial
```

## 2) Install and start service

From your repo root on the Pi:

```bash
cd ~/IEEE_R5/raspberry_pi/systemd
./install_pico_mineral_service.sh --port /dev/serial0 --speed 170 --threshold 22.0
```

If your Pico is USB serial instead:

```bash
./install_pico_mineral_service.sh --port /dev/ttyUSB0
```

## 3) Useful commands

```bash
sudo systemctl status pico-mineral.service --no-pager
sudo journalctl -u pico-mineral.service -f
sudo systemctl restart pico-mineral.service
sudo systemctl stop pico-mineral.service
```

## 4) Disable/uninstall

```bash
sudo systemctl disable --now pico-mineral.service
sudo rm -f /etc/systemd/system/pico-mineral.service
sudo systemctl daemon-reload
```
