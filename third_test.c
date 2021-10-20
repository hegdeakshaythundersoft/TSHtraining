#include "header.h" 
struct buffer{

	unsigned int size;
	void *mem;

};
typedef struct video_device{

	int fd;
	enum v4l2_buf_type type;
	enum v4l2_memory memtype;
	unsigned int nbufs;
	struct buffer *buffers;
	unsigned int width;
	unsigned int height;
	unsigned int bytesperline;
	unsigned int imagesize;
	void *pattern;


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
//static int video_set_format(struct device *dev, unsigned int w, unsigned int h, unsigned int format)



static int set_the_format(device *dev,unsigned int w,unsigned int h,unsigned int format){


	struct v4l2_format vid_fmt;
	int ret;
	memset(&vid_fmt,0,sizeof(vid_fmt));
	vid_fmt.type=dev->type;
	vid_fmt.fmt.pix.width=w;
	vid_fmt.fmt.pix.height=h;
	vid_fmt.fmt.pix.pixelformat=format;
	vid_fmt.fmt.pix.field=V4L2_FIELD_ALTERNATE;
	printf("video_set_format:VIDIOC_G_FMT\n");
	ret = ioctl(dev->fd, VIDIOC_G_FMT, &vid_fmt);
	if (ret < 0) {
		printf("Format setting failed: %s (%d).\n", strerror(errno),
			errno);
		return ret;
	}

	printf("Video format set: width: %u height: %u buffer size: %u\n",
		vid_fmt.fmt.pix.width, vid_fmt.fmt.pix.height, vid_fmt.fmt.pix.sizeimage);
	return 0;





}




static int video_device_close(device *dev){

	close(dev->fd);
	return 0;


}


int main(int argc,char* argv[]){
	
	int ret;
	char *device_name="/dev/video0";
	unsigned int pixelformat = V4L2_PIX_FMT_YUYV;
	unsigned int width = 640;
	unsigned int height = 480;
	printf("\nCalling the open function\n");
	ret=video_device_open(&my_video_device,device_name,0);
	if(!(ret<0))
	printf("\nSUCCESSFULLY OPENED\n");
	else
	printf("\nFAILED TO OPEN\n");
	
		/* Set the format */	
	printf("Setting the format\n");	
		ret=set_the_format(&my_video_device,width,height,pixelformat); 
			if(ret < 0){
		printf("video_set_format:before:video_close\n");
			video_device_close(&my_video_device);
			return 1;
		}
	
	
	
	
	video_device_close(&my_video_device);
	printf("\nClosed the sesion\n");
	return 0;
}




/*


new kernel space log :




akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./run_3.sh 


Calling the open function

Video opened successfully

In the open function : VIDEOC_QERYCAP
Device /dev/video0 opened: Integrated Camera: Integrated C (usb-0000:00:14.0-7).

SUCCESSFULLY OPENED
Setting the format
video_set_format:VIDIOC_G_FMT
Video format set: width: 640 height: 480 buffer size: 614400

Closed the sesion


[12707.904940] video0: VIDIOC_QUERYCAP: driver=uvcvideo, card=Integrated Camera: Integrated C, bus=usb-0000:00:14.0-7, version=0x00050b16, capabilities=0x84a00001, device_caps=0x04200001
[12707.905190] video0: VIDIOC_G_FMT: type=vid-cap, width=640, height=480, pixelformat=YUYV, field=none, bytesperline=1280, sizeimage=614400, colorspace=8, flags=0x0, ycbcr_enc=1, quantization=0, xfer_func=1

























old log : userspace 
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./test_app_4 

Calling the open function

Video opened successfully

In the open function : VIDEOC_QERYCAP
Device /dev/video0 opened: Integrated Camera: Integrated C (usb-0000:00:14.0-7).

SUCCESSFULLY OPENED
Setting the format
video_set_format:VIDIOC_G_FMT
Video format set: width: 640 height: 480 buffer size: 614400

Closed the sesion
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ 


*/




