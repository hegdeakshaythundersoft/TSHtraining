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
static int video_load_test_pattern(device *dev, const char *filename);
static int video_alloc_buffers(device *dev, int nbufs, unsigned int offset);
static int video_queue_buffer(device *dev, int index);

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
	vid_fmt.fmt.pix.field=V4L2_FIELD_ANY;
	printf("video_set_format:VIDIOC_S_FMT\n");
	ret = ioctl(dev->fd, VIDIOC_S_FMT, &vid_fmt);
	if (ret < 0) {
		printf("Format setting failed: %s (%d).\n", strerror(errno),
			errno);
		return ret;
	}

	printf("Video format set: width: %u height: %u buffer size: %u\n",
		vid_fmt.fmt.pix.width, vid_fmt.fmt.pix.height, vid_fmt.fmt.pix.sizeimage);
	return 0;





}
static int set_frame_rate(device *dev,struct v4l2_fract *time_per_frame){


	struct v4l2_streamparm parm;	
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = dev->type;

	printf("Setting frame rate to: %u/%u\n",
		time_per_frame->numerator,
		time_per_frame->denominator);

	parm.parm.capture.timeperframe.numerator = time_per_frame->numerator;
	parm.parm.capture.timeperframe.denominator = time_per_frame->denominator;
	printf("video_set_framerate:VIDIOC_S_PARAM\n");
	ret = ioctl(dev->fd, VIDIOC_S_PARM, &parm);
	if (ret < 0) {
		printf("Unable to set frame rate: %d.\n", errno);
		return ret;
	}
	

	printf("Frame rate set: %u/%u\n",
		parm.parm.capture.timeperframe.numerator,
		parm.parm.capture.timeperframe.denominator);
	return 0;

}


static int capture_prepare(device *dev,int nbufs,unsigned int offset,const char *filename){

	unsigned int i;
	int ret_val;
	/*allocate the map buffers*/
	ret_val=video_alloc_buffers(dev,nbufs,offset);
	if(ret_val<0)
		return ret_val;
	if(dev->type==V4L2_BUF_TYPE_VIDEO_OUTPUT){
		ret_val = video_load_test_pattern(dev,filename);
		if (ret_val < 0)
			return ret_val;
	}
	
	/*que the buffers*/
	
	for (i = 0; i < dev->nbufs; ++i) {
		ret_val = video_queue_buffer(dev, i);
		if (ret_val < 0)
			return ret_val;
	}

	return 0;
	
	
	
}


static int video_alloc_buffers(device *dev, int nbufs, unsigned int offset)
{
	struct v4l2_requestbuffers rb;
	struct v4l2_buffer buf;
	int page_size;
	struct buffer *buffers;
	unsigned int i;
	int ret;

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = dev->type;
	rb.memory = dev->memtype;
	printf("video_alloc_buffers:VIDIOC_REQBUFS\n");
	ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		printf("Unable to request buffers: %d.\n", errno);
		return ret;
	}

	printf("%u buffers requested.\n", rb.count);

	buffers = malloc(rb.count * sizeof buffers[0]);
	if (buffers == NULL)
		return -ENOMEM;

	page_size = getpagesize();

	/* Map the buffers. */
	for (i = 0; i < rb.count; ++i) {
		memset(&buf, 0, sizeof buf);
		buf.index = i;
		buf.type = dev->type;
		buf.memory = dev->memtype;
		printf("video_alloc_buffers:VIDIOC_QUERYBUF\n");
		ret = ioctl(dev->fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			printf("Unable to query buffer %u (%d).\n", i, errno);
			return ret;
		}
		printf("length: %u offset: %u\n", buf.length, buf.m.offset);

		switch (dev->memtype) {
		case V4L2_MEMORY_MMAP:
			buffers[i].mem = mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, buf.m.offset);
			if (buffers[i].mem == MAP_FAILED) {
				printf("Unable to map buffer %u (%d)\n", i, errno);
				return ret;
			}
			buffers[i].size = buf.length;
			printf("Buffer %u mapped at address %p.\n", i, buffers[i].mem);
			break;

		case V4L2_MEMORY_USERPTR:
			ret = posix_memalign(&buffers[i].mem, page_size, buf.length + offset);
			if (ret < 0) {
				printf("Unable to allocate buffer %u (%d)\n", i, ret);
				return -ENOMEM;
			}
			buffers[i].mem += offset;
			buffers[i].size = buf.length;
			printf("Buffer %u allocated at address %p.\n", i, buffers[i].mem);
			break;

		default:
			break;
		}
	}

	dev->buffers = buffers;
	dev->nbufs = rb.count;
	return 0;
}

