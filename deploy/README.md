# SmartLife Primary Deployment

Target host layout:

- Project: `/home/ubuntu/smartlife-primary`
- Dashboard: `127.0.0.1:19267`
- Relay: `127.0.0.1:19266`
- MQTT: `127.0.0.1:19283`
- Public URL: `https://hongkongxiao.ilelezhan.cn/`

Runtime secrets are server-only:

1. Copy `primary.env.template` to `.env` and replace the MQTT credentials.
2. Create `mosquitto.passwd` with the same MQTT username and password.
3. Set both files to mode `600`.

Install the three unit templates under `/etc/systemd/system/`, then run `systemctl daemon-reload`. Start and verify the new services before enabling the Nginx site. Always run `nginx -t` before a reload.

Do not modify or stop the `smartlife-junior-*` services.
