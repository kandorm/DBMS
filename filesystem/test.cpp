#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"
#include <iostream>

using namespace std;

int main() {
	MyBitMap::initConst();   //�¼ӵĳ�ʼ��
	FileManager* fm = new FileManager();
	BufPageManager* bpm = new BufPageManager(fm);
	fm->createFile("testfile.txt"); //�½��ļ�
	int fileID;
	fm->openFile("testfile.txt", fileID); //���ļ���fileID�Ƿ��ص��ļ�id
	for (int pageID = 0; pageID < 1000; ++pageID) {
		int index;
		//ΪpageID��ȡһ������ҳ
		BufType b = bpm->getPage(fileID, pageID, index, false);
		//ע�⣬��allocPage����getPage��ǧ��Ҫ����delete[] b�����Ĳ���
		//�ڴ�ķ���͹�����BufPageManager�����ã�����Ҫ���ģ���������ͷŻᵼ������
		for (int i = 0; i < 8192; i++)
			cout << "b[" << i << "]=" <<b[i] << endl;
		b[0] = pageID; //�Ի���ҳ����д����
		b[1] = fileID;
		bpm->markDirty(index); //�����ҳ
							   //�����µ���allocPage��ȡ��һ��ҳ������ʱ��û�н�ԭ��bָ����ڴ��ͷŵ�
							   //��Ϊ�ڴ������BufPageManager��������
	}
	bpm->close();
	return 0;
}