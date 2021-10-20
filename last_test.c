
/*
 * yavta --  Yet Another V4L2 Test Application
 *
 * Copyright (C) 2005-2010 Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 */




/* 1.open the video device
   2.querying the video device capabilities
   3.set the format
   4.set the framerate
   5.allocate  and map buffers
   6.stream on
   7.stream off
   8.free buffers
   9.close the video device */




#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>


#include <linux/videodev2.h>
#include "videodev2-fsr172x.h"

#ifndef V4L2_BUF_FLAG_ERROR
#define V4L2_BUF_FLAG_ERROR	0x0040
#endif

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof((a)[0]))

#define V4L_BUFFERS_DEFAULT	10
#define V4L_BUFFERS_MAX	32

struct buffer                     //struct buffer decalred and its members are size and mem which is void pointer
{
	unsigned int size;
	void *mem;
};


struct device             //struct device declared we are passing one paramter as this strcut for every test  case
{
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
	
}dev;


static int test_video_open(struct device *dev, const char *devname, int no_query);
static void test_video_close(struct device *dev);
static int video_set_format(struct device *dev, unsigned int w, unsigned int h, unsigned int format);
static int video_set_framerate(struct device *dev, struct v4l2_fract *time_per_frame);
static int video_prepare_capture(struct device *dev, int nbufs, unsigned int offset,const char *filename);
static int video_load_test_pattern(struct device *dev, const char *filename);
static int video_alloc_buffers(struct device *dev, int nbufs, unsigned int offset);
static int video_queue_buffer(struct device *dev, int index);
static int video_do_capture(struct device *dev, unsigned int nframes,unsigned int skip, unsigned int delay, 
						const char *filename_prefix,int do_requeue_last);
static int video_free_buffers(struct device *dev);


/*open the device *devname= "/dev/video0" which is passed by user as parameter and also query the capability of devices i.e hardware specific informationn like name of device and type.*/ 
static int test_video_open(struct device *dev, const char *devname, int no_query)
{
	struct v4l2_capability cap;       //this variable is passed as argument quarycap ioctl
	int ret;                          //https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/videodev2.h#L441

	
	dev->fd = open(devname, O_RDWR);      //dev->fd open video0 in read and write mode
	if (dev->fd < 0) {
		printf("Error opening device %s: %d.\n", devname, errno);
		return dev->fd;
	}

	if (!no_query) {
		memset(&cap, 0, sizeof cap);
		printf("test_video_open:VIDIOC_QERYCAP\n");
		ret = ioctl(dev->fd, VIDIOC_QUERYCAP, &cap);      //ioctl arguments file descriptor ,VIDIOC_QUERYCAP annd cap
		if (ret < 0) {
			printf("Error opening device %s: unable to query "
				"device.\n", devname);
			close(dev->fd);
			return ret;
		}

		if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
			dev->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		else if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
			dev->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		else {
			printf("Error opening device %s: neither video capture "
				"nor video output supported.\n", devname);
			close(dev->fd);
			return -EINVAL;
		}

		printf("Device %s opened: %s (%s).\n", devname, cap.card, cap.bus_info);
	} 
	else {
		dev->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		printf("Device %s opened.\n", devname);
	}

	return 0;
}

