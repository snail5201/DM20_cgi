/*
** system
** time   : 2018-10-23
** author : chenks (godence@126.com)


** 2019-02-21:
		删除无用的函数
		修改 DM_Infrared_Stream_Register 函数传入用户数据


*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "dmsdk.h"


#define CHECK(cond, txt)\
    do{\
        if(!(cond)){\
            printf("[Func]:%s [Line]:%d  %s\n", __FUNCTION__, __LINE__, txt);\
            return;\
        }\
    }while(0);


#define CHECK_RET(cond, ret, txt)\
    do{\
        if(!(cond)){\
            printf("[Func]:%s [Line]:%d  %s\n", __FUNCTION__, __LINE__, txt);\
            return (ret);\
        }\
    }while(0);



typedef void (*FUNC)(long hdl, int argc, const char *argv[]);

typedef struct _mycmd{
    char *cmd;
    FUNC fun;
    char *help;
}MYCMD;

typedef struct _userMessage{
	int notifi_cb;
	int data_cb;
}USER_MESSAGE;


static void usage(const char *exe)
{
    printf(" Usage: %s ip item [param]\n", exe);
    printf("-----------------------------------\n");
	printf("rtsp [port]\n");
	printf("raw  [port] [framerate]\n");
    printf("\n");
}

static int rtsp_notify_cb(int event, void * puser)
{
	if (event == STREAM_EVE_CONNECTING)
	{
		printf("Connecting\n");
	}
	else if (event == STREAM_EVE_CONNFAIL)
	{
		printf("Connect failed\n");
	}
	else if (event == STREAM_EVE_CONNSUCC)
	{
		printf("Connect success\n");
	}
	else if (event == STREAM_EVE_NODATA)
	{
		printf("NO Data\n");
	}
	else if (event == STREAM_EVE_RESUME)
	{
		printf("Resume\n");
	}
	else if (event == STREAM_EVE_STOPPED)
	{
		printf("Disconnect\n");
	}

    return 0;
}



static int rtsp_video_cb(unsigned char* pdata, int len, unsigned int ts, unsigned short seq, void *puser)
{
	printf("V: data=0x%lX, len=%d\n", (long)pdata, len);

	return 0;
}

static int raw_data_cb(long hdl, unsigned char* pdata, int len, int width, int height, void *puser)
{

	USER_MESSAGE *p = (USER_MESSAGE *) puser;

	printf("D: data=0x%lX, len=%d, w=%d, h=%d -- %#X\n", (long)pdata, len, width, height, p->data_cb);
/*
	FILE *fp;
	fp = fopen("save", "ab+");
	fwrite(pdata,len,1,fp);
	fclose(fp);
*/
	return 0;
}

static int raw_notify_cb(long hdl, int event, void * puser)
{

	USER_MESSAGE *p = (USER_MESSAGE *) puser;

	printf("notify -- %#X", p->notifi_cb);

	if (event == STREAM_EVE_CONNECTING)
	{
		printf("Connecting\n");
	}
	else if (event == STREAM_EVE_CONNFAIL)
	{
		printf("Connect failed\n");
	}
	else if (event == STREAM_EVE_CONNSUCC)
	{
		printf("Connect success\n");
	}
	else if (event == STREAM_EVE_STOPPED)
	{
		printf("Disconnect\n");
	}
	else if (event == STREAM_EVE_CONNERR)
	{
		printf("Connection has error\n");
	}

    return 0;
}



static void do_blk_rtsp(long hdl, int argc, const char *argv[])
{
    int port;

	port = 554;

    if(argc >= 5){
		port = atoi(argv[4]);
    }

    DM_Video_Stream_Register(hdl, rtsp_notify_cb, rtsp_video_cb);
	DM_Video_Stream_Start(hdl, port);		//此函数执行完后调用rtsp_notify_cb()函数，之后
											//调用rtsp_video_cb()函数开始输出值
	sleep(1);
	printf("input any key for exit !!!\n");	//一秒后，输出9次后打印该信息
	getchar();		//等待输出一个字符

	DM_Video_Stream_Stop(hdl);
}

static void do_blk_raw(long hdl, int argc, const char *argv[])
{
    int port;
	int framerate;
	USER_MESSAGE *pstUser;

	pstUser = (USER_MESSAGE *) malloc(sizeof (USER_MESSAGE));

	pstUser->notifi_cb = 0x12345678;
	pstUser->data_cb = 0x87654321;

	port = 5000;
	framerate = 25;

    if(argc >= 6){
		port = atoi(argv[4]);
    }

    if(argc >= 6){
		framerate = atoi(argv[5]);
    }

	DM_Infrared_Stream_Register(hdl, raw_notify_cb, raw_data_cb, (void *)pstUser);
	DM_Infrared_Stream_Start(hdl, port, framerate);

	sleep(1);
	printf("input any key for exit !!!\n");
	getchar();

	DM_Infrared_Stream_Stop(hdl);
}


static MYCMD blk_cmd_table[] =
{
    {"rtsp",             do_blk_rtsp,             "rtsp"},
	{"raw",              do_blk_raw,              "raw"},
};

static void fun_blk(long hdl, int argc, const char *argv[])
{
    int i;

    for(i=0; i<sizeof(blk_cmd_table)/sizeof(blk_cmd_table[0]); i++){
        if(strcmp(argv[2], blk_cmd_table[i].cmd) == 0){
            blk_cmd_table[i].fun(hdl, argc, argv);
            return;
        }
    }

    printf("[%s] not support\n", argv[2]);
	usage(argv[0]);
}

int main(int argc, const char *argv[])
{
    int ret;
    long hdl;
    const char *ip;

    if(argc < 3){
        usage(argv[0]);		//项目用法
        return 0;
    }

    DM_VERSION ver;

    DM_Version(&ver);		//获取dm版本
    printf("%s\n", ver.version);

	DM_Sys_Init();		//创建并初始化系统所需的链表

    DM_LOG log;

    memset((char*)&log, 0x00, sizeof(log));
    log.level[0] = LOG_LEVEL_DEFAULT;
    log.roll = 7;
    snprintf(log.path, sizeof(log.path), "./log");	//初始化logo路径

    DM_Log_Init(&log);

    ip = argv[1];

    ret = DM_Open(&hdl, ip, 80, "admin", "Admin123");	//hdl参数获取config的数据
    if(ret != DM_SUCCESS){
        printf("DM_Open failed, ret=%d\n", ret);
        return 0;
    }

    fun_blk(hdl, argc, argv);

    DM_Close(hdl);

    DM_Log_Exit();

	DM_Sys_UnInit();

	return 0;
}



