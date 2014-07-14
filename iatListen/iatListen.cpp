// iatListen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TestIat.h"

void test_iat_file(string szMetaPathFile, string szOutPathFile)
{
    CTestIat iat(szMetaPathFile, szOutPathFile);
    if (!iat.Init()){
        return;
    }

    iat.LoopFile();
}

void test_iat_key()
{
    CTestIat iat("", "");
    if (!iat.Init()){
        return;
    }

    iat.LoopKey();
}

void showUsage()
{
    cerr << "参数错误" << endl;
    cerr << "usage:" << endl;
    cerr << "iatListen -iat_key ---语音听写交互模式 -iat_key" << endl;
    cerr << "iatListen -iat_file <input file> <output file> ---语音听写静默模式 -iat_file <输入指令文件> <输出识别文件>" << endl;
    system("pause");
}

void showVersion()
{
    cout << "iatListen" << endl;
    cout << "version:" << "1.0.1" << endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        showUsage();
        return -1;
    }

    showVersion();
    if (!strcmp(argv[1], "-iat_key")) {
        cout << "当前模式:讯飞云语音听写交互模式" << endl;
        cout << "excute: " << argv[0] << " " << argv[1] << endl;
        test_iat_key();
    }
    else if (!strcmp(argv[1], "-iat_file")) {
        cout << "当前模式:讯飞云语音听写静默模式" << endl;
        if (argc < 4) {
            showUsage();
            return -1;
        }

        cout << "excute: " << argv[0] << " " << argv[1] << " " << argv[2] << " " << argv[3] << endl;
        test_iat_file(argv[2], argv[3]);
    }
    else{
        cout << "excute: " << argv[0] << " " << argv[1] << endl;
        showUsage();
        return -1;
    }

	return 0;
}
