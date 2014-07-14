#include "stdafx.h"
#include "TestIat.h"
#include "Utility.h"

#include "../iflyLib/include/qisr.h"
#include "../iflyLib/include/msp_cmn.h"
#include "../iflyLib/include/msp_errors.h"

#pragma comment(lib, "Winmm.lib")          //x86
#pragma comment(lib, "../iflyLib/lib/msc.lib")   //x86

#define IATLISTEN_MAX_AUDIO_SIZE      10*1024*1024  // max audio size 10 M

// ifly error string
#define MAX_STRING_SIZE     32
char g_iflyRecStatus[][MAX_STRING_SIZE] =
{
    "识别成功",
    "识别结束，没有识别结果",
    "正在识别中",
    "保留",
    "发现有效音频",
    "识别结束",
    "保留",
    "保留",
    "保留",
    "保留"
};

char g_iflyEpStatus[][MAX_STRING_SIZE] =
{
    "还没有检测到音频的前端点",
    "正在进行音频处理",
    "NULL",
    "检测到音频的后端点",
    "超时",
    "出现错误",
    "音频过大"
};

#define IL_MSP_ERR_CODE_DICT   0
#define IL_WAVE_ERR_CODE_DICT  1

CTestIat::CTestIat(string szMetaPathFile, string szOutputPathFile)
{
    m_bInit = false;
    m_bStart = false;
    m_sessionID = NULL;
    m_pWaveData = NULL;
    m_nWaveDataSize = 0;

    m_szMetaPathFile = szMetaPathFile;
    m_szOutputPathFile = szOutputPathFile;

    initErrCode();
}

CTestIat::~CTestIat()
{
    m_bInit = false;
    m_bStart = false;
    m_sessionID = NULL;
    m_nWaveDataSize = 0;

    if (m_pWaveData){
        free(m_pWaveData);
        m_pWaveData = NULL;
    }

    Fini();
}

bool CTestIat::Init()
{
    if (!iflyInit()){
        //cout << "[CTestIat::Init] fail to iflyInit" << endl;
        cout << "无法连接到讯飞语音平台，请检查网络连接" << endl;
        return false;
    }

    if (!waveInit()){
        //cout << "[CTestIat::Init] fail to waveInit" << endl;
        cout << "无法打开本地音频输入设备" << endl;

        iflyFini();
        return false;
    }

    if (!m_pWaveData){
        m_pWaveData = (char *)malloc(IATLISTEN_MAX_AUDIO_SIZE);
        assert(m_pWaveData);

        m_nWaveDataSize = 0;
        memset(m_pWaveData, 0, IATLISTEN_MAX_AUDIO_SIZE);
    }
    else{
        assert(0);
    }

    m_bInit = true;
    return true;
}

void CTestIat::Fini()
{
    iflyFini();
    waveFini();
    m_bInit = false;
}

bool CTestIat::LoopKey()
{
    if (!m_bInit){
        cout << "对象未初始化" << endl;
        return false;
    }
        
    cout << "按S键开始语音检测，按Q键离开..." << endl;
    char key = 0;
    while (1){
        // 如果有键盘输入
        if (_kbhit()) {
            key = _getch();
            cout << "press key： " << key << endl;
            if (key == 's'){ // Start iat
                StartAudio();
            }
            else if (key == 'q'){ // eXit
                m_bStart = false;
                cout << "退出循环检测" << endl;
                break;
            }
        }

        if (m_bStopAudio){
            cout << "[CTestIat::Loop] stop audio" << endl;
            StopAudio();
        }

        ::Sleep(1000); // delay 1s
    }

    return true;
}

bool CTestIat::LoopFile()
{
    if (!m_bInit){
        cout << "对象未初始化" << endl;
        return false;
    }

    cout << "读取文件[" << m_szMetaPathFile << "]获取指令，按Q键退出..." << endl;
    //initMeta();
    char key = 0;
    while (1){
        // 如果有键盘输入
        if (_kbhit()) {
            key = _getch();
            if (key == 'q'){ // eXit
                m_bStart = false;
                cout << "用户已退出程序" << endl;
                break;
            }
        }

        checkCmdFile();
        if (m_bStopAudio){
            // cout << "[CTestIat::LoopFile] stop detect audio" << endl;
            StopAudio();

            // update meta and text file
            if (saveText(m_result)){
                cout << "[CTestIat::LoopFile]结果输出文件: " << m_szOutputPathFile << endl;
                if (setMeta(CMD_STATUS_DONE)){
                    cout << "[CTestIat::LoopFile] update cmd status: done!" << endl;
                }
            }
        }

        ::Sleep(1000); // delay 1s
    }

    //initMeta();
    return true;
}

