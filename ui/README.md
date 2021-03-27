
# Dimmer UI

## One-time setup
### IP ports
To allow connection to the web ui via port 80, without needing to run with elevated privileges, use this bit of IP tables magic:
```
sudo iptables -t nat -I PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 8080

```

### Python virtual environment
```
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

## Running the UI

```
source venv/bin/activate
./web.py
```