/* setting the video format taking arguments as height as 480 and width as 640 and format as YUYV */
static int video_set_format(struct device *dev, unsigned int w, unsigned int h, unsigned int format)
{
	struct v4l2_format fmt;    //https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/videodev2.h#L2279
	int ret;
	
	memset(&fmt, 0, sizeof fmt);
	fmt.type = dev->type;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;
	printf("video_set_format:VIDIOC_S_FMT\n");
	ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		printf("Unable to set format: %s (%d).\n", strerror(errno),
			errno);
		return ret;
	}

	printf("Video format set: width: %u height: %u buffer size: %u\n",
		fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage);
		printf("Video format: %c%c%c%c (%08x) %ux%u\n",
		(fmt.fmt.pix.pixelformat >> 0) & 0xff,
		(fmt.fmt.pix.pixelformat >> 8) & 0xff,
		(fmt.fmt.pix.pixelformat >> 16) & 0xff,
		(fmt.fmt.pix.pixelformat >> 24) & 0xff,
		fmt.fmt.pix.pixelformat,
		fmt.fmt.pix.width, fmt.fmt.pix.height);
	return 0;
}
/*setting the frame rate here we are passing  dev strcut and v4l2_fract struct membeers are numarator and denominator */
static int video_set_framerate(struct device *dev, struct v4l2_fract *time_per_frame)   //https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/videodev2.h#L420
{
	struct v4l2_streamparm parm;  //https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/videodev2.h#L2295
	int ret;

	memset(&parm, 0, sizeof parm);  //set the parm variable memory blocks with zero
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
/*allocating 10buffers taking arguments as file descriptor,count ofbuffers,address,filename */
static int video_prepare_capture(struct device *dev, int nbufs, unsigned int offset,
				 const char *filename)
{	
unsigned int i;
	int ret;

	/* Allocate and map buffers. */
	if ((ret = video_alloc_buffers(dev, nbufs, offset)) < 0)
		return ret;

	if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = video_load_test_pattern(dev, filename);
		if (ret < 0)
			return ret;
	}

	/* Queue the buffers. */
	for (i = 0; i < dev->nbufs; ++i) {
		ret = video_queue_buffer(dev, i);
		if (ret < 0)
			return ret;
	}

	return 0;
}
/* allocating the buffers using malloc and mapping the buffers by using mmap */
static int video_alloc_buffers(struct device *dev, int nbufs, unsigned int offset)
{
	struct v4l2_requestbuffers rb;  //https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/videodev2.h#L942
	struct v4l2_buffer buf;       //https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/videodev2.h#L1021
	int page_size;
	struct buffer *buffers;       //members are size and void *
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
//dev type is V4L2_BUF_TYPE_VIDEO_OUTPUT it wil call the below function
static int video_load_test_pattern(struct device *dev, const char *filename)
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

	return 0;
}

/* buffers are placing into input queue */
static int video_queue_buffer(struct device *dev, int index)
{
	struct v4l2_buffer buf;  //https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/videodev2.h#L1021
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

/* stream on and stream off checking */
static int video_enable(struct device *dev, int enable)
{
	int type = dev->type;
	int ret;
	printf("video_enable:enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF\n");
	ret = ioctl(dev->fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		printf("Unable to %s streaming: %d.\n", enable ? "start" : "stop",
			errno);
		return ret;
	}

	return 0;
}


/* buffers are queueing and dequeueing and if stream off buffers will be free */
static int video_do_capture(struct device *dev, unsigned int nframes,
	unsigned int skip, unsigned int delay, const char *filename_prefix,
	int do_requeue_last)
{
	char *filename = NULL;
	struct timeval start = { 0, 0 };
	struct timeval end, ts;
	struct v4l2_buffer buf;
	unsigned int size;
	unsigned int i;
	FILE *file;
	double bps;
	double fps;
	int ret;

	if (filename_prefix != NULL) {
		filename = malloc(strlen(filename_prefix) + 12);
		if (filename == NULL)
			return -ENOMEM;
	}

	/* Start streaming. */
	video_enable(dev, 1);

	size = 0;

	for (i = 0; i < nframes; ++i) {
		/* Dequeue a buffer. */
		memset(&buf, 0, sizeof buf);
		buf.type = dev->type;
		buf.memory = dev->memtype;
		printf("video_do_capture:VIDIOC_DQBUF\n");
		ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf);
		if (ret < 0) {
			if (errno != EIO) {
				printf("Unable to dequeue buffer (%d).\n", errno);
				goto done;
			}
			buf.type = dev->type;
			buf.memory = dev->memtype;
			if (dev->memtype == V4L2_MEMORY_USERPTR)
				buf.m.userptr = (unsigned long)dev->buffers[i].mem;
		}

		if (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE &&
		    dev->imagesize != 0	&& buf.bytesused != dev->imagesize)
			printf("Warning: bytes used %u != image size %u\n",
			       buf.bytesused, dev->imagesize);

		size += buf.bytesused;

		gettimeofday(&ts, NULL);
		printf("%u (%u) [%c] %u %u bytes %ld.%06ld %ld.%06ld\n", i, buf.index,
			(buf.flags & V4L2_BUF_FLAG_ERROR) ? 'E' : '-',
			buf.sequence, buf.bytesused, buf.timestamp.tv_sec,
			buf.timestamp.tv_usec, ts.tv_sec, ts.tv_usec);

		if (i == 0)
			start = ts;

		/* Save the image. */
		if (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE && filename_prefix && !skip) {
			sprintf(filename, "%s-%06u.bin", filename_prefix, i);
			file = fopen(filename, "wb");
			if (file != NULL) {
				ret = fwrite(dev->buffers[buf.index].mem, buf.bytesused, 1, file);
				fclose(file);
			}
		}
		if (skip)
			--skip;

		/* Requeue the buffer. */
		if (delay > 0)
			usleep(delay * 1000);

		fflush(stdout);

		if (i == nframes - dev->nbufs && !do_requeue_last)
			continue;

		ret = video_queue_buffer(dev, buf.index);
		if (ret < 0) {
			printf("Unable to requeue buffer (%d).\n", errno);
			goto done;
		}
	}
	gettimeofday(&end, NULL);

	/* Stop streaming. */
	video_enable(dev, 0);

	if (nframes == 0) {
		printf("No frames captured.\n");
		goto done;
	}

	if (end.tv_sec == start.tv_sec && end.tv_usec == start.tv_usec)
		goto done;

	end.tv_sec -= start.tv_sec;
	end.tv_usec -= start.tv_usec;
	if (end.tv_usec < 0) {
		end.tv_sec--;
		end.tv_usec += 1000000;
	}

	bps = size/(end.tv_usec+1000000.0*end.tv_sec)*1000000.0;
	fps = (i-1)/(end.tv_usec+1000000.0*end.tv_sec)*1000000.0;

	printf("Captured %u frames in %lu.%06lu seconds (%f fps, %f B/s).\n",
		i-1, end.tv_sec, end.tv_usec, fps, bps);

done:
	free(filename);
	return video_free_buffers(dev);
}