bool CTestIat::checkCmdFile()
{
    int nCmd;
    if (getMeta(nCmd)){
        if (nCmd == CMD_STATUS_READY){
            cout << "[CTestIat::checkCmdFile] get cmd: ready!" << endl;
            if (StartAudio()){
                if (setMeta(CMD_STATUS_RUN)){
                    cout << "[CTestIat::checkCmdFile] update cmd status: run!" << endl;
                }
            }
        }
        else if (nCmd == CMD_STATUS_STOP){
            //cout << "[CTestIat::checkCmdFile] get cmd: stop!" << endl;
            StopAudio();
        }
    }

    return true;
}

bool CTestIat::StartAudio()
{
    if (m_bStart){
        return false;
    }

    if (!iflyStartSession()){
        return false;
    }

    m_bStart = true;
    m_bFirst = true;
    m_bStopAudio = false;
    memset(m_result, 0, sizeof(m_result));

    // init dump data
    m_nWaveDataSize = 0;
    memset(m_pWaveData, 0, IATLISTEN_MAX_AUDIO_SIZE);

    //将两个wHdr添加到waveIn中去  
    //waveInAddBuffer(m_hWaveIn, &m_wHdr1, sizeof (WAVEHDR));
    //waveInAddBuffer(m_hWaveIn, &m_wHdr2, sizeof (WAVEHDR));
    for (int i = 0; i < IAT_MAX_AUDIO_BUFFERS; ++i){
        waveInAddBuffer(m_hWaveIn, &m_wHdr[i], sizeof (WAVEHDR));
    }

    //开始音频采集  
    waveInStart(m_hWaveIn);
    cout << "开始语音听写..." << endl;
    return true;
}

void CTestIat::StopAudio()
{
    if (m_bStart){
        m_bStart = false;
        //memset(m_result, 0, sizeof(m_result));
        iflyStopSession();

        //停止音频采集  
        waveInStop(m_hWaveIn);

        // waveFini();
        //closeDumpFile();
        m_bStopAudio = false;
        cout << "语音听写结束！" << endl;

        dumpWaveFile();
    }
}

bool CTestIat::waveInit()
{
    WAVEFORMATEX waveform; //采集音频的格式，结构体  
    waveform.wFormatTag = WAVE_FORMAT_PCM;//声音格式为PCM  
    waveform.nSamplesPerSec = 16000;//采样率，16000次/秒  
    waveform.wBitsPerSample = 16;//采样比特，16bits/次  
    waveform.nChannels = 1;//采样声道数，单声道  
    waveform.nAvgBytesPerSec = 16000 * 2;//每秒的数据率，就是每秒能采集多少字节的数据  
    waveform.nBlockAlign = 2;//一个块的大小，采样bit的字节数乘以声道数  
    waveform.cbSize = 0;//一般为0  

    //使用waveInOpen函数开启音频采集  
    MMRESULT mmr = waveInOpen(&m_hWaveIn, WAVE_MAPPER, &waveform,
        (DWORD)(MicCallback), DWORD(this), CALLBACK_FUNCTION);
    if (mmr != MMSYSERR_NOERROR){
        cout << "[CTestIat::waveInit] fail to waveInOpen， error(" << mmr << "): " << CUtility::getErrDict(IL_WAVE_ERR_CODE_DICT, mmr) << endl;
        return false;
    }

    ////建立两个数组（这里可以建立多个数组）用来缓冲音频数据  
    //BYTE *pBuffer1, *pBuffer2;//采集音频时的数据缓存  
    //DWORD bufsize = 8 * 1024;

    //pBuffer1 = new BYTE[bufsize];
    //m_wHdr1.lpData = (LPSTR)pBuffer1;
    //m_wHdr1.dwBufferLength = bufsize;
    //m_wHdr1.dwBytesRecorded = 0;
    //m_wHdr1.dwUser = 0;
    //m_wHdr1.dwFlags = 0;
    //m_wHdr1.dwLoops = 1;
    //m_wHdr1.lpNext = NULL;
    //m_wHdr1.reserved = 0;

    ////将建立好的m_wHdr1做为备用  
    //waveInPrepareHeader(m_hWaveIn, &m_wHdr1, sizeof(WAVEHDR));

    //pBuffer2 = new BYTE[bufsize];
    //m_wHdr2.lpData = (LPSTR)pBuffer2;
    //m_wHdr2.dwBufferLength = bufsize;
    //m_wHdr2.dwBytesRecorded = 0;
    //m_wHdr2.dwUser = 0;
    //m_wHdr2.dwFlags = 0;
    //m_wHdr2.dwLoops = 1;
    //m_wHdr2.lpNext = NULL;
    //m_wHdr2.reserved = 0;

    ////将建立好的m_wHdr2做为备用  
    //waveInPrepareHeader(m_hWaveIn, &m_wHdr2, sizeof(WAVEHDR));

    for (int i = 0; i < IAT_MAX_AUDIO_BUFFERS; ++i){
        BYTE *pBuffer1;//采集音频时的数据缓存  
        DWORD bufsize = 8 * 1024;

        pBuffer1 = new BYTE[bufsize];
        m_wHdr[i].lpData = (LPSTR)pBuffer1;
        m_wHdr[i].dwBufferLength = bufsize;
        m_wHdr[i].dwBytesRecorded = 0;
        m_wHdr[i].dwUser = 0;
        m_wHdr[i].dwFlags = 0;
        m_wHdr[i].dwLoops = 1;
        m_wHdr[i].lpNext = NULL;
        m_wHdr[i].reserved = 0;

        //将建立好的m_wHdr1做为备用  
        waveInPrepareHeader(m_hWaveIn, &m_wHdr[i], sizeof(WAVEHDR));
    }

    return true;
}

