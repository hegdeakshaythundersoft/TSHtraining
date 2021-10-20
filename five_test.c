
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
   8.close the video device */




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



struct buffer
{
	unsigned int size;
	void *mem;
};


struct device
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



static int test_video_open(struct device *dev, const char *devname, int no_query)
{
	struct v4l2_capability cap;
	int ret;

	
	dev->fd = open(devname, O_RDWR);
	if (dev->fd < 0) {
		printf("Error opening device %s: %d.\n", devname, errno);
		return dev->fd;
	}

	if (!no_query) {
		memset(&cap, 0, sizeof cap);
		printf("test_video_open:VIDIOC_QERYCAP\n");
		ret = ioctl(dev->fd, VIDIOC_QUERYCAP, &cap);
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


static int video_set_format(struct device *dev, unsigned int w, unsigned int h, unsigned int format)
{
	struct v4l2_format fmt;
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
	return 0;
}

static int video_set_framerate(struct device *dev, struct v4l2_fract *time_per_frame)
{
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

static int video_alloc_buffers(struct device *dev, int nbufs, unsigned int offset)
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


static int video_queue_buffer(struct device *dev, int index)
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
				free(filename);
				return 1;
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
			free(filename);
			return 1;
		}
	}
	gettimeofday(&end, NULL);

	/* Stop streaming. */
	video_enable(dev, 0);

	if (nframes == 0) {
		printf("No frames captured.\n");
		free(filename);
		return 1;
	}

return 0;
}

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
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./run_4.sh 

test_video_open

*********4 = size of V4L2_CAP_VIDEO_CAPTURE***********
test_video_open:VIDIOC_QERYCAP
Device /dev/video0 opened: Integrated Camera: Integrated C (usb-0000:00:14.0-7).
video_set_format:
video_set_format:VIDIOC_S_FMT
Video format set: width: 640 height: 480 buffer size: 614400
video_set_framerate
Setting frame rate to: 1/30
video_set_framerate:VIDIOC_S_PARAM
Frame rate set: 1/30
video_prepare_capture
video_alloc_buffers:VIDIOC_REQBUFS
15 buffers requested.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 0
Buffer 0 mapped at address 0x7fc05f595000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 614400
Buffer 1 mapped at address 0x7fc05f4ff000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 1228800
Buffer 2 mapped at address 0x7fc05f469000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 1843200
Buffer 3 mapped at address 0x7fc05f3d3000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 2457600
Buffer 4 mapped at address 0x7fc05f33d000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 3072000
Buffer 5 mapped at address 0x7fc05f2a7000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 3686400
Buffer 6 mapped at address 0x7fc05f211000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 4300800
Buffer 7 mapped at address 0x7fc05f17b000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 4915200
Buffer 8 mapped at address 0x7fc05f0e5000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 5529600
Buffer 9 mapped at address 0x7fc05f04f000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 6144000
Buffer 10 mapped at address 0x7fc05efb9000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 6758400
Buffer 11 mapped at address 0x7fc05ef23000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 7372800
Buffer 12 mapped at address 0x7fc05ee8d000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 7987200
Buffer 13 mapped at address 0x7fc05edf7000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 8601600
Buffer 14 mapped at address 0x7fc05ed61000.
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
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
video_queue_buffer:VIDIOC_QBUF
test_video_close


