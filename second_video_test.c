#include "header.h" 

typedef struct video_device{

	int fd;
	enum v4l2_buf_type type;


}device;

device my_video_device;

//static int video_device_open(device*,const char*,int);
//static int video_device_close(device*);
static int video_device_open(device *dev,const char* name,int no_query){
	
	struct v4l2_capability cap;
	int ret_val;
	
	dev->fd=open(name,O_RDWR);
	if((dev->fd)>=0)
	printf("\nVideo opened successfully\n");
	else
	printf("Can't open the video");
	
	
	if(!(no_query)){
	memset(&cap,0,sizeof(cap));
	printf("\nIn the open function : VIDEOC_QERYCAP\n");
	ret_val=ioctl(dev->fd,VIDIOC_QUERYCAP,&cap);
	if(ret_val<0){
	
		printf("\nDevice cannot be opened\n");
		close(dev->fd);
	}
	if(cap.capabilities&V4L2_CAP_VIDEO_CAPTURE)
		dev->type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	else if(cap.capabilities&V4L2_CAP_VIDEO_OUTPUT)
		dev->type=V4L2_BUF_TYPE_VIDEO_OUTPUT;
	else{
			printf("Error opening device %s: neither video capture "
				"nor video output supported.\n",name);
			close(dev->fd);
			return -EINVAL;
	
		}
		
		printf("Device %s opened: %s (%s).\n",name, cap.card, cap.bus_info);
	}

	else{
		dev->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		printf("Device %s opened.\n", name);
	}

	
	


}

static int video_device_close(device *dev){

	close(dev->fd);
	return 0;


}


int main(int argc,char* argv[]){
	
	int ret;
	char *device_name="/dev/video0";
	printf("\nCalling the open function\n");
	ret=video_device_open(&my_video_device,device_name,0);
	if(!(ret<0))
	printf("\nSUCCESSFULLY OPENED\n");
	else
	printf("\nFAILED TO OPEN\n");
	video_device_close(&my_video_device);
	printf("\nClosed the sesion\n");
	return 0;
}




/*

kernel space log:




Calling the open function

Video opened successfully

In the open function : VIDEOC_QERYCAP
Device /dev/video0 opened: Integrated Camera: Integrated C (usb-0000:00:14.0-7).

SUCCESSFULLY OPENED

Closed the sesion


[12653.651126] video0: VIDIOC_QUERYCAP: driver=uvcvideo, card=Integrated Camera: Integrated C, bus=usb-0000:00:14.0-7, version=0x00050b16, capabilities=0x84a00001, device_caps=0x04200001























user space log:

akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ make
gcc -Iinclude/linux    second_video_test.c   -o second_video_test
gcc -Iinclude/linux    my_first_video_test.c   -o my_first_video_test
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ 
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ls
header.h  include  Makefile  my_first_video_test  my_first_video_test.c  second_video_test  second_video_test.c
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./second_video_test 

Calling the open function

Video opened successfully

In the open function : VIDEOC_QERYCAP
Device /dev/video0 opened: Integrated Camera: Integrated C (usb-0000:00:14.0-7).

SUCCESSFULLY OPENED

Closed the sesion
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ 






*/