void CTestIat::waveFini()
{
    m_bStart = false;
    waveInStop(m_hWaveIn);
    waveInReset(m_hWaveIn);
    //waveInUnprepareHeader(m_hWaveIn, &m_wHdr1, sizeof(WAVEHDR));
    //waveInUnprepareHeader(m_hWaveIn, &m_wHdr2, sizeof(WAVEHDR));
    for (int i = 0; i < IAT_MAX_AUDIO_BUFFERS; ++i){
        waveInUnprepareHeader(m_hWaveIn, &m_wHdr[i], sizeof(WAVEHDR));
    }

    waveInClose(m_hWaveIn);
    m_hWaveIn = NULL;
}

void CTestIat::dumpData(char *pPCM, int pcmSize)
{
    if (!m_pWaveData){
        cout << "[CTestIat::dumpData] fail to dump wave, data_ptr: NULL" << endl;
        return;
    }
    
    if ((m_nWaveDataSize + pcmSize) > IATLISTEN_MAX_AUDIO_SIZE){
        cout << "[CTestIat::dumpData] fail to dump wave, too many data, current:" << m_nWaveDataSize;
        cout << ", next:" << pcmSize << ", limit:" << IATLISTEN_MAX_AUDIO_SIZE << endl;
    }

    memcpy(m_pWaveData + m_nWaveDataSize, pPCM, pcmSize);
    m_nWaveDataSize += pcmSize;
}