[12771.270219] video0: VIDIOC_QUERYCAP: driver=uvcvideo, card=Integrated Camera: Integrated C, bus=usb-0000:00:14.0-7, version=0x00050b16, capabilities=0x84a00001, device_caps=0x04200001
[12771.282841] video0: VIDIOC_S_FMT: type=vid-cap, width=640, height=480, pixelformat=YUYV, field=none, bytesperline=1280, sizeimage=614400, colorspace=8, flags=0x0, ycbcr_enc=1, quantization=0, xfer_func=1
[12771.295359] video0: VIDIOC_S_PARM: type=vid-cap, capability=0x0, capturemode=0x0, timeperframe=1/30, extendedmode=0, readbuffers=0
[12771.296050] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 0, plane 0 offset 0x00000000
[12771.296556] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 1, plane 0 offset 0x00096000
[12771.297047] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 2, plane 0 offset 0x0012c000
[12771.297535] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 3, plane 0 offset 0x001c2000
[12771.298941] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 4, plane 0 offset 0x00258000
[12771.299537] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 5, plane 0 offset 0x002ee000
[12771.300027] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 6, plane 0 offset 0x00384000
[12771.300384] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 7, plane 0 offset 0x0041a000
[12771.300716] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 8, plane 0 offset 0x004b0000
[12771.301108] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 9, plane 0 offset 0x00546000
[12771.301465] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 10, plane 0 offset 0x005dc000
[12771.301935] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 11, plane 0 offset 0x00672000
[12771.302420] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 12, plane 0 offset 0x00708000
[12771.302789] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 13, plane 0 offset 0x0079e000
[12771.303135] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 14, plane 0 offset 0x00834000
[12771.303145] videobuf2_common: [cap-00000000f84598f1] __vb2_queue_alloc: allocated 15 buffers, 1 plane(s) each
[12771.303167] video0: VIDIOC_REQBUFS: count=15, type=vid-cap, memory=mmap
[12771.303313] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=0, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x0, length=614400
[12771.303364] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.303541] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 0, plane 0 successfully mapped
[12771.303666] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=1, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x96000, length=614400
[12771.303710] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.303838] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 1, plane 0 successfully mapped
[12771.303929] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=2, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x12c000, length=614400
[12771.303967] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.304091] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 2, plane 0 successfully mapped
[12771.304176] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=3, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x1c2000, length=614400
[12771.304210] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.304332] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 3, plane 0 successfully mapped
[12771.304387] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=4, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x258000, length=614400
[12771.304427] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.304542] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 4, plane 0 successfully mapped
[12771.304596] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=5, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x2ee000, length=614400
[12771.304631] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.304741] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 5, plane 0 successfully mapped
[12771.304792] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=6, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x384000, length=614400
[12771.304828] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.304943] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 6, plane 0 successfully mapped
[12771.305032] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=7, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x41a000, length=614400
[12771.305071] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.305290] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 7, plane 0 successfully mapped
[12771.305472] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=8, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x4b0000, length=614400
[12771.305520] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.305772] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 8, plane 0 successfully mapped
[12771.305966] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=9, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x546000, length=614400
[12771.306006] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.306227] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 9, plane 0 successfully mapped
[12771.306412] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=10, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x5dc000, length=614400
[12771.306450] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.306594] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 10, plane 0 successfully mapped
[12771.306761] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=11, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x672000, length=614400
[12771.306809] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.306994] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 11, plane 0 successfully mapped
[12771.307094] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=12, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x708000, length=614400
[12771.307127] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.307239] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 12, plane 0 successfully mapped
[12771.307330] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=13, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x79e000, length=614400
[12771.307365] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.307497] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 13, plane 0 successfully mapped
[12771.307552] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=14, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x834000, length=614400
[12771.307586] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12771.307699] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 14, plane 0 successfully mapped
[12771.308186] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 0 succeeded
[12771.308757] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 1 succeeded
[12771.309367] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 2 succeeded
[12771.310492] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 3 succeeded
[12771.311126] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 4 succeeded
[12771.311739] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 5 succeeded
[12771.312377] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 6 succeeded
[12771.313077] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 7 succeeded
[12771.313906] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 8 succeeded
[12771.314644] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 9 succeeded
[12771.315206] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 10 succeeded
[12771.316036] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 11 succeeded
[12771.316537] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 12 succeeded
[12771.317014] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 13 succeeded
[12771.317531] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 14 succeeded
[12771.319568] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 0
[12771.319694] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 1
[12771.319819] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 2
[12771.319947] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 3
[12771.320050] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 4
[12771.320175] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 5
[12771.320321] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 6
[12771.320404] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 7
[12771.320487] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 8
[12771.320579] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 9
[12771.320660] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 10
[12771.320756] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 11
[12771.320836] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 12
[12771.320917] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 13
[12771.321009] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 14
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./run_5.sh 

