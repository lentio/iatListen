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
    cerr << "��������" << endl;
    cerr << "usage:" << endl;
    cerr << "iatListen -iat_key ---������д����ģʽ -iat_key" << endl;
    cerr << "iatListen -iat_file <input file> <output file> ---������д��Ĭģʽ -iat_file <����ָ���ļ�> <���ʶ���ļ�>" << endl;
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
        cout << "��ǰģʽ:Ѷ����������д����ģʽ" << endl;
        cout << "excute: " << argv[0] << " " << argv[1] << endl;
        test_iat_key();
    }
    else if (!strcmp(argv[1], "-iat_file")) {
        cout << "��ǰģʽ:Ѷ����������д��Ĭģʽ" << endl;
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
