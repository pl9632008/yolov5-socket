#!/bin/bash
set -e
echo -e "\e[34m **************Start install AnoServer************** \e[0m" 

# stop start.sh
echo -e "\e[32m 1.stop start.sh... \e[0m" 
process_id=$(ps -aux|grep start.sh|grep -v "$0"|grep -v "grep"|awk '{print $2}')

if [ ! -n "$process_id" ];then
	echo -e "\e[33m <WARN> Can not find start.sh process,continue... \e[0m" 
else
	echo "Get start.sh process ID=$process_id"
	kill -9 $process_id
fi

# make start.sh
echo -e "\e[32m 2.make start.sh \e[0m"
if [ -f /home/aaeon/start.sh ];then
	echo "start.sh is already exit! continue..."
else
	echo "make start.sh...."
	mkdir -p /home/aaeon/.config/autostart/
	touch /home/aaeon/.config/autostart/bash.desktop
	echo "[Desktop Entry]
Type=Application
Exec=gnome-terminal -x /home/aaeon/start.sh
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
Name[en_US]=aaeon_start
Name=aaeon_start
Comment[en_US]=
Comment=
" > /home/aaeon/.config/autostart/bash.desktop
	chmod 777 /home/aaeon/.config/autostart/bash.desktop
	echo "cd /home/aaeon/AnoServer
./AnoServer.bin
"> /home/aaeon/start.sh
	chmod a+x /home/aaeon/start.sh
fi