test_video_open
test_video_open:VIDIOC_QERYCAP
Device /dev/video0 opened: Integrated Camera: Integrated C (usb-0000:00:14.0-7).
video_set_format:
video_set_format:VIDIOC_S_FMT
Video format set: width: 640 height: 480 buffer size: 614400
video_set_framerate
Setting frame rate to: 1/30
video_set_framerate:VIDIOC_S_PARAM
Frame rate set: 1/30
video_prepare_capture
video_alloc_buffers:VIDIOC_REQBUFS
10 buffers requested.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 0
Buffer 0 mapped at address 0x7f9ae375a000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 614400
Buffer 1 mapped at address 0x7f9ae36c4000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 1228800
Buffer 2 mapped at address 0x7f9ae362e000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 1843200
Buffer 3 mapped at address 0x7f9ae3598000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 2457600
Buffer 4 mapped at address 0x7f9ae3502000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 3072000
Buffer 5 mapped at address 0x7f9ae346c000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 3686400
Buffer 6 mapped at address 0x7f9ae33d6000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 4300800
Buffer 7 mapped at address 0x7f9ae3340000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 4915200
Buffer 8 mapped at address 0x7f9ae32aa000.
video_alloc_buffers:VIDIOC_QUERYBUF
length: 614400 offset: 5529600
Buffer 9 mapped at address 0x7f9ae3214000.
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
0 (0) [-] 0 614400 bytes 12828.654159 1634720774.841252
video_do_capture:VIDIOC_DQBUF
1 (1) [-] 1 614400 bytes 12828.742227 1634720774.873134
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
2 (2) [-] 2 614400 bytes 12828.774180 1634720774.909165
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
3 (3) [-] 3 614400 bytes 12828.810195 1634720774.941147
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
4 (4) [-] 4 614400 bytes 12828.842163 1634720774.973122
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
5 (5) [-] 5 614400 bytes 12828.874132 1634720775.009181
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
6 (6) [-] 6 614400 bytes 12828.910196 1634720775.041147
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
7 (7) [-] 7 614400 bytes 12828.942156 1634720775.073271
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
8 (8) [-] 8 614400 bytes 12828.974329 1634720775.105176
video_queue_buffer:VIDIOC_QBUF
video_do_capture:VIDIOC_DQBUF
9 (9) [-] 9 614400 bytes 12829.006183 1634720775.140993
video_queue_buffer:VIDIOC_QBUF
video_enable:enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF
test_video_close


