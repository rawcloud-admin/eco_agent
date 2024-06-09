#!/bin/bash


systemctl disable eco_agent.service
systemctl stop eco_agent.service
rm  /etc/systemd/system/eco_agent.service
systemctl daemon-reload

rm -rf /root/eco_agent



