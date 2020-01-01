r2=$(getent ahosts "r2" | cut -d " " -f 1 | uniq)

r2_adapter=$(ip route get $r2 | grep -Po '(?<=(dev )).*(?= src| proto)')

sudo tc qdisc replace dev $r2_adapter root netem delay 3ms loss 5%
