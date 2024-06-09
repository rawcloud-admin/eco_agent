#!/bin/bash

mkdir /root/eco_agent
mkdir /root/eco_agent/LOG
cp exec /root/eco_agent
cp public.pem /root/eco_agent

cp eco_agent.service  /etc/systemd/system/eco_agent.service
systemctl daemon-reload
systemctl start eco_agent.service
systemctl enable eco_agent.service


