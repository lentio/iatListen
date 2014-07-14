// iatdemo.c : Defines the entry point for the console application.
//

#include "stdlib.h"
#include "stdio.h"
#include <windows.h>
#include <conio.h>
#include <errno.h>

#include "../iflyLib/include/qisr.h"
#include "../iflyLib/include/msp_cmn.h"
#include "../iflyLib/include/msp_errors.h"


#ifdef _WIN64
#pragma comment(lib,"../iflyLib/lib/msc_x64.lib")//x64
#else
#pragma comment(lib,"../iflyLib/lib/msc.lib")//x86
#endif

void run_iat(const char* src_wav_filename ,  const char* param)
{
	char rec_result[1024] = {0};
	const char *sessionID = NULL;
	FILE *f_pcm = NULL;
	char *pPCM = NULL;
	int lastAudio = 0 ;
	int audStat = MSP_AUDIO_SAMPLE_CONTINUE ;
	int epStatus = MSP_EP_LOOKING_FOR_SPEECH;
	int recStatus = MSP_REC_STATUS_SUCCESS ;
	long pcmCount = 0;
	long pcmSize = 0;
	int errCode = 10 ;
	
	sessionID = QISRSessionBegin(NULL, param, &errCode);//��ʼһ·�Ự
	f_pcm = fopen(src_wav_filename, "rb");
	if (NULL != f_pcm) {
		fseek(f_pcm, 0, SEEK_END);
		pcmSize = ftell(f_pcm);
		fseek(f_pcm, 0, SEEK_SET);
		pPCM = (char *)malloc(pcmSize);
		fread((void *)pPCM, pcmSize, 1, f_pcm);
		fclose(f_pcm);
		f_pcm = NULL;
	}//��ȡ��Ƶ�ļ�
	while (1) {
		unsigned int len = 6400;
		int ret = 0;
		if (pcmSize < 12800) {
			len = pcmSize;
			lastAudio = 1;//��Ƶ����С��12800
		}
		audStat = MSP_AUDIO_SAMPLE_CONTINUE;//�к����Ƶ
		if (pcmCount == 0)
			audStat = MSP_AUDIO_SAMPLE_FIRST;
		if (len<=0)
		{
			break;
		}
		printf("csid=%s,count=%d,aus=%d,",sessionID,pcmCount/len,audStat);
		ret = QISRAudioWrite(sessionID, (const void *)&pPCM[pcmCount], len, audStat, &epStatus, &recStatus);//д��Ƶ
		printf("eps=%d,rss=%d,ret=%d\n",epStatus,recStatus,errCode);
		if (ret != 0)
		break;
		pcmCount += (long)len;
		pcmSize -= (long)len;
		if (recStatus == MSP_REC_STATUS_SUCCESS) {
			const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &errCode);//������Ѿ���ʶ���������Ի�ȡ
			if (NULL != rslt)
				strcat(rec_result,rslt);
		}
		if (epStatus == MSP_EP_AFTER_SPEECH)
			break;
		_sleep(150);//ģ����˵��ʱ���϶
	}
	QISRAudioWrite(sessionID, (const void *)NULL, 0, MSP_AUDIO_SAMPLE_LAST, &epStatus, &recStatus);
	free(pPCM);
	pPCM = NULL;
	while (recStatus != MSP_REC_STATUS_COMPLETE && 0 == errCode) {
		const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &errCode);//��ȡ���
		if (NULL != rslt)
		{
			strcat(rec_result,rslt);
		}
		_sleep(150);
	}
	QISRSessionEnd(sessionID, NULL);
	printf("=============================================================\n");
	printf("The result is: %s\n",rec_result);
	printf("=============================================================\n");
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: iatdeamo <wave_file>");
        printf("δָ������wave�ļ���");
        return -1;
    }

    ///APPID��������Ķ�
	const char* login_configs = "appid = 538849f9, work_dir =   .  ";//��¼����  test:538849f9, user:538293af
	const char* param1 = "sub=iat,auf=audio/L16;rate=16000,aue=speex-wb,ent=sms16k,rst=plain,rse=gb2312,vad_speech_tail=3000";//�ɲο������ò����б�
	const char* param2 = "sub=iat,auf=audio/L16;rate=16000,aue=speex-wb,ent=sms16k,rst=json,rse=utf8";//תдΪjson��ʽ������ֻ��Ϊutf8
	int ret = 0;
	char key = 0;

	//�û���¼
	ret = MSPLogin(NULL, NULL, login_configs);//��һ������Ϊ�û������ڶ�������Ϊ���룬�����������ǵ�¼�������û�����������Ҫ��http://open.voicecloud.cnע�Ტ��ȡappid
	if ( ret != MSP_SUCCESS )
	{
		printf("MSPLogin failed , Error code %d.\n",ret);
		goto exit ;
	}
	//��ʼһ·תд�Ự
    run_iat(argv[1], param1);
	// run_iat("wav/iflytek04.wav" ,  param1);                                     //iflytek09��Ӧ����Ƶ���ݡ����۲���ǧ����������ǰͷ��ľ������
	// run_iat("wav/iflytek01.wav" ,  param2);                                     //iflytek01��Ӧ����Ƶ���ݡ��ƴ�Ѷ�ɡ�

	exit:
	MSPLogout();//�˳���¼
	key = _getch();
	return 0;
}

