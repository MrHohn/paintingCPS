zkServer.sh start
storm nimbus >/dev/null 2>&1 &
storm ui >/dev/null 2>&1 &
storm supervisor >/dev/null 2>&1 &
