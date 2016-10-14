#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"

class RecordManager {
private:
	BufPageManager* bufPageManager;
	bool initFileHead(const char* name, int recordSize) {
		int fileID;
		int index;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, index, false);	//SIZE=PAGE_INT_NUM=2048
		b[0] = recordSize;												//�ļ��м�¼����
		b[1] = PAGE_SIZE / recordSize;									//�ļ�ÿһҳ���Դ洢����¼��
		b[2] = 0;														//�ļ��м�¼����
		bufPageManager->markDirty(index);
		bufPageManager->writeBack(index);
	}
public:
	//File Operation
	bool createFile(const char* name, int recordSize) {
		if (bufPageManager->fileManager->createFile(name)) {
			if (initFileHead(name, recordSize))
				return true;
			remove(name);
			return false;
		}
		return false;
	}
	bool deleteFile(const char* name) {
		if (remove(name) == 0)
			return true;
		return false;
	}
	bool openFile(const char* name, int& fileID) {
		return bufPageManager->fileManager->openFile(name, fileID);
	}
	int closeFile(int fileID) {
		return bufPageManager->fileManager->closeFile(fileID);
	}

	//Record Operation
	bool insertRecord(int fileID, unsigned char[] record, int& rID) {
		int index;
		int recordSize;
		int recordNum;
		BufType b = bufPageManager->getPage(fileID, 0, index);
		recordSize = b[0];	//���ļ��м�¼�ĳ��ȣ���¼�������UNSIGNED_MAX_INT��
		recordNum = b[1];   //���ļ��м�¼�ܸ���

	}
	bool deleteRecord() {

	}
	bool updateRecord() {

	}
	bool getRecord() {

	}
	RecordManager(BufPageManager* bpm) {
		MyBitMap::initConst();
		bufPageManager = bpm;
	}
}
