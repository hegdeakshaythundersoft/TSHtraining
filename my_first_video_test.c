#include "header.h" 

typedef struct video_device{

	int fd;


}device;

device my_video_device;

//static int video_device_open(device*,const char*,int);
//static int video_device_close(device*);
static int video_device_open(device *dev,const char* name,int no_request){
	dev->fd=open(name,O_RDWR);
	if((dev->fd)>=0)
	printf("\nVideo opened successfully\n");
	else
	printf("Can't open the video");


}

static int video_device_close(device *dev){

	close(dev->fd);


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

akshay@akshay-ThinkPad-L570-W10DG:~/video_module/akshay$ ./run_1.sh 


Calling the open function

Video opened successfully

SUCCESSFULLY OPENED

Closed the sesion



*/





