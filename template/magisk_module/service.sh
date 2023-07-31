MODDIR=${0%/*}

echo "2: $(date)\n" >> $MODDIR/log.txt
cp -rf /system/app/hook.js /data/local/tmp/