/*releasing the buffers(free) by using request buffers to know the count of buffers to be deallocated */
static int video_free_buffers(struct device *dev)
{
	struct v4l2_requestbuffers rb;
	unsigned int i;
	int ret;

	if (dev->nbufs == 0)
		return 0;

	if (dev->memtype == V4L2_MEMORY_MMAP) {
		for (i = 0; i < dev->nbufs; ++i) {
			ret = munmap(dev->buffers[i].mem, dev->buffers[i].size);
			if (ret < 0) {
				printf("Unable to unmap buffer %u (%d)\n", i, errno);
				return ret;
			}
		}
	}

	memset(&rb, 0, sizeof rb);
	rb.count = 0;
	rb.type = dev->type;
	rb.memory = dev->memtype;
	printf("video_free_buffers:VIDIOC_REQBUFS\n");
	ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		printf("Unable to release buffers: %d.\n", errno);
		return ret;
	}

	printf("%u buffers released.\n", dev->nbufs);

	free(dev->buffers);
	dev->nbufs = 0;
	dev->buffers = NULL;

	return 0;
}


/* closing the video device using file descriptor */
static void test_video_close(struct device *dev)
{
	
	close(dev->fd);
}

int main(int argc, char *argv[])
{

	struct device dev;
	int ret;
	
	char* name="/dev/video0";
	unsigned int pixelformat = V4L2_PIX_FMT_YUYV;
	unsigned int width = 640;
	unsigned int height = 480;
	struct v4l2_fract time_per_frame = {1,30};
	enum v4l2_memory memtype = V4L2_MEMORY_MMAP;
	unsigned int nbufs = V4L_BUFFERS_DEFAULT;
	unsigned int userptr_offset = 0;
	const char *filename = "/dev/shm/capture.output";
	unsigned int delay = 0, nframes = 10;
	unsigned int skip=0;
	int do_requeue_last = 0;	

	/*open the video device */
	printf("test_video_open\n");
	ret=test_video_open(&dev,name,0);
		if (ret < 0)
			return 1;

	dev.memtype = memtype;

	/* Set the format */	
	printf("video_set_format:\n");	
		ret=video_set_format(&dev, width, height, pixelformat); 
			if(ret < 0){
		printf("video_set_format:before:video_close\n");
			test_video_close(&dev);
			return 1;
		}
	

	/* Set the frame rate */
	printf("video_set_framerate\n");
		ret=video_set_framerate(&dev, &time_per_frame); 
	if(ret < 0){
	printf("video_set_framerate:before:video_close\n");
			test_video_close(&dev);
			return 1;
		}


	/* allocate and map the buffers */
	printf("video_prepare_capture\n");
	ret=video_prepare_capture(&dev, nbufs, userptr_offset, filename);
      	if(ret < 0) {
	printf("video_prepare_capture:video_close\n");
		test_video_close(&dev);
		return 1;
	}

	/* stream on and stream off */
	printf("video_do_capture\n");
	ret=video_do_capture(&dev, nframes, skip, delay, filename, do_requeue_last);
	if(ret < 0){
	printf("video_do_capture:video_close\n");
		test_video_close(&dev);
		return 1;
	}

	/* close the video device */
	printf("test_video_close\n");
	test_video_close(&dev);

	return 0;
}

