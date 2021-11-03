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
	struct v4l2_plane plane;
	struct v4l2_framebuffer frbuf;
	struct v4l2_clip clip;
	struct v4l2_window window;
	struct v4l2_captureparm capture;
	struct v4l2_outputparm output_parm;
	struct v4l2_cropcap crop;
	struct v4l2_selection select;
		
	
	
	
	
	
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

	

	printf("From the v4l2_framebuffer frbuf \n");
	printf("All are __u32\n");
	printf("frbuf.capability:%u\t",frbuf.capability);
	printf("frbuf.flags:%u\t",frbuf.flags);	
	printf("frbuf.base:%p\t",frbuf.base);/*void *basestruct v4l2_pix_format  fmt;*/



	printf("From the struct v4l2_clip clip :  \n");
	printf("Struct v4l2_rect and a self ref pointer\n");	

	
	printf("From the struct v4l2_window window \n");
	printf("Struct v4l2_rect w and void __user bitmap\n");	
	printf("All are __u32\n");
	printf("window.field :%u\t",window.field);
	printf("window.chromakey :%u\t",window.chromakey);
	printf("window.clipcount :%u\t",window.clipcount);
	printf("All are __u8\n");
	printf("window.global_alpha :%u\t",window.global_alpha);
	


	printf("All are __u32\n");
	printf("capture.capability :%u\t",capture.capability);
	printf("capture.capturemode :%u\t",capture.capturemode);
	printf("capture.extendedmode :%u\t",capture.extendedmode);
	printf("capture.readbuffers :%u\t",capture.readbuffers);
	printf("capture.reserved :%u\t",capture.reserved[0]);
	printf("capture.reserved :%u\t",capture.reserved[1]);
	printf("capture.reserved :%u\t",capture.reserved[2]);
	printf("capture.reserved :%u\n",capture.reserved[3]);
	printf("struct v4l2_fract timeperframe\n");
		
	
	
	
	
	printf("From the struct v4l2_outputparm output_parm\n");
	printf("All are __u32\n");
	printf("output_parm.:%u\t",output_parm.capability);
	printf("output_parm.:%u\t",output_parm.outputmode);
	printf("output_parm.:%u\t",output_parm.extendedmode);
	printf("output_parm.:%u\n",output_parm.writebuffers);
	printf("output_parm.reserved:%u\t",output_parm.reserved[0]);
	printf("output_parm.reserved:%u\t",output_parm.reserved[1]);
	printf("output_parm.reserved:%u\t",output_parm.reserved[2]);
	printf("output_parm.reserved:%u\n",output_parm.reserved[3]);
	printf("     struct v4l2_fract  timeperframe\n");
	
	
	
	
	

	
	printf("From the struct v4l2_outputparm output_parm\n");
	printf("All are __u32\n");
	printf("crop.type.:%u\t",crop.type);
	printf("v4l2_rect bounds,v4l2_rext defrect,v4l2_pixelaspect\n");
	
	
	
	printf("From the struct v4l2_selection select\n");
	printf("select.type:%u\t",select.type);
	printf("select.target:%u\t",select.target);	
	printf("select.flags:%u\t",select.flags);
	printf("select.reserved:%u\t",select.reserved[0]);
	printf("select.reserved:%u\t",select.reserved[1]);
	printf("select.reserved:%u\t",select.reserved[2]);
	printf("select.reserved:%u\t",select.reserved[3]);
	printf("select.reserved:%u\t",select.reserved[4]);
	printf("select.reserved:%u\t",select.reserved[5]);
	printf("select.reserved:%u\t",select.reserved[6]);
	printf("select.reserved:%u\t",select.reserved[7]);
	printf("select.reserved:%u\t",select.reserved[8]);
	printf("v4l2_rect r\n");
	


	return 0;
} 
/*From the struct v4l2_buffer buf : 
Index : 896	Type : 896	Bytes used : 896	Flags : 896	Sequence : 896	Field : 896	Field : 256	buf.m.userptr : 0	buf.m.offset : 0	Userptr reserved : 0	Userptr reserved2 : 64	From the struct v4l2_fract  f: 

Numerator : 1196521273	Denominator : 32545
From the v4l2_struct ap :

From the struct struct v4l2_rect : 
All are __s32
left:795370944	top:32765	width:32765	height:0	From the struct v4l2_pix_format : 
All are __u32
fmt.width:795370896	fmt.height:32765	fmt.pixelformat:0	fmt.bytesperline:0	fmt.field:0	fmt.sizeimage:0fmt.colorspace:0	fmt.priv:0
From the struct struct v4l2_frmsize_discrete frm_dicrete : 
All are __u32
frm_dicrete.width:1196339120	frm_dicrete.height:32545	From the struct v4l2_frmsize_stepwise step : 
All are __u32
step. min_width:7	step.max_width:8	step.step_width:1196336512	step.min_height:32545	step.max_height:1196526056	step.step_height:32545
From the struct struct v4l2_frmsizeenum  frmsizeenum: 
All are __u32
frmsizeenum.index:1196527968	frmsizeenum.pixel_format:32545	frmsizeenum.type:795371009	frmsizeenum.type:0	frmsizeenum.type:0
Union containing struct v4l2_frmsize_discrete    discrete ,struct v4l2_frmsize_stepwise    stepwise
struct v4l2_frmival_stepwise : 
contains struct v4l2_fract   min ,struct v4l2_fract   max ,struct v4l2_fract   step
From thestruct v4l2_frmivalenum frmivalenum: 
All are __u32
frmivalenum.index:0	frmivalenum.pixel_format:0	frmivalenum.height:0	frmivalenum.type:796398120	Union containing struct v4l2_fract     discrete ,struct v4l2_frmival_stepwise stepwise
frmivalenum.type:0	frmivalenum.type:0
From the struct v4l2_timecode time_code 
All are __u32
time_code.type:0	time_code.flags:0	All are __u8
time_code.frames:0	time_code.seconds:0	time_code.minutes:0	time_code.hours:0	time_code.userbits:0	time_code.userbits:0	time_code.userbits:0	time_code.userbits:0
From the struct v4l2_jpegcompression jpegcompression 
All are int
jpegcompression.quality:0	jpegcompression.APPns:0	jpegcompression.APP_len:0	jpegcompression.APP_data:	jpegcompression.COM_len:15775231	jpegcompression.COM_data:	All are __u32
jpegcompression.jpeg_markers:2234187968
From the struct v4l2_requestbuffers rb 
All are __u32
rb.count:1196248544	rb.type:32545	rb.memory:862452112	rb.reserved:0	rb.reserved:0	From the struct v4l2_plane plane 
All are __u32
plane.bytesused:1	plane.length:0	plane.m.offset:61765110	plane.m.userptr:4356732406	plane.data_offset:0
plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0	plane.reserved:0
From the v4l2_framebuffer frbuf 
All are __u32
frbuf.capability:795370896	frbuf.flags:32765	frbuf.base:0x7f21474ea000	From the struct v4l2_clip clip :  
Struct v4l2_rect and a self ref pointer
From the struct v4l2_window window 
Struct v4l2_rect w and void __user bitmap
All are __u32
window.field :1196337920	window.chromakey :32545	window.clipcount :42	All are __u8
window.global_alpha :0	All are __u32
capture.capability :4294967288	capture.capturemode :4294967295	capture.extendedmode :795371456	capture.readbuffers :32765	capture.reserved :1196353451	capture.reserved :32545	capture.reserved :0	capture.reserved :0
struct v4l2_fract timeperframe
From the struct v4l2_outputparm output_parm
All are __u32
output_parm.:2060782804	output_parm.:4294945446	output_parm.:4	output_parm.:0
output_parm.reserved:1	output_parm.reserved:0	output_parm.reserved:1196529408	output_parm.reserved:32545
     struct v4l2_fract  timeperframe
From the struct v4l2_outputparm output_parm
All are __u32
crop.type.:9	v4l2_rect bounds,v4l2_rext defrect,v4l2_pixelaspect
From the struct v4l2_selection select
select.type:0	select.target:0	select.flags:576	select.reserved:896	select.reserved:896	select.reserved:896	select.reserved:896	select.reserved:896	select.reserved:896	select.reserved:896	select.reserved:896	select.reserved:896	v4l2_rect r


*/

