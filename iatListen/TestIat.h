#pragma once

#define CMD_STATUS_STOP      0
#define CMD_STATUS_READY     1
#define CMD_STATUS_RUN       2
#define CMD_STATUS_DONE      3

#define IAT_MAX_AUDIO_BUFFERS     5

class CTestIat
{
public:
    CTestIat(string szMetaPathFile, string szOutputPathFile);
    ~CTestIat();

    bool LoopKey();
    bool LoopFile();

    bool Init();
    void Fini();

private:
    bool StartAudio();
    void StopAudio();


    bool waveInit();
    void waveFini();
    static DWORD CALLBACK CTestIat::MicCallback(HWAVEIN hwavein, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

    bool iflyInit();
    void iflyFini();

    bool iflyStartSession();
    void iflyStopSession();
    bool iflySendSession(char *pPCM, int pcmSize);

    // dump file
    void dumpData(char *pPCM, int pcmSize);
    void dumpWaveHead(int nWaveSize);
    void dumpWaveFile();

    // fime mode
    bool checkCmdFile();

    bool saveText(string szResult);

    bool checkStartCmd();
    bool checkStopCmd();

    bool logDoneStat();
    bool logRuningStat();

    bool getMeta(int &nCmd);
    bool setMeta(int nStatus);

    bool initMeta();
    void initErrCode();

private:
    HWAVEIN m_hWaveIn;  //输入设备  

    //采集音频时包含数据缓存的结构体  
    //WAVEHDR m_wHdr1;
    //WAVEHDR m_wHdr2; 
    WAVEHDR m_wHdr[IAT_MAX_AUDIO_BUFFERS];

    bool m_bInit;
    bool m_bStart;
    bool m_bStopAudio;

    bool m_bFirst;
    char *m_sessionID;
    char m_result[1024];

    ofstream m_sfile;

    string m_szMetaPathFile;
    string m_szOutputPathFile;

    char *m_pWaveData;
    int   m_nWaveDataSize;
};

