REMOTE_IP_ADDRESS=192.168.1.22
UDP_PORT=5005

ffplay -window_title "UDP" -x 128 -y 128 udp://$REMOTE_IP_ADDRESS:$UDP_PORT



