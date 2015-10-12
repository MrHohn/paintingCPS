#
# Purpose: Shell script, the finger saver
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 1.0
# License: 
#

case $1 in
start)
	echo "now start the storm program"
	zkServer.sh start
	storm nimbus >/dev/null 2>&1 &
	storm ui >/dev/null 2>&1 &
	storm supervisor >/dev/null 2>&1 &
	java assist.SpoutFinder >/dev/null 2>&1 &
	;;
start-all)
	echo "now start programs on all machines"
	zkServer.sh start
	storm nimbus >/dev/null 2>&1 &
	storm ui >/dev/null 2>&1 &
	storm supervisor >/dev/null 2>&1 &
	ssh root@node1-2 "storm supervisor >/dev/null 2>&1 &"
	java assist.SpoutFinder >/dev/null 2>&1 &
	;;
make)
	echo "now complie the mvn project"
	mvn clean package
    ;;
run)
	echo "now submit the topo as CPS"
	storm jar target/storm-winlab-cps-0.9.5-jar-with-dependencies.jar storm.winlab.cps.StormMatch CPS
	;;
kill)
	echo "now kill the topo CPS"
	storm kill CPS
	;;
stop)
        echo "now kill all storm processes"
        jps -l | grep core | cut -d ' ' -f 1 | xargs -rn1 kill
        jps -l | grep nimbus | cut -d ' ' -f 1 | xargs -rn1 kill
        jps -l | grep QuorumPeerMain | cut -d ' ' -f 1 | xargs -rn1 kill
        jps -l | grep supervisor | cut -d ' ' -f 1 | xargs -rn1 kill
        jps -l | grep SpoutFinder | cut -d ' ' -f 1 | xargs -rn1 kill
	;;
esac