/*
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./run_6.sh 

test_video_open
test_video_open:VIDIOC_QERYCAP
Device /dev/video0 opened: Integrated Camera: Integrated C (usb-0000:00:14.0-7).
video_set_format:
video_set_format:VIDIOC_S_FMT
Video format set: width: 640 height: 480 buffer size: 614400
Video format: YUYV (56595559) 640x480
video_set_framerate
Setting frame rate to: 1/30
video_set_framerate:VIDIOC_S_PARAM
Frame rate set: 1/30
video_prepare_capture
video_alloc_buffers:VIDIOC_REQBUFS
10 buffers requested.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 0
Buffer 0 mapped at address 0x7f1db88a4000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 614400
Buffer 1 mapped at address 0x7f1db880e000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 1228800
Buffer 2 mapped at address 0x7f1db8778000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 1843200
Buffer 3 mapped at address 0x7f1db86e2000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 2457600
Buffer 4 mapped at address 0x7f1db864c000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 3072000
Buffer 5 mapped at address 0x7f1db85b6000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 3686400
Buffer 6 mapped at address 0x7f1db8520000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 4300800
Buffer 7 mapped at address 0x7f1db848a000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 4915200
Buffer 8 mapped at address 0x7f1db83f4000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 5529600
Buffer 9 mapped at address 0x7f1db835e000.
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_do_capture
video_enable:enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF
video_do_capture:VIDIOC_DQBUF
0 (0) [-] 0 614400 bytes 12885.764793 1634720831.951965
video_do_capture:VIDIOC_DQBUF
1 (1) [-] 1 614400 bytes 12885.852860 1634720831.983673
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
2 (2) [-] 2 614400 bytes 12885.884639 1634720832.019683
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
3 (3) [-] 3 614400 bytes 12885.920723 1634720832.051607
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
4 (4) [-] 4 614400 bytes 12885.952747 1634720832.083539
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
5 (5) [-] 5 614400 bytes 12885.984671 1634720832.115732
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
6 (6) [-] 6 614400 bytes 12886.016663 1634720832.151778
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
7 (7) [-] 7 614400 bytes 12886.052757 1634720832.183682
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
8 (8) [-] 8 614400 bytes 12886.084722 1634720832.215662
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
9 (9) [-] 9 614400 bytes 12886.116716 1634720832.251771
video_queue_buffer:VIDIOC_QBUF
video_enable:enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF
Captured 9 frames in 0.302707 seconds (29.731721 fps, 20296854.714295 B/s).
video_free_buffers:VIDIOC_REQBUFS
10 buffers released.
test_video_close


[12885.626224] video0: VIDIOC_QUERYCAP: driver=uvcvideo, card=Integrated Camera: Integrated C, bus=usb-0000:00:14.0-7, version=0x00050b16, capabilities=0x84a00001, device_caps=0x04200001
[12885.639422] video0: VIDIOC_S_FMT: type=vid-cap, width=640, height=480, pixelformat=YUYV, field=none, bytesperline=1280, sizeimage=614400, colorspace=8, flags=0x0, ycbcr_enc=1, quantization=0, xfer_func=1
[12885.652533] video0: VIDIOC_S_PARM: type=vid-cap, capability=0x0, capturemode=0x0, timeperframe=1/30, extendedmode=0, readbuffers=0
[12885.653178] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 0, plane 0 offset 0x00000000
[12885.653603] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 1, plane 0 offset 0x00096000
[12885.654172] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 2, plane 0 offset 0x0012c000
[12885.654692] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 3, plane 0 offset 0x001c2000
[12885.655147] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 4, plane 0 offset 0x00258000
[12885.655510] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 5, plane 0 offset 0x002ee000
[12885.655937] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 6, plane 0 offset 0x00384000
[12885.656309] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 7, plane 0 offset 0x0041a000
[12885.656670] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 8, plane 0 offset 0x004b0000
[12885.657053] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 9, plane 0 offset 0x00546000
[12885.657063] videobuf2_common: [cap-00000000f84598f1] __vb2_queue_alloc: allocated 10 buffers, 1 plane(s) each
[12885.657078] video0: VIDIOC_REQBUFS: count=10, type=vid-cap, memory=mmap
[12885.657205] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=0, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x0, length=614400
[12885.657265] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.657435] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 0, plane 0 successfully mapped
[12885.657510] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=1, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x96000, length=614400
[12885.657558] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.657691] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 1, plane 0 successfully mapped
[12885.657880] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=2, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x12c000, length=614400
[12885.657993] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.658184] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 2, plane 0 successfully mapped
[12885.658259] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=3, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x1c2000, length=614400
[12885.658333] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.658520] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 3, plane 0 successfully mapped
[12885.658590] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=4, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x258000, length=614400
[12885.658650] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.658825] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 4, plane 0 successfully mapped
[12885.658898] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=5, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x2ee000, length=614400
[12885.658954] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.659158] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 5, plane 0 successfully mapped
[12885.659268] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=6, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x384000, length=614400
[12885.659322] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.659540] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 6, plane 0 successfully mapped
[12885.659631] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=7, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x41a000, length=614400
[12885.659689] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.659881] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 7, plane 0 successfully mapped
[12885.659964] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=8, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x4b0000, length=614400
[12885.660021] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.660217] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 8, plane 0 successfully mapped
[12885.660304] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=9, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x546000, length=614400
[12885.660355] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12885.660548] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 9, plane 0 successfully mapped
[12885.661163] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 0 succeeded
[12885.661844] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 1 succeeded
[12885.662457] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 2 succeeded
[12885.663116] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 3 succeeded
[12885.663752] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 4 succeeded
[12885.664347] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 5 succeeded
[12885.664927] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 6 succeeded
[12885.665505] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 7 succeeded
[12885.666153] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 8 succeeded
[12885.666735] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 9 succeeded
[12885.670953] videobuf2_common: [cap-00000000f84598f1] vb2_core_streamon: successful
[12885.670971] video0: VIDIOC_STREAMON: type=vid-cap
[12885.671051] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12885.943468] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12885.943490] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 0, state: dequeued
[12885.945505] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12885.975192] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12885.975223] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 1, state: dequeued
[12885.977279] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 1 succeeded
[12885.977324] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.011200] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.011232] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 2, state: dequeued
[12886.014048] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 2 succeeded
[12886.014097] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.043154] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.043169] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 3, state: dequeued
[12886.044336] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 3 succeeded
[12886.044355] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.075086] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.075100] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 4, state: dequeued
[12886.076534] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 4 succeeded
[12886.076562] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.107264] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.107286] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 5, state: dequeued
[12886.110697] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 5 succeeded
[12886.110776] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.143267] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.143310] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 6, state: dequeued
[12886.146376] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 6 succeeded
[12886.146484] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.175201] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.175228] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 7, state: dequeued
[12886.177627] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 7 succeeded
[12886.177664] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.207175] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.207203] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 8, state: dequeued
[12886.210005] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 8 succeeded
[12886.210079] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12886.243261] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12886.243299] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 9, state: dequeued
[12886.246218] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 9 succeeded
[12886.247704] videobuf2_common: [cap-00000000f84598f1] vb2_core_streamoff: successful
[12886.247724] video0: VIDIOC_STREAMOFF: type=vid-cap
[12886.249621] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 0
[12886.249865] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 1
[12886.250059] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 2
[12886.250194] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 3
[12886.250324] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 4
[12886.250617] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 5
[12886.250744] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 6
[12886.250868] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 7
[12886.251008] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 8
[12886.251129] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 9
[12886.251152] video0: VIDIOC_REQBUFS: count=0, type=vid-cap, memory=mmap

*/















