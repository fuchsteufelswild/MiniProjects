r1=$(getent ahosts "r1" | cut -d " " -f 1 | uniq)
r3=$(getent ahosts "r3" | cut -d " " -f 1 | uniq)

r1_adapter=$(ip route get $r1 | grep -Po '(?<=(dev )).*(?= src| proto)')
r3_adapter=$(ip route get $r3 | grep -Po '(?<=(dev )).*(?= src| proto)')

sudo tc qdisc replace dev $r1_adapter root netem delay 3ms loss 38%
sudo tc qdisc replace dev $r3_adapter root netem delay 3ms loss 38%