void CTestIat::dumpWaveHead(int nWaveSize)
{
    /*------------------------Wave File Structure ------------------------------------ */
    typedef struct RIFF_CHUNK{
        char fccID[4]; // must be "RIFF"
        unsigned long dwSize; // all bytes of the wave file subtracting 8,
        // which is the size of fccID and dwSize
        char fccType[4]; // must be "WAVE"
    }WAVE_HEADER;

    // 12 bytes
    typedef struct FORMAT_CHUNK{
        char fccID[4]; // must be "fmt "
        unsigned long dwSize; // size of this struct, subtracting 8, which
        // is the sizeof fccID and dwSize
        unsigned short wFormatTag; // one of these: 1: linear,6: a law,7:u-law
        unsigned short wChannels; // channel number
        unsigned long dwSamplesPerSec; // sampling rate
        unsigned long dwAvgBytesPerSec; // bytes number per second
        unsigned short wBlockAlign; // 每样本的数据位数(按字节算), 其值为:通道
        // 数*每样本的数据位值/8，播放软件需要一次处
        // 理多个该值大小的字节数据, 以便将其值用于
        // 缓冲区的调整每样本占几个字节:
        // NumChannels * uiBitsPerSample/8
        unsigned short uiBitsPerSample; // quantization
    }FORMAT;

    // 12 bytes
    // 数据结构
    typedef struct {
        char fccID[4]; // must be "data"
        unsigned long dwSize; // byte_number of PCM data in byte
    }DATA;

    // 24 bytes
    // The fact chunk is required for all new WAVE formats.
    // and is not required for the standard WAVE_FORMAT_PCM files
    // 也就是说，这个结构体目前不是必须的，一般当wav文件由某些软件转化而成，则包含该Chunk
    // 但如果这里写了，则必须是如下的结构，并且在四个结构体中的位置也要放在第三
    typedef struct {
        char fccID[4]; // must be "fact"
        unsigned long id; // must be 0x4
        unsigned long dwSize; // 暂时没发现有啥用
    }FACT;

    WAVE_HEADER WaveHeader;
    FORMAT WaveFMT;
    DATA WaveData;
    FACT WaveFact;
    memset(&WaveHeader, 0, sizeof(WAVE_HEADER));
    memcpy(WaveHeader.fccID, "RIFF", 4);
    memcpy(WaveHeader.fccType, "WAVE", 4);

    // dwSize 是整个wave文件的大小（字节数，但不包括不包括HEADER中的前面两个结构:
    // HEADER.fccID和HEAD.dwSize)
    // WaveHeader.dwSize = length + 0x24; // 如果不写入fact，就是36个字节，
    // 44- 8 = 36个

    WaveHeader.dwSize = nWaveSize + 0x30; // 如果写入fact，就是48 个bytes
    memset(&WaveFMT, 0, sizeof(FORMAT));
    memcpy(WaveFMT.fccID, "fmt ", 4);
    WaveFMT.dwSize = 0x10;
    WaveFMT.dwSamplesPerSec = 16000;
    WaveFMT.dwAvgBytesPerSec = 1 * 16000 * 2;
    WaveFMT.wChannels = 1;
    WaveFMT.uiBitsPerSample = 16;
    WaveFMT.wFormatTag = WAVE_FORMAT_PCM;
    WaveFMT.wBlockAlign = 2;
    memset(&WaveFact, 0, sizeof(FACT));
    memcpy(WaveFact.fccID, "fact", 4);
    WaveFact.dwSize = nWaveSize; // 这个值不知道什么意思
    WaveFact.id = 0x4;
    memset(&WaveData, 0, sizeof(DATA));
    memcpy(WaveData.fccID, "data", 4);
    WaveData.dwSize = nWaveSize;
    
    m_sfile.write((char *)&WaveHeader, sizeof(WAVE_HEADER));
    m_sfile.write((char *)&WaveFMT, sizeof(FORMAT));
    m_sfile.write((char *)&WaveFact, sizeof(FACT)); // fact不是必须的
    m_sfile.write((char *)&WaveData, sizeof(DATA));
}

void CTestIat::dumpWaveFile()
{
    string szOutPath, szName, szDumpFile;
    if (CUtility::getPathFile(m_szOutputPathFile, szOutPath, szName)){
        szDumpFile = szOutPath + "/" + CUtility::getNowTime() + ".wav";
    }
    else{
        szDumpFile = CUtility::getNowTime() + ".wav"; // current path
    }

    m_sfile.open(szDumpFile, ios::out | ios::binary | ios::trunc);
    assert(m_sfile.is_open());

    // add wave header
    if (m_pWaveData && m_nWaveDataSize > 0){
        dumpWaveHead(m_nWaveDataSize);
        m_sfile.write(m_pWaveData, m_nWaveDataSize);
    }

    m_sfile.close();
    cout << "保存语音文件:" << szDumpFile << endl;
}

//下面这个是callback函数，对于采集到的音频数据都在这个函数中处理  
DWORD CALLBACK CTestIat::MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    //这个CIMAADPCMDlg就是你的音频采集类  
    CTestIat *pThis = (CTestIat*)dwInstance;
    if (pThis->m_bStopAudio){
        return -1;
    }

    switch (uMsg)
    {
    case WIM_OPEN:
        printf("WIM_OPEN\n");
        break;

    case WIM_DATA:
        // TRACE("WIM_DATA\n");
        //printf("[CTestIat::MicCallback]waveInProc audio size: %d\n", ((WAVEHDR*)dwParam1)->dwBytesRecorded);

        //这里就是对采集到的数据做处理的地方，我是做了发送处理  
        //((PWAVEHDR)dwParam1)->lpData这就是采集到的数据指针  
        //((PWAVEHDR)dwParam1)->dwBytesRecorded这就是采集到的数据长度  
        if (!pThis->iflySendSession(((PWAVEHDR)dwParam1)->lpData, ((PWAVEHDR)dwParam1)->dwBytesRecorded)){
            //cout << "[CTestIat::MicCallback] need to stop audio sesion" << endl;
            pThis->m_bStopAudio = true;
            return -1;
        }

        //::Sleep(100);

        //处理完了之后还要再把这个缓冲数组加回去  
        //pWnd->win代表是否继续采集，因为当停止采集的时候，只有当所有的  
        //缓冲数组都腾出来之后才能close，所以需要停止采集时就不要再waveInAddBuffer了  
        // if (pWnd->win)
        waveInAddBuffer(hwavein, (PWAVEHDR)dwParam1, sizeof (WAVEHDR));
        break;

    case WIM_CLOSE:
        printf("WIM_CLOSE\n");
        break;
    default:
        break;
    }

    return 0;
}

