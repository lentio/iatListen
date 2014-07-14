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
    "ʶ��ɹ�",
    "ʶ�������û��ʶ����",
    "����ʶ����",
    "����",
    "������Ч��Ƶ",
    "ʶ�����",
    "����",
    "����",
    "����",
    "����"
};

char g_iflyEpStatus[][MAX_STRING_SIZE] =
{
    "��û�м�⵽��Ƶ��ǰ�˵�",
    "���ڽ�����Ƶ����",
    "NULL",
    "��⵽��Ƶ�ĺ�˵�",
    "��ʱ",
    "���ִ���",
    "��Ƶ����"
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
        cout << "�޷����ӵ�Ѷ������ƽ̨��������������" << endl;
        return false;
    }

    if (!waveInit()){
        //cout << "[CTestIat::Init] fail to waveInit" << endl;
        cout << "�޷��򿪱�����Ƶ�����豸" << endl;

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
        cout << "����δ��ʼ��" << endl;
        return false;
    }
        
    cout << "��S����ʼ������⣬��Q���뿪..." << endl;
    char key = 0;
    while (1){
        // ����м�������
        if (_kbhit()) {
            key = _getch();
            cout << "press key�� " << key << endl;
            if (key == 's'){ // Start iat
                StartAudio();
            }
            else if (key == 'q'){ // eXit
                m_bStart = false;
                cout << "�˳�ѭ�����" << endl;
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
        cout << "����δ��ʼ��" << endl;
        return false;
    }

    cout << "��ȡ�ļ�[" << m_szMetaPathFile << "]��ȡָ���Q���˳�..." << endl;
    //initMeta();
    char key = 0;
    while (1){
        // ����м�������
        if (_kbhit()) {
            key = _getch();
            if (key == 'q'){ // eXit
                m_bStart = false;
                cout << "�û����˳�����" << endl;
                break;
            }
        }

        checkCmdFile();
        if (m_bStopAudio){
            // cout << "[CTestIat::LoopFile] stop detect audio" << endl;
            StopAudio();

            // update meta and text file
            if (saveText(m_result)){
                cout << "[CTestIat::LoopFile]�������ļ�: " << m_szOutputPathFile << endl;
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

    //������wHdr��ӵ�waveIn��ȥ  
    //waveInAddBuffer(m_hWaveIn, &m_wHdr1, sizeof (WAVEHDR));
    //waveInAddBuffer(m_hWaveIn, &m_wHdr2, sizeof (WAVEHDR));
    for (int i = 0; i < IAT_MAX_AUDIO_BUFFERS; ++i){
        waveInAddBuffer(m_hWaveIn, &m_wHdr[i], sizeof (WAVEHDR));
    }

    //��ʼ��Ƶ�ɼ�  
    waveInStart(m_hWaveIn);
    cout << "��ʼ������д..." << endl;
    return true;
}

void CTestIat::StopAudio()
{
    if (m_bStart){
        m_bStart = false;
        //memset(m_result, 0, sizeof(m_result));
        iflyStopSession();

        //ֹͣ��Ƶ�ɼ�  
        waveInStop(m_hWaveIn);

        // waveFini();
        //closeDumpFile();
        m_bStopAudio = false;
        cout << "������д������" << endl;

        dumpWaveFile();
    }
}

bool CTestIat::waveInit()
{
    WAVEFORMATEX waveform; //�ɼ���Ƶ�ĸ�ʽ���ṹ��  
    waveform.wFormatTag = WAVE_FORMAT_PCM;//������ʽΪPCM  
    waveform.nSamplesPerSec = 16000;//�����ʣ�16000��/��  
    waveform.wBitsPerSample = 16;//�������أ�16bits/��  
    waveform.nChannels = 1;//������������������  
    waveform.nAvgBytesPerSec = 16000 * 2;//ÿ��������ʣ�����ÿ���ܲɼ������ֽڵ�����  
    waveform.nBlockAlign = 2;//һ����Ĵ�С������bit���ֽ�������������  
    waveform.cbSize = 0;//һ��Ϊ0  

    //ʹ��waveInOpen����������Ƶ�ɼ�  
    MMRESULT mmr = waveInOpen(&m_hWaveIn, WAVE_MAPPER, &waveform,
        (DWORD)(MicCallback), DWORD(this), CALLBACK_FUNCTION);
    if (mmr != MMSYSERR_NOERROR){
        cout << "[CTestIat::waveInit] fail to waveInOpen�� error(" << mmr << "): " << CUtility::getErrDict(IL_WAVE_ERR_CODE_DICT, mmr) << endl;
        return false;
    }

    ////�����������飨������Խ���������飩����������Ƶ����  
    //BYTE *pBuffer1, *pBuffer2;//�ɼ���Ƶʱ�����ݻ���  
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

    ////�������õ�m_wHdr1��Ϊ����  
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

    ////�������õ�m_wHdr2��Ϊ����  
    //waveInPrepareHeader(m_hWaveIn, &m_wHdr2, sizeof(WAVEHDR));

    for (int i = 0; i < IAT_MAX_AUDIO_BUFFERS; ++i){
        BYTE *pBuffer1;//�ɼ���Ƶʱ�����ݻ���  
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

        //�������õ�m_wHdr1��Ϊ����  
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
        unsigned short wBlockAlign; // ÿ����������λ��(���ֽ���), ��ֵΪ:ͨ��
        // ��*ÿ����������λֵ/8�����������Ҫһ�δ�
        // ������ֵ��С���ֽ�����, �Ա㽫��ֵ����
        // �������ĵ���ÿ����ռ�����ֽ�:
        // NumChannels * uiBitsPerSample/8
        unsigned short uiBitsPerSample; // quantization
    }FORMAT;

    // 12 bytes
    // ���ݽṹ
    typedef struct {
        char fccID[4]; // must be "data"
        unsigned long dwSize; // byte_number of PCM data in byte
    }DATA;

    // 24 bytes
    // The fact chunk is required for all new WAVE formats.
    // and is not required for the standard WAVE_FORMAT_PCM files
    // Ҳ����˵������ṹ��Ŀǰ���Ǳ���ģ�һ�㵱wav�ļ���ĳЩ���ת�����ɣ��������Chunk
    // ���������д�ˣ�����������µĽṹ���������ĸ��ṹ���е�λ��ҲҪ���ڵ���
    typedef struct {
        char fccID[4]; // must be "fact"
        unsigned long id; // must be 0x4
        unsigned long dwSize; // ��ʱû������ɶ��
    }FACT;

    WAVE_HEADER WaveHeader;
    FORMAT WaveFMT;
    DATA WaveData;
    FACT WaveFact;
    memset(&WaveHeader, 0, sizeof(WAVE_HEADER));
    memcpy(WaveHeader.fccID, "RIFF", 4);
    memcpy(WaveHeader.fccType, "WAVE", 4);

    // dwSize ������wave�ļ��Ĵ�С���ֽ�������������������HEADER�е�ǰ�������ṹ:
    // HEADER.fccID��HEAD.dwSize)
    // WaveHeader.dwSize = length + 0x24; // �����д��fact������36���ֽڣ�
    // 44- 8 = 36��

    WaveHeader.dwSize = nWaveSize + 0x30; // ���д��fact������48 ��bytes
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
    WaveFact.dwSize = nWaveSize; // ���ֵ��֪��ʲô��˼
    WaveFact.id = 0x4;
    memset(&WaveData, 0, sizeof(DATA));
    memcpy(WaveData.fccID, "data", 4);
    WaveData.dwSize = nWaveSize;
    
    m_sfile.write((char *)&WaveHeader, sizeof(WAVE_HEADER));
    m_sfile.write((char *)&WaveFMT, sizeof(FORMAT));
    m_sfile.write((char *)&WaveFact, sizeof(FACT)); // fact���Ǳ����
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
    cout << "���������ļ�:" << szDumpFile << endl;
}

//���������callback���������ڲɼ�������Ƶ���ݶ�����������д���  
DWORD CALLBACK CTestIat::MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    //���CIMAADPCMDlg���������Ƶ�ɼ���  
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

        //������ǶԲɼ���������������ĵط����������˷��ʹ���  
        //((PWAVEHDR)dwParam1)->lpData����ǲɼ���������ָ��  
        //((PWAVEHDR)dwParam1)->dwBytesRecorded����ǲɼ��������ݳ���  
        if (!pThis->iflySendSession(((PWAVEHDR)dwParam1)->lpData, ((PWAVEHDR)dwParam1)->dwBytesRecorded)){
            //cout << "[CTestIat::MicCallback] need to stop audio sesion" << endl;
            pThis->m_bStopAudio = true;
            return -1;
        }

        //::Sleep(100);

        //��������֮��Ҫ�ٰ������������ӻ�ȥ  
        //pWnd->win�����Ƿ�����ɼ�����Ϊ��ֹͣ�ɼ���ʱ��ֻ�е����е�  
        //�������鶼�ڳ���֮�����close��������Ҫֹͣ�ɼ�ʱ�Ͳ�Ҫ��waveInAddBuffer��  
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
    ///APPID��������Ķ�
    const char* login_configs = "appid = 538849f9, work_dir =   .  ,timeout=5000";//��¼���� test:538849f9, user:538293af
    int ret = 0;

    //�û���¼
    cout << "[CTestIat::iflyInit] MSPLogin" << endl;
    ret = MSPLogin(NULL, NULL, login_configs);//��һ������Ϊ�û������ڶ�������Ϊ���룬�����������ǵ�¼�������û�����������Ҫ��http://open.voicecloud.cnע�Ტ��ȡappid
    if (ret != MSP_SUCCESS)
    {
        printf("[CTestIat::iflyInit] MSPLogin failed , Error code %d.\n", ret);
        return false;
    }

    return true;
}

void CTestIat::iflyFini()
{
    MSPLogout();//�˳���¼
    cout << "[CTestIat::iflyFin] MSPLogout" << endl;
}

bool CTestIat::iflyStartSession()
{
    const char* param = "ssm=1,sub=iat,auf=audio/L16;rate=16000,aue=speex-wb,ent=sms16k,rst=plain,rse=gb2312,vad_speech_tail=2000,timeout=5000";//�ɲο������ò����б�
    int errCode = 10;

    m_sessionID = (char *)QISRSessionBegin(NULL, param, &errCode);//��ʼһ·�Ự
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

    int ret = QISRAudioWrite(m_sessionID, (const void *)pPCM, pcmSize, audStat, &epStatus, &recStatus);//д��Ƶ
    cout << "eps: " << g_iflyEpStatus[epStatus] << ", rss: " << g_iflyRecStatus[recStatus] << ", gap: " << CUtility::getTimeGap() << " ms" << endl; //  ", ret(" << ret << "):" << CUtility::getErrDict(IL_MSP_ERR_CODE_DICT, ret) << endl;;

    if (ret != 0){
        cout << "[CTestIat::iflySendSession] QISRAudioWrite fail, errCode(" << ret << "):" << CUtility::getErrDict(IL_MSP_ERR_CODE_DICT, ret) << endl;
        return false;
    }

    if (recStatus == MSP_REC_STATUS_SUCCESS) {
        const char *rslt = QISRGetResult(m_sessionID, &recStatus, 0, &errCode);//������Ѿ���ʶ���������Ի�ȡ
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
        cout << "ʶ�����: " << m_result << endl;
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
        cout << "�û�δ�������������ļ�" << endl;
        return false;
    }

    string szText;
    if (CUtility::readText(m_szMetaPathFile, szText)){
        nCmd = atoi(szText.c_str());
        if (nCmd >= CMD_STATUS_STOP && nCmd <= CMD_STATUS_DONE){
            return true;
        }

        cout << "[CTestIat::getMeta]read error cmd: " << nCmd << endl; 
        cout << "��ȡ����������[1:Stop,2:Ready,3:RUN,4:Done]�� " << nCmd << endl;
    }
    else{
        cout << "[CTestIat::getMeta]Fail to read file: " << m_szMetaPathFile << endl;
        cout << "Ӧ���޷�����������δ���������ļ���" << m_szMetaPathFile << endl;
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

    // filter the last ���� "��", "��","��","��"
    if (szResult.size() > 2){
        string szFilter = szResult.substr(szResult.size() - 2);
        if (szFilter == "��" || szFilter == "��" || szFilter == "��" || szFilter == "��"){
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
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_SUCCESS, "�ɹ�");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_FAIL, "ʧ��");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_EXCEPTION, "�쳣");

    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_TIME_OUT, "��ʱ");

    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NOT_INIT, "δ��ʼ��");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_INVALID_HANDLE, "��Ч�ĻỰID");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_INVALID_PARA, "��Ч�Ĳ���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NO_DATA, "û������");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NO_LICENSE, "û����Ȩ���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_DB_INVALID_APPID, "��ЧAppid");

    // networking error
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_GENERAL, "����һ�����");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_OPENSOCK, "���׽��ִ���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_CONNECTSOCK, "�׽������Ӵ���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_ACCEPTSOCK, "�׽��ֽ��մ���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_SENDSOCK, "���ʹ���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_RECVSOCK, "���մ���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_INVALIDSOCK, "��Ч���׽���");
    CUtility::setErrDict(IL_MSP_ERR_CODE_DICT, MSP_ERROR_NET_DNS, "DNS��������");

    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_NOERROR, "�ɹ�");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_ALLOCATED, "��Դ�ѱ�����");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_BADDEVICEID, "��Ч���豸ID");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_NODRIVER, "����Ч�豸����");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, MMSYSERR_NOMEM, "�ڴ治��");
    CUtility::setErrDict(IL_WAVE_ERR_CODE_DICT, WAVERR_BADFORMAT, "��Ч��wave��Ƶ��ʽ");
}
