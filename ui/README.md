
# Dimmer UI

The get this to run at startup on the pi, I added this to `/etc/rc.local`:
```
su -l pi -c "screen -dm bash -c /home/pi/dimmer/ui/startup.sh"
```
The intent is to run the startup script, in a detached screen session, as user _pi_.  It seems awful, but it runs correctly from on boot, which is the main aim of the game.


---

At this stage, it's best to ignore most of what's below.

## One-time setup
### IP ports
To allow connection to the web ui via port 80, without needing to run with elevated privileges, use this bit of IP tables magic:
```
sudo iptables -t nat -I PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 8080
```
...okay, this isn't one-time!
To check: `iptables -t nat --list --line-numbers`  (`--line-numbers` needed if planning to delete a rule)
To remove: `iptables -t nat -D PREROUTING 1`  (`1` is the line number from above)
Better: switch `-I` for `-D`

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