static int video_load_test_pattern(device *dev, const char *filename)
{
printf("video_load_test_pattern\n");	
unsigned int size = dev->buffers[0].size;
	unsigned int x, y;
	uint8_t *data;
	int ret;
	int fd;
	/* Load or generate the test pattern */
	dev->pattern = malloc(size);
	if (dev->pattern == NULL)
		return -ENOMEM;

	if (filename == NULL) {
		if (dev->bytesperline == 0) {
			printf("Compressed format detect and no test pattern filename given.\n"
				"The test pattern can't be generated automatically.\n");
			return -EINVAL;
		}

		data = dev->pattern;

		for (y = 0; y < dev->height; ++y) {
			for (x = 0; x < dev->bytesperline; ++x)
				*data++ = x + y;
		}

		return 0;
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf("Unable to open test pattern file '%s': %s.\n", filename,
			strerror(errno));
		return -errno;
	}

	ret = read(fd, dev->pattern, size);
	close(fd);

	if (ret != (int)size) {
		printf("Test pattern file size %u doesn't match image size %u\n",
			ret, size);
		return -EINVAL;
	}
	printf("LOAD TEST SUCCESS\n");
	return 0;
}


static int video_queue_buffer(device *dev, int index)
{
	struct v4l2_buffer buf;
	int ret;

	memset(&buf, 0, sizeof buf);
	buf.index = index;
	buf.type = dev->type;
	buf.memory = dev->memtype;
	buf.length = dev->buffers[index].size;
	if (dev->memtype == V4L2_MEMORY_USERPTR)
		buf.m.userptr = (unsigned long)dev->buffers[index].mem;

	if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		buf.bytesused = buf.length;
		memcpy(dev->buffers[buf.index].mem, dev->pattern, buf.bytesused);
	} else
		memset(dev->buffers[buf.index].mem, 0x55, buf.length);
	printf("video_queue_buffer:VIDIOC_QBUF\n");
	ret = ioctl(dev->fd, VIDIOC_QBUF, &buf);
	if (ret < 0)
		printf("Unable to queue buffer (%d).\n", errno);

	return ret;
}






















static int video_device_close(device *dev){

	close(dev->fd);
	return 0;


}


int main(int argc,char* argv[]){
	
	int ret;
	char *device_name="/dev/video0";
	device dev;
	unsigned int pixelformat = V4L2_PIX_FMT_YUYV;
	unsigned int width = 640;
	unsigned int height = 480;
	struct v4l2_fract time_per_frame = {1,30};
	enum v4l2_memory memtype = V4L2_MEMORY_MMAP;
	unsigned int nbufs =10;
	unsigned int userptr_offset = 0;
	const char *filename = "/dev/shm/capture.output";
	printf("\nCalling the open function\n");
	ret=video_device_open(&dev,device_name,0);
	if(!(ret<0))
	printf("\nSUCCESSFULLY OPENED\n");
	else
	printf("\nFAILED TO OPEN\n");
	
		/* Set the format */	
	printf("Setting the format\n");	
		ret=set_the_format(&dev,width,height,pixelformat); 
			if(ret < 0){
		printf("video_set_format:before:video_close\n");
			video_device_close(&dev);
			return 1;
		}
	
	/*set_frame_rate*/
	
	ret=set_frame_rate(&dev,&time_per_frame);
	if(ret<0){
	printf("Framerate setting failed\n");
	goto close;
	}
	
	/* allocate and map the buffers */
	printf("video_prepare_capture\n");
	ret=capture_prepare(&dev, nbufs, userptr_offset, filename);
      if(ret < 0) {
	printf("video_prepare_capture:video_close\n");
	goto close;
	}
	

	
	
close:video_device_close(&dev);
	printf("\nClosed the sesion\n");
	return 0;
}




/*


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




