#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"

#define PAGE_NUM_BETWEEN_FREEPAGE 4096

class RecordManager {
private:
	BufPageManager* bufPageManager;
	bool initFileHead(const char* name, int[] colSize, int colNum) {
		
		//��¼�ĳ���
		int recordSize = 0;
		for (int i = 0; i < colNum; i++)
			recordSize += colSize[i];

		int fileID;
		int index;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, index, false);	//SIZE=PAGE_INT_NUM=2048
		
		b[0] = recordSize;												//�ļ��м�¼����
		b[1] = PAGE_SIZE / recordSize;									//�ļ�ÿһҳ���Դ洢����¼��
		b[2] = 0;														//�ļ��м�¼����
		b[3] = colNum;													//�������
		for (int i = 4; i < colNum + 4; i++)							//��ÿһ�е�size
			b[i] = colSize[i - 4];

		bufPageManager->markDirty(index);
		bufPageManager->writeBack(index);
	}
public:
	//File Operation
	bool createFile(const char* name, int[] colSize, int colNum) {
		if (bufPageManager->fileManager->createFile(name)) {
			if (initFileHead(name, colSize, colNum))
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
	bool insertRecord(int fileID, unsigned char[] record, int& recordID) {
		int index;
		int recordSize;
		int recordNumPerFile;
		BufType b = bufPageManager->getPage(fileID, 0, index);
		recordSize = b[0];			//���ļ��м�¼�ĳ��ȣ���¼�������UNSIGNED_MAX_INT��
		recordNumPerFile = b[1];	//ÿһҳ������¼��

		b[2] += 1;
		bufPageManager->markDirty(index);




	}
	bool deleteRecord(int fileID, int pageID, int recordID) {
		int fileHeadIndex;
		int recordSize;
		BufType fileHeadBuf = bufPageManager->getPage(fileID, 0, fileHeadIndex);
		recordSize = fileHeadBuf[0];
		
		BufType buf = bufPageManager->getPage(fileID, pageID, index);

	}
	bool updateRecord() {

	}
	int[][3] getRecord(int fileID) {

	}
	RecordManager(BufPageManager* bpm) {
		MyBitMap::initConst();
		bufPageManager = bpm;
	}
}