bool CTestIat::iflyInit()
{
    ///APPID请勿随意改动
    const char* login_configs = "appid = 538849f9, work_dir =   .  ,timeout=5000";//登录参数 test:538849f9, user:538293af
    int ret = 0;

    //用户登录
    cout << "[CTestIat::iflyInit] MSPLogin" << endl;
    ret = MSPLogin(NULL, NULL, login_configs);//第一个参数为用户名，第二个参数为密码，第三个参数是登录参数，用户名和密码需要在http://open.voicecloud.cn注册并获取appid
    if (ret != MSP_SUCCESS)
    {
        printf("[CTestIat::iflyInit] MSPLogin failed , Error code %d.\n", ret);
        return false;
    }

    return true;
}

void CTestIat::iflyFini()
{
    MSPLogout();//退出登录
    cout << "[CTestIat::iflyFin] MSPLogout" << endl;
}

bool CTestIat::iflyStartSession()
{
    const char* param = "ssm=1,sub=iat,auf=audio/L16;rate=16000,aue=speex-wb,ent=sms16k,rst=plain,rse=gb2312,vad_speech_tail=2000,timeout=5000";//可参考可设置参数列表
    int errCode = 10;

    m_sessionID = (char *)QISRSessionBegin(NULL, param, &errCode);//开始一路会话
    if (errCode){
        cout << "[CTestIat::iflySendSession] fail to QISRSessionBegin, errCode(" << errCode << "):" << CUtility::getErrDict(IL_MSP_ERR_CODE_DICT, errCode) << endl;
    }

    if (!m_sessionID){
        return false;
    }

    m_bFirst = true;
    //cout << "[CTestIat::iflyStartSession] QISRSessionBegin: " << m_sessionID << endl;
    return true;
}

void CTestIat::iflyStopSession()
{
    //cout << "[CTestIat::iflyStopSession] QISRSessionEnd: " << m_sessionID << endl;
    if (m_sessionID){
        QISRSessionEnd(m_sessionID, NULL);
        m_sessionID = NULL;
    }
}

bool CTestIat::iflySendSession(char *pPCM, int pcmSize)
{
    int errCode = 10;
    int audStat = MSP_AUDIO_SAMPLE_CONTINUE;
    int epStatus = MSP_EP_LOOKING_FOR_SPEECH;
    int recStatus = MSP_REC_STATUS_SUCCESS;

    dumpData(pPCM, pcmSize);

    assert(pcmSize < 12800);
    if (m_bFirst){
        audStat = MSP_AUDIO_SAMPLE_FIRST;
        m_bFirst = false;
    }

    int ret = QISRAudioWrite(m_sessionID, (const void *)pPCM, pcmSize, audStat, &epStatus, &recStatus);//写音频
    cout << "eps: " << g_iflyEpStatus[epStatus] << ", rss: " << g_iflyRecStatus[recStatus] << ", gap: " << CUtility::getTimeGap() << " ms" << endl; //  ", ret(" << ret << "):" << CUtility::getErrDict(IL_MSP_ERR_CODE_DICT, ret) << endl;;

    if (ret != 0){
        cout << "[CTestIat::iflySendSession] QISRAudioWrite fail, errCode(" << ret << "):" << CUtility::getErrDict(IL_MSP_ERR_CODE_DICT, ret) << endl;
        return false;
    }

    if (recStatus == MSP_REC_STATUS_SUCCESS) {
        const char *rslt = QISRGetResult(m_sessionID, &recStatus, 0, &errCode);//服务端已经有识别结果，可以获取
        if (NULL != rslt){
            cout << "iat: " << rslt << endl;
            strcat_s(m_result, rslt);
        }
        
        if (errCode){
            cout << "[CTestIat::iflySendSession] fail to QISRGetResult, errCode(" << errCode << "):" << CUtility::getErrDict(IL_MSP_ERR_CODE_DICT, errCode) << endl;
        }
    }

    // over, stop audio input
    if (epStatus >= MSP_EP_AFTER_SPEECH){
        cout << "识别结束: " << m_result << endl;
        return false;
    }

    // continue receive audio
    return true;
}