[12828.520243] video0: VIDIOC_QUERYCAP: driver=uvcvideo, card=Integrated Camera: Integrated C, bus=usb-0000:00:14.0-7, version=0x00050b16, capabilities=0x84a00001, device_caps=0x04200001
[12828.533258] video0: VIDIOC_S_FMT: type=vid-cap, width=640, height=480, pixelformat=YUYV, field=none, bytesperline=1280, sizeimage=614400, colorspace=8, flags=0x0, ycbcr_enc=1, quantization=0, xfer_func=1
[12828.545745] video0: VIDIOC_S_PARM: type=vid-cap, capability=0x0, capturemode=0x0, timeperframe=1/30, extendedmode=0, readbuffers=0
[12828.546643] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 0, plane 0 offset 0x00000000
[12828.547070] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 1, plane 0 offset 0x00096000
[12828.547376] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 2, plane 0 offset 0x0012c000
[12828.547811] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 3, plane 0 offset 0x001c2000
[12828.548250] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 4, plane 0 offset 0x00258000
[12828.548645] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 5, plane 0 offset 0x002ee000
[12828.548964] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 6, plane 0 offset 0x00384000
[12828.549267] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 7, plane 0 offset 0x0041a000
[12828.549561] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 8, plane 0 offset 0x004b0000
[12828.549872] videobuf2_common: [cap-00000000f84598f1] __setup_offsets: buffer 9, plane 0 offset 0x00546000
[12828.549881] videobuf2_common: [cap-00000000f84598f1] __vb2_queue_alloc: allocated 10 buffers, 1 plane(s) each
[12828.549896] video0: VIDIOC_REQBUFS: count=10, type=vid-cap, memory=mmap
[12828.550028] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=0, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x0, length=614400
[12828.550076] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.550250] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 0, plane 0 successfully mapped
[12828.550323] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=1, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x96000, length=614400
[12828.550361] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.550488] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 1, plane 0 successfully mapped
[12828.550536] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=2, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x12c000, length=614400
[12828.550565] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.550683] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 2, plane 0 successfully mapped
[12828.550728] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=3, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x1c2000, length=614400
[12828.550756] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.550878] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 3, plane 0 successfully mapped
[12828.550923] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=4, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x258000, length=614400
[12828.550951] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.551067] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 4, plane 0 successfully mapped
[12828.551124] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=5, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x2ee000, length=614400
[12828.551159] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.551279] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 5, plane 0 successfully mapped
[12828.551323] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=6, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x384000, length=614400
[12828.551350] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.551467] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 6, plane 0 successfully mapped
[12828.551520] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=7, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x41a000, length=614400
[12828.551552] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.551753] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 7, plane 0 successfully mapped
[12828.551843] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=8, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x4b0000, length=614400
[12828.551918] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.552085] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 8, plane 0 successfully mapped
[12828.552149] video0: VIDIOC_QUERYBUF: 00:00:00.000000000 index=9, type=vid-cap, request_fd=0, flags=0x00012000, field=any, sequence=0, memory=mmap, bytesused=0, offset/userptr=0x546000, length=614400
[12828.552193] timecode=00:00:00 type=0, flags=0x00000000, frames=0, userbits=0x00000000
[12828.552485] videobuf2_common: [cap-00000000f84598f1] vb2_mmap: buffer 9, plane 0 successfully mapped
[12828.553083] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 0 succeeded
[12828.553863] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 1 succeeded
[12828.554441] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 2 succeeded
[12828.555186] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 3 succeeded
[12828.555842] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 4 succeeded
[12828.556406] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 5 succeeded
[12828.556959] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 6 succeeded
[12828.557517] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 7 succeeded
[12828.558094] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 8 succeeded
[12828.558718] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 9 succeeded
[12828.562255] videobuf2_common: [cap-00000000f84598f1] vb2_core_streamon: successful
[12828.562273] video0: VIDIOC_STREAMON: type=vid-cap
[12828.562368] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12828.834639] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12828.834661] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 0, state: dequeued
[12828.836855] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12828.866537] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12828.866565] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 1, state: dequeued
[12828.869296] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 1 succeeded
[12828.869354] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12828.902582] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12828.902602] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 2, state: dequeued
[12828.904798] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 2 succeeded
[12828.904835] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12828.934563] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12828.934584] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 3, state: dequeued
[12828.937360] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 3 succeeded
[12828.937462] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12828.966536] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12828.966558] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 4, state: dequeued
[12828.969090] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 4 succeeded
[12828.969128] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12829.002591] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12829.002614] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 5, state: dequeued
[12829.004859] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 5 succeeded
[12829.004917] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12829.034546] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12829.034573] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 6, state: dequeued
[12829.037104] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 6 succeeded
[12829.037160] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12829.066671] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12829.066697] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 7, state: dequeued
[12829.069507] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 7 succeeded
[12829.069569] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12829.098586] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12829.098607] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 8, state: dequeued
[12829.101495] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 8 succeeded
[12829.101560] videobuf2_common: [cap-00000000f84598f1] __vb2_wait_for_done_vb: will sleep waiting for buffers
[12829.134429] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: returning done buffer
[12829.134438] videobuf2_common: [cap-00000000f84598f1] vb2_core_dqbuf: dqbuf of buffer 9, state: dequeued
[12829.134989] videobuf2_common: [cap-00000000f84598f1] vb2_core_qbuf: qbuf of buffer 9 succeeded
[12829.135851] videobuf2_common: [cap-00000000f84598f1] vb2_core_streamoff: successful
[12829.135858] video0: VIDIOC_STREAMOFF: type=vid-cap
[12829.136460] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 0
[12829.136514] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 1
[12829.136552] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 2
[12829.136581] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 3
[12829.136608] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 4
[12829.136629] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 5
[12829.136653] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 6
[12829.136673] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 7
[12829.136695] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 8
[12829.136726] videobuf2_common: [cap-00000000f84598f1] __vb2_buf_mem_free: freed plane 0 of buffer 9









*/
