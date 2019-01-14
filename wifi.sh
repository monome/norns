#!/bin/bash

WIFI_INTERFACE=$(ip addr|grep "^2:" | awk '{print $2}'|sed -e s/:$//)
function wpa_boot {
    WPA_PS=$(ps aux | grep wpa_supplicant |grep -v grep | awk '{print $2}')
    if [ -z $WPA_PS ]; then
	WPA_FILE=$HOME/wpa_supplicant.conf
	echo ctrl_interface=/run/wpa_supplicant > $WPA_FILE
	echo update_config=1 >> $WPA_FILE
	sudo wpa_supplicant -B -i$WIFI_INTERFACE -c$WPA_FILE > /dev/null
    fi
    
    while [ ! -z `sudo wpa_cli status |grep "ctrl_ifname: (nil)"` ]
    do
	    echo connecting
	    sleep 0.1
	done
}

function all_off {
    sudo service hostapd stop &> /dev/null
    sudo service dnsmasq stop &> /dev/null
    sudo killall wpa_supplicant &> /dev/null
    while [ "`ps aux |grep wpa_supplicant |grep -v grep`" ]
    do
	echo waiting for wpa_supplicant to die
	sleep 0.1
    done
    sudo killall dhcpcd &> /dev/null
    sudo ip addr flush dev $WIFI_INTERFACE
}

function wait_scanning {
	sudo wpa_cli status
    while [ `sudo wpa_cli status|grep wpa_state|sed -e s/wpa_state=//` == "SCANNING" ]
    do
	sleep 0.1
    done
}

function wait_associating {
	sudo wpa_cli status
    tries=0;
    while [ `sudo wpa_cli status|grep wpa_state|sed -e s/wpa_state=//` != "COMPLETED" ]
    do
	sudo wpa_cli status
	tries=$((tries+1));
	sleep 0.1
	if [ $tries -gt 50 ]
       	then
	    echo "password fail" > ~/status.wifi
	    exit
	fi
    done
}

if [ -d $1 ]; then
    echo usage:
    echo ./wifi.sh on
    echo ./wifi.sh off
    echo ./wifi.sh hotspot
    echo ./wifi.sh scan
    echo ./wifi.sh select "\"My Wifi SSID\"" "\"wifi-password\""
elif [ $1 = "on" ]; then
    if grep "initialising\|router" $HOME/status.wifi; then
	echo "wifi.sh already running"
	exit;
    fi
    echo initialising wifi...> $HOME/status.wifi
    all_off;
    wpa_boot;
    SSID=$(cat $HOME/ssid.wifi);
    PSK=$(cat $HOME/psk.wifi);

    sudo wpa_cli disable_network 0 &> /dev/null

    sudo wpa_cli remove_network 0 &> /dev/null
    sudo wpa_cli remove_network 1 &> /dev/null
    sudo wpa_cli remove_network 2 &> /dev/null
    sudo wpa_cli remove_network 3 &> /dev/null
    sudo wpa_cli remove_network 4 &> /dev/null

    sudo wpa_cli add_network

    if [ -n "$PSK" ]; then
        sudo wpa_cli set_network 0 psk "\"$PSK\""
    else
        sudo wpa_cli set_network 0 key_mgmt NONE
    fi

    echo sudo wpa_cli set_network 0 ssid "\"$SSID\""
    sudo wpa_cli set_network 0 ssid "\"$SSID\""

    sudo wpa_cli enable_network 0

    wait_associating;
    sudo dhcpcd
    
    gw=$(ip route |grep default |awk '{print $3}')
    n=0
    until [ -n "$gw" ] || [ $n -ge 5 ] ; do # retry
	    echo retrying gateway check...
	    sleep 1s
	    gw=$(ip route |grep default |awk '{print $3}')
	    n=$[$n+1]
    done
    
    if [ -d $gw ]; then
	    echo failed to get a gateway > $HOME/status.wifi
    else
	    ping -c 1 $gw
	    if [ $? -ne 0 ]; then
		    echo no gateway ping > $HOME/status.wifi
	    else
		    echo router > $HOME/status.wifi
	    fi
    fi
elif [ $1 = "scan" ]; then
    wpa_boot;
    sudo wpa_cli scan > /dev/null;
    wait_scanning;
    sudo wpa_cli scan_results \
	| sed -e s/.\*\\\]// -e s/\[\ \\t\]\*// \
	| awk '(NR>2) {print};' > $HOME/scan.wifi
elif [ $1 = "select" ]; then
    if [ -n "$1" ]; then
	echo $2 > $HOME/ssid.wifi;
	echo $3 > $HOME/psk.wifi;
    else
	echo "usage: ./wifi.sh select \"SSID\" \"PSK\""
    fi
elif [ $1 = "hotspot" ]; then
    if grep "initialising\|hotspot" $HOME/status.wifi; then
	echo "wifi.sh already running"
	exit;
    fi
    echo initialising hotspot... > $HOME/status.wifi
    all_off
    sudo ip addr add 172.24.1.1/255.255.255.0 \
            broadcast 172.24.1.255 dev $WIFI_INTERFACE
    sudo service hostapd start
    sudo service dnsmasq start

    if [ $? -ne 0 ]; then
	    echo failed > $HOME/status.wifi
    else
	    echo hotspot > $HOME/status.wifi
    fi
elif [ $1 = "off" ]; then
    kill `ps aux | grep wifi.sh | grep -v $$ | awk '{print $2}'` &> /dev/null
    echo stopped > $HOME/status.wifi
    all_off
else
    echo invalid command: $1
    echo usage:
    echo ./wifi.sh on
    echo ./wifi.sh off
    echo ./wifi.sh hotspot
    echo ./wifi.sh scan
    echo ./wifi.sh select "\"My Wifi SSID\"" "\"wifi-password\""
fi