bool CTestIat::initMeta()
{
    if (m_szMetaPathFile.empty()){
        return false;
    }

    setMeta(CMD_STATUS_STOP);
    saveText("");
    return true;
}


bool CTestIat::getMeta(int &nCmd)
{
    if (m_szMetaPathFile.empty()){
        cout << "用户未配置命令输入文件" << endl;
        return false;
    }

    string szText;
    if (CUtility::readText(m_szMetaPathFile, szText)){
        nCmd = atoi(szText.c_str());
        if (nCmd >= CMD_STATUS_STOP && nCmd <= CMD_STATUS_DONE){
            return true;
        }

        cout << "[CTestIat::getMeta]read error cmd: " << nCmd << endl; 
        cout << "读取到错误命令[1:Stop,2:Ready,3:RUN,4:Done]： " << nCmd << endl;
    }
    else{
        cout << "[CTestIat::getMeta]Fail to read file: " << m_szMetaPathFile << endl;
        cout << "应用无法正常启动，未发现命令文件：" << m_szMetaPathFile << endl;
    }

    system("pause");
    return false;
}

bool CTestIat::setMeta(int nStatus)
{
    if (m_szMetaPathFile.empty()){
        return false;
    }

    assert(nStatus == CMD_STATUS_STOP || nStatus == CMD_STATUS_DONE || nStatus == CMD_STATUS_RUN);
    //string szText = itoa(nStatus);
    char buf[8] = {0,};
    sprintf_s(buf, "%d", nStatus);
    //szText = buf;
    if (CUtility::writeText(m_szMetaPathFile, buf)){
        return true;
    }
    else{
        cout << "[CTestIat::getMeta]Fail to write file: " << m_szMetaPathFile << endl;
    }

    return false;
}

bool CTestIat::checkStartCmd()
{
    int nCmd = 0;
    if (getMeta(nCmd) && nCmd == CMD_STATUS_READY){
        return true;
    }

    return false;
}

bool CTestIat::checkStopCmd()
{
    int nCmd = 0;
    if (getMeta(nCmd) && nCmd == CMD_STATUS_STOP){
        return true;
    }

    return false;
}

bool CTestIat::logDoneStat()
{
    if (setMeta(CMD_STATUS_DONE)){
        return true;
    }

    return false;
}

bool CTestIat::logRuningStat()
{
    if (setMeta(CMD_STATUS_RUN)){
        return true;
    }

    return false;
}

bool CTestIat::saveText(string szResult)
{
    if (m_szOutputPathFile.empty()){
        return false;
    }

    // filter the last 中文 "，", "。","！","？"
    if (szResult.size() > 2){
        string szFilter = szResult.substr(szResult.size() - 2);
        if (szFilter == "，" || szFilter == "。" || szFilter == "！" || szFilter == "？"){
            szResult = szResult.substr(0, szResult.size() - 2);
        }
    }

    string utf8Text = CUtility::GBToUTF8(szResult.c_str());
    if (CUtility::writeText(m_szOutputPathFile, utf8Text)){
        return true;
    }
    else{
        cout << "[CTestIat::saveText]Fail to write file: " << m_szOutputPathFile << endl;
    }

    return false;
}

void CTestIat::initErrCode()
{
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_SUCCESS, "成功");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_FAIL, "失败");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_EXCEPTION, "异常");

    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_TIME_OUT, "超时");

    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NOT_INIT, "未初始化");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_INVALID_HANDLE, "无效的会话ID");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_INVALID_PARA, "无效的参数");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NO_DATA, "没有数据");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NO_LICENSE, "没有授权许可");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_DB_INVALID_APPID, "无效Appid");

    // networking error
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_GENERAL, "网络一般错误");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_OPENSOCK, "打开套接字错误");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_CONNECTSOCK, "套接字连接错误");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_ACCEPTSOCK, "套接字接收错误");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_SENDSOCK, "发送错误");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_RECVSOCK, "接收错误");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_INVALIDSOCK, "无效的套接字");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_DNS, "DNS解析错误");

    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_NOERROR, "成功");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_ALLOCATED, "资源已被分配");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_BADDEVICEID, "无效的设备ID");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_NODRIVER, "无有效设备驱动");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_NOMEM, "内存不足");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, WAVERR_BADFORMAT, "无效的wave音频格式");
}
