#include "header.h"
int main(){




	struct v4l2_rect rect;
	struct v4l2_buffer buf;
	struct v4l2_fract f;
	struct v4l2_pix_format fmt;
	struct v4l2_frmsize_discrete frm_dicrete;
	struct v4l2_frmsize_stepwise step;
	struct v4l2_frmsizeenum  frmsizeenum;
	struct v4l2_frmival_stepwise  frmival_stepwise;
	struct v4l2_frmivalenum frmivalenum;
	struct v4l2_timecode time_code;
	struct v4l2_jpegcompression jpegcompression; 
	struct v4l2_requestbuffers rb;
	
	
	
	
	
	
	printf("From the struct v4l2_buffer buf : \n");
	printf("Index : %u\t",buf.index);
	printf("Type : %u\t",buf.type);
	printf("Bytes used : %u\t",buf.bytesused);
	printf("Flags : %u\t",buf.flags);
	printf("Sequence : %u\t",buf.sequence);
	printf("Field : %u\t",buf.field);
	printf("Field : %u\t",buf.length);
	printf("buf.m.userptr : %lu\t",buf.m.userptr);
	printf("buf.m.offset : %u\t",buf.m.offset);
	printf("Userptr reserved : %u\t",buf.reserved);
	printf("Userptr reserved2 : %u\t",buf.reserved2);
		
	

	printf("From the struct v4l2_fract  f: \n");
	printf("\nNumerator : %u\t",f.numerator);
	printf("Denominator : %u\n",f.denominator);
	
	
	
	
	
	printf("From the v4l2_struct ap :\n");
	printf("\n");
	
	
	
	
	
	printf("From the struct struct v4l2_rect : \n");
	printf("All are __s32\n");
	printf("left:%d\t",rect.left);
	printf("top:%d\t",rect.top);
	printf("width:%d\t",rect.top);
	printf("height:%d\t",rect.height);
	
	
	
	
	printf("From the struct v4l2_pix_format : \n");
	printf("All are __u32\n");
	printf("fmt.width:%u\t",fmt.width);	
	printf("fmt.height:%u\t",fmt.height);	
	printf("fmt.pixelformat:%u\t",fmt.pixelformat);	
	printf("fmt.bytesperline:%u\t",fmt.bytesperline);	/* for padding, zero if unused */
	printf("fmt.field:%u\t",fmt.field);	 /* enum v4l2_field */
	printf("fmt.sizeimage:%u\t",fmt.sizeimage);	
	printf("fmt.colorspace:%u\t",fmt.colorspace);
	printf("fmt.priv:%u\n",fmt.priv);	/* private data, depends on pixelformat */
			
					
					
	printf("From the struct struct v4l2_frmsize_discrete frm_dicrete : \n");
	printf("All are __u32\n");
	printf("frm_dicrete.width:%u\t",frm_dicrete.width);	
	printf("frm_dicrete.height:%u\t",frm_dicrete.height);

	printf("From the struct v4l2_frmsize_stepwise step : \n");
	printf("All are __u32\n");
	printf("step. min_width:%u\t",step. min_width);
	printf("step.max_width:%u\t",step.max_width);
	printf("step.step_width:%u\t",step.step_width);
	printf("step.min_height:%u\t",step.min_height);
	printf("step.max_height:%u\t",step.max_height);
	printf("step.step_height:%u\n",step.step_height);/*all are frame parameters per pixel*/
	

	printf("From the struct struct v4l2_frmsizeenum  frmsizeenum: \n");
	printf("All are __u32\n");
	printf("frmsizeenum.index:%u\t",frmsizeenum.index);
	printf("frmsizeenum.pixel_format:%u\t",frmsizeenum.pixel_format);
	printf("frmsizeenum.type:%u\t",frmsizeenum.type);
	printf("frmsizeenum.type:%u\t",frmsizeenum.reserved[0]);
	printf("frmsizeenum.type:%u\n",frmsizeenum.reserved[1]);
	printf("Union containing struct v4l2_frmsize_discrete    discrete ,struct v4l2_frmsize_stepwise    stepwise\n");

	
	printf("struct v4l2_frmival_stepwise : \n");
	printf("contains struct v4l2_fract   min ,struct v4l2_fract   max ,struct v4l2_fract   step\n");
	
	
	
	
	printf("From thestruct v4l2_frmivalenum frmivalenum: \n");
	printf("All are __u32\n");
	printf("frmivalenum.index:%u\t",frmivalenum.index);
	printf("frmivalenum.pixel_format:%u\t",frmivalenum.pixel_format);
	printf("frmivalenum.height:%u\t",frmivalenum.height);
	printf("frmivalenum.type:%u\t",frmivalenum.type);
	printf("Union containing struct v4l2_fract     discrete ,struct v4l2_frmival_stepwise stepwise\n");	
	printf("frmivalenum.type:%u\t",frmivalenum.reserved[0]);
	printf("frmivalenum.type:%u\n",frmivalenum.reserved[1]);
	
	
	
	
	printf("From the struct v4l2_timecode time_code \n");
	printf("All are __u32\n");
	printf("time_code.type:%u\t",time_code.type);
	printf("time_code.flags:%u\t",time_code.flags);
	printf("All are __u8\n");
	printf("time_code.frames:%u\t",time_code.frames);
	printf("time_code.seconds:%u\t",time_code.seconds);
	printf("time_code.minutes:%u\t",time_code.minutes);
	printf("time_code.hours:%u\t",time_code.hours);
	printf("time_code.userbits:%u\t",time_code.userbits[0]);
	printf("time_code.userbits:%u\t",time_code.userbits[1]);
	printf("time_code.userbits:%u\t",time_code.userbits[2]);
	printf("time_code.userbits:%u\n",time_code.userbits[3]);
					
	
	
	
	
	
	
	printf("From the struct v4l2_jpegcompression jpegcompression \n");
	printf("All are int\n");
	printf("jpegcompression.quality:%d\t",jpegcompression.quality);
	printf("jpegcompression.APPns:%d\t",jpegcompression.APPn);
	printf("jpegcompression.APP_len:%d\t",jpegcompression.APP_len);
	printf("jpegcompression.APP_data:%s\t",jpegcompression.APP_data);
	printf("jpegcompression.COM_len:%d\t",jpegcompression.COM_len);
	printf("jpegcompression.COM_data:%s\t",jpegcompression.COM_data);
	printf("All are __u32\n");
	printf("jpegcompression.jpeg_markers:%u\n",jpegcompression.jpeg_markers);			

	

	printf("From the struct v4l2_requestbuffers rb \n");
	printf("All are __u32\n");
	printf("rb.count:%u\t",rb.count);
	printf("rb.type:%u\t",rb.type);
	printf("rb.memory:%u\t",rb.memory);
	printf("rb.reserved:%u\t",rb.reserved[0]);
	printf("rb.reserved:%u\t",rb.reserved[1]);


	struct v4l2_plane plane;
	printf("From the struct v4l2_plane plane \n");
	printf("All are __u32\n");
	printf("plane.bytesused:%u\t",plane.bytesused);
	printf("plane.length:%u\t",plane.length);
	printf("plane.m.offset:%u\t",plane.m.mem_offset);
	printf("plane.m.userptr:%lu\t",plane.m.userptr);
	printf("plane.data_offset:%u\n",plane.data_offset);
	printf("plane.reserved:%u\t",plane.reserved[0]);
	printf("plane.reserved:%u\t",plane.reserved[1]);
	printf("plane.reserved:%u\t",plane.reserved[2]);
	printf("plane.reserved:%u\t",plane.reserved[3]);
	printf("plane.reserved:%u\t",plane.reserved[4]);
	printf("plane.reserved:%u\t",plane.reserved[5]);
	printf("plane.reserved:%u\t",plane.reserved[6]);
	printf("plane.reserved:%u\t",plane.reserved[7]);
	printf("plane.reserved:%u\t",plane.reserved[8]);
	printf("plane.reserved:%u\t",plane.reserved[9]);
	printf("plane.reserved:%u\n",plane.reserved[10]);

	struct v4l2_framebuffer frbuf;
	printf("From the v4l2_framebuffer frbuf \n");
	printf("All are __u32\n");
	printf("frbuf.capability:%u\t",frbuf.capability);
	printf("frbuf.flags:%u\t",frbuf.flags);	
	printf("frbuf.base:%p\t",frbuf.base);



	return 0;
} 
/*akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ 

akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ make
gcc -Iinclude/linux    new_test.c   -o new_test
akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./new_test 
From the struct v4l2_buffer buf : 
Index : 896	Type : 896	Bytes used : 896	Flags : 896	Sequence : 896	Field : 896	Field : 256	buf.m.userptr : 0	buf.m.offset : 0	Userptr reserved : 0	Userptr reserved2 : 64	From the struct v4l2_fract  f: 

Numerator : 0	Denominator : 0
From the v4l2_struct ap :

From the struct struct v4l2_rect : 
All are __s32
left:-1112134176	top:32569	width:32569	height:17922	From the struct v4l2_pix_format : 
All are __u32
fmt.width:3922468944	fmt.height:32766	fmt.pixelformat:3182938027	fmt.bytesperline:0	fmt.field:32569	fmt.sizeimage:0	fmt.colorspace:0	fmt.priv:0
From the struct struct v4l2_frmsize_discrete frm_dicrete : 
All are __u32
frm_dicrete.width:0	frm_dicrete.height:0	From the struct v4l2_frmsize_stepwise step : 
All are __u32
step. min_width:0	step.max_width:0	step.step_width:3183112592	step.min_height:32569	step.max_height:4294967288	step.step_height:4294967295
From the struct struct v4l2_frmsizeenum  frmsizeenum: 
All are __u32
frmsizeenum.index:3923182120	frmsizeenum.pixel_format:32766	frmsizeenum.type:0	frmsizeenum.type:0	frmsizeenum.type:0
Union containing struct v4l2_frmsize_discrete    discrete ,struct v4l2_frmsize_stepwise    stepwise
struct v4l2_frmival_stepwise : 
contains struct v4l2_fract   min ,struct v4l2_fract   max ,struct v4l2_fract   step
From thestruct v4l2_frmivalenum frmivalenum: 
All are __u32
frmivalenum.index:1	frmivalenum.pixel_format:0	frmivalenum.height:1	frmivalenum.type:0	Union containing struct v4l2_fract     discrete ,struct v4l2_frmival_stepwise stepwise
frmivalenum.type:0	frmivalenum.type:0
From the struct v4l2_timecode time_code 
All are __u32
time_code.type:3183110632	time_code.flags:32569	All are __u8
time_code.frames:0	time_code.seconds:0	time_code.minutes:0	time_code.hours:0	time_code.userbits:0	time_code.userbits:0	time_code.userbits:0	time_code.userbits:0
From the struct v4l2_jpegcompression jpegcompression 
All are int
jpegcompression.quality:0	jpegcompression.APPns:0	jpegcompression.APP_len:0	jpegcompression.APP_data:	jpegcompression.COM_len:15775231	jpegcompression.COM_data:	All are __u32
jpegcompression.jpeg_markers:3022536896
From the struct v4l2_requestbuffers rb 
All are __u32
rb.count:0	rb.type:0	rb.memory:0	rb.reserved:0	rb.reserved:0	From the struct v4l2_plane plane 
All are __u32
plane.bytesused:0	plane.length:0	plane.m.offset:576	plane.m.userptr:3573412790848	plane.data_offset:896
plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896	plane.reserved:896
From the v4l2_framebuffer frbuf 
All are __u32
frbuf.capability:4	frbuf.flags:0	frbuf.base:0x1	akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ 

*/

