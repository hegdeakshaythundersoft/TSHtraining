echo ""
sudo dmesg -C
sudo sh -c 'echo "3" >> /sys/class/video4linux/video0/dev_debug'
sudo sh -c 'echo "3" >> /sys/module/videobuf2_v4l2/parameters/debug'
sudo sh -c 'echo "3" >> /sys/module/videobuf2_common/parameters/debug'
./last_test --capture=1000 -n 3 --encode-to=file.h264 -f UYVY -m -T /dev/video0
echo ""
echo ""
 sudo dmesg
	
	

